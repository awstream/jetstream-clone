#ifndef JetStream_cube_h
#define JetStream_cube_h

#include <iterator>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <boost/functional/hash.hpp>
#include "cube_iterator.h"
#include "subscriber.h"
#include "queue_congestion_mon.h"
#include "chained_queue_mon.h"
#include "node_config.h"

#include <boost/shared_ptr.hpp>
#include "jetstream_types.pb.h"
#include "js_executor.h"
#include <boost/interprocess/detail/atomic.hpp>

// For cube process threads
#define BLOCKING_QUEUE 1
#define NONBLOCK_QUEUE 0

#if NONBLOCK_QUEUE
//#include <boost/lockfree/spsc_queue.hpp>
//using boost::lockfree::spsc_queue;

#include <boost/lockfree/queue.hpp>
using boost::lockfree::queue;

#endif

namespace jetstream {
class DataCube;
class TupleProcessingInfo;
typedef std::string DimensionKey;
}

#include "tuple_batch.h"
#include "operator_chain.h"

namespace jetstream {

/**
*  A class to represent a cube in memory.
*/


class TupleProcessingInfo {
  public:
    TupleProcessingInfo(boost::shared_ptr<Tuple> t, DimensionKey key, boost::shared_ptr<std::vector<unsigned int> > levels): t(t), key(key), levels(levels),  need_new_value(false), need_old_value(false) {};
    boost::shared_ptr<Tuple> t;
    DimensionKey key;
    boost::shared_ptr<std::vector<unsigned int> > levels;
    std::list<operator_id_t> insert;
    std::list<operator_id_t> update;
    bool need_new_value;
    bool need_old_value;
};

class FlushInfo {
  public:

    unsigned id;
    boost::shared_ptr<cube::Subscriber> subsc;
    void set_count(unsigned c) {
      count = c;
    }
  
    unsigned dec_and_get() {
      unsigned v = boost::interprocess::ipcdetail::atomic_dec32(&count);
      return v;
    }
    
  private:
    volatile unsigned count;
};


class ProcessThreadTask {
  public:
    bool is_insert;
    ProcessThreadTask(bool i): is_insert(i) {};
    virtual ~ProcessThreadTask() {}
//    virtual void run(ProcessCallable * p) = 0;
};


class TupleInsertion: public ProcessThreadTask {
  public:
    TupleInsertion(OperatorChain * c,
                   boost::shared_ptr<Tuple> t,
                   DimensionKey k, boost::shared_ptr<std::vector<unsigned int> > levs):
      ProcessThreadTask(true),chain(c), tuple(t), key(k), levels(levs) {}
    OperatorChain * chain;
    boost::shared_ptr<Tuple> tuple;
    DimensionKey key;
    boost::shared_ptr<std::vector<unsigned int> > levels;
};

class FlushTask: public ProcessThreadTask {
  public:
    FlushTask(const boost::shared_ptr<FlushInfo>& f): ProcessThreadTask(false), flush(f) {}
    boost::shared_ptr<FlushInfo> flush;
};

class ProcessCallable {
//  friend class TupleInsertion;
//  friend FlushInfo;
  public:
    ProcessCallable(DataCube * cube, std::string name);
    ~ProcessCallable();

    void run_process();
    void run_flush();

    void assign(OperatorChain * chain, const boost::shared_ptr<Tuple>& t, DimensionKey key, const boost::shared_ptr<std::vector<unsigned int> >& levels);
    boost::shared_ptr<cube::TupleBatch> batch_flush();
    bool batcher_ready();
    void check_flush();
    void barrier(boost::shared_ptr<FlushInfo>); //called from outside

    mutable boost::mutex batcherLock; // protects tupleBatcher

  private:
    std::string name;
    boost::thread thread_process;
    boost::thread thread_flush;
#if BLOCKING_QUEUE
    std::queue< ProcessThreadTask* > process_tasks;
  
    boost::mutex process_lock;
    boost::condition_variable has_data;
#elif NONBLOCK_QUEUE
    boost::lockfree::queue< ProcessThreadTask* >  process_tasks;
#endif
    unsigned cntr;

    volatile bool process_is_running;
  
      //each service has a single thread associated with it so posts will be ordered.
 //   shared_ptr<io_service> service_process;
    shared_ptr<io_service> service_flush;
//    io_service::work work_process;
    io_service::work work_flush;


    jetstream::DataCube * cube;

    // This runs in the thread_process thread
    void do_check_flush();

    boost::shared_ptr<cube::TupleBatch> tupleBatcher;
  
    void barrier_to_flushqueue(boost::shared_ptr<FlushInfo>); //invoked on service_process
    void do_barrier(boost::shared_ptr<FlushInfo>); //invoked on flush-strand  
};


class DataCube : public ChainMember {
  public:
    typedef jetstream::DimensionKey DimensionKey;
    friend class ProcessCallable;

    DataCube(jetstream::CubeSchema _schema, std::string _name, const NodeConfig &conf);
    virtual ~DataCube() {}

    //main external functions
    virtual void create() = 0;
    virtual void destroy() = 0;

    /**
     * @brief insert the tuple for processing. Note that it is not safe to alter the tuple after calling process
     * (until do_process returns).
     *
     * @param t
     */

    virtual void process(OperatorChain * chain, boost::shared_ptr<Tuple> t);
    virtual void process_delta (OperatorChain * c, Tuple& oldV, boost::shared_ptr<Tuple> newV) = 0;

    virtual void process(OperatorChain * chain,  std::vector<boost::shared_ptr<Tuple> > &, DataplaneMessage&);
    virtual bool is_source() {return false;}


    void wait_for_commits();


    //query functions
    static unsigned int const LEAF_LEVEL;
    virtual boost::shared_ptr<jetstream::Tuple>
    get_cell_value(jetstream::Tuple const &t, std::vector<unsigned int> const &levels, bool final = true) const= 0;

    virtual jetstream::Tuple get_sourceformat_tuple(const jetstream::Tuple &t) const = 0;

    virtual cube::CubeIterator
    slice_query(jetstream::Tuple const &min, jetstream::Tuple const& max,
                bool final = true, std::list<std::string> const &sort = std::list<std::string>(),
                size_t limit = 0) const = 0;

    virtual cube::CubeIterator rollup_slice_query(std::vector<unsigned int> const &levels,
        jetstream::Tuple const &min, jetstream::Tuple const &max, bool final = true,
        std::list<std::string> const &sort = std::list<std::string>(), size_t limit = 0) const = 0;

    virtual cube::CubeIterator slice_and_rollup( std::vector<unsigned int> const &levels,
                                                 jetstream::Tuple const &min,
                                                 jetstream::Tuple const& max,
                                                 std::list<std::string> const &sort = std::list<std::string>(),
                                                 size_t limit = 0) const = 0;

    virtual void
    do_rollup(std::vector<unsigned int> const &levels,jetstream::Tuple const &min, jetstream::Tuple const& max) = 0;


    // info/utility functions
    virtual jetstream::cube::CubeIterator end() const = 0;
    virtual size_t num_leaf_cells() const = 0;
    Tuple empty_tuple();
    const jetstream::CubeSchema& get_schema() ;
    virtual std::string id_as_str() const;
    virtual const std::string& typename_as_str() const;
    virtual void chain_stopping(OperatorChain * );
    virtual void add_chain(boost::shared_ptr<OperatorChain>);

    virtual void merge_tuple_into(jetstream::Tuple &into, jetstream::Tuple const &update) const =0;

    virtual std::vector<size_t> dimension_offset(std::string) const = 0; //offsets in tuples
    virtual std::vector<size_t> aggregate_offset(std::string) const = 0;

    virtual size_t num_dimensions() const = 0;
    virtual size_t num_aggregates() const = 0;


    /**
    * It's possible to mark a cube as locked. The intended use of this is to allow
    * graceful deletion. The deleter marks the cube as frozen. As updates to the cube fail,
    * data sources drop their pointer. When the last smart pointer is removed,
    * the cube is deleted.
    *
    * Possibly a different mechanism is needed to do visibility control.
    *
    */
    void mark_as_deleted() {
      is_frozen = true;
    }

    //subscriber stuff
    void add_subscriber(boost::shared_ptr<cube::Subscriber> sub);
    void remove_subscriber(boost::shared_ptr<cube::Subscriber> sub);
    void remove_subscriber(operator_id_t id);

    void flush(boost::shared_ptr<FlushInfo>);


    //only used by tuple batch
    virtual DimensionKey get_dimension_key(Tuple const &t, const std::vector<unsigned int> & levels) const = 0;
    virtual void get_dimension_key(const Tuple &t, const std::vector<unsigned int>&  levels,  std::ostringstream &ostr) const = 0;
    virtual boost::shared_ptr<std::vector<unsigned int> > get_leaf_levels() const = 0;

    void save_callback(jetstream::TupleProcessingInfo &tpi,
                       boost::shared_ptr<jetstream::Tuple> new_tuple, boost::shared_ptr<jetstream::Tuple> old_tuple);

    virtual void save_tuple(jetstream::Tuple const &t, bool need_new_value, bool need_old_value,
                            boost::shared_ptr<jetstream::Tuple> &new_tuple,boost::shared_ptr<jetstream::Tuple> &old_tuple)=0;


  virtual void save_tuple_batch(const std::vector<boost::shared_ptr<jetstream::Tuple> > &tuple_store,
       const std::vector<bool> &need_new_value_store, const std::vector<bool> &need_old_value_store,
       std::vector<boost::shared_ptr<jetstream::Tuple> > &new_tuple_store, std::vector<boost::shared_ptr<jetstream::Tuple> > &old_tuple_store) = 0 ;

  virtual void save_tuple_batch(const std::vector<boost::shared_ptr<jetstream::Tuple> > &tuple_store,
       const std::vector<boost::shared_ptr<std::vector<unsigned int> > > &levels_store,
       const std::vector<bool> &need_new_value_store, const std::vector<bool> &need_old_value_store,
       std::vector<boost::shared_ptr<jetstream::Tuple> > &new_tuple_store, std::vector<boost::shared_ptr<jetstream::Tuple> > &old_tuple_store) =0;

    virtual boost::shared_ptr<CongestionMonitor> congestion_monitor() { return processCongestMon;}

//    virtual void meta_from_upstream(const DataplaneMessage & msg, const operator_id_t pred);

    virtual void do_process(OperatorChain * chain, boost::shared_ptr<Tuple> t, DimensionKey key, boost::shared_ptr<std::vector<unsigned int> > levels, boost::shared_ptr<cube::TupleBatch> &tupleBatcher, ProcessCallable * proc);

    virtual bool is_unrolled(std::vector<unsigned int> levels) const = 0;

    boost::shared_ptr<ChainedQueueMonitor> process_congestion_monitor() { return processCongestMon;}

    virtual void clear_contents() = 0;
    unsigned in_chains();


  protected:
    jetstream::CubeSchema schema;
    std::string name;
    bool is_frozen;
    NodeConfig config;
    unsigned in_chain_count;
  
    std::map<operator_id_t, boost::shared_ptr<jetstream::cube::Subscriber> > subscribers;
    mutable boost::shared_mutex subscriberLock; // protects list of operators; reader/writer semantics

    void set_current_levels(OperatorChain *, const std::vector<unsigned int> &levels);
    void set_current_levels(OperatorChain *, boost::shared_ptr<std::vector<unsigned int> > levels);

    uint64_t version;

  private:
    static const std::string my_tyepename;


  protected:
    boost::shared_ptr<QueueCongestionMonitor> flushCongestMon;
    boost::shared_ptr<ChainedQueueMonitor> processCongestMon;

    std::vector<boost::shared_ptr<ProcessCallable> > processors;
    // The current rollup levels per incoming chain into the cube
    std::map<OperatorChain *, boost::shared_ptr<std::vector<unsigned int> > > current_levels;
};

}

#endif
