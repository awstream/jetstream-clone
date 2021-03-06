#ifndef SUBSCRIBER_2FAEJ0UJ
#define SUBSCRIBER_2FAEJ0UJ

#include "js_counting_executor.h"
#include "jetstream_types.pb.h"
#include "operator_chain.h"
#include "chain_ops.h"


namespace jetstream {

class FlushInfo;
class DataCube;

namespace cube {

class Subscriber: public jetstream::COperator {
  friend class jetstream::DataCube;

//  friend DataCube::add_subscriber(boost::shared_ptr<cube::Subscriber> sub);
  // Would ideally just have the one method as friend but it would be a circular dependency

  protected:
    DataCube * cube;
    boost::shared_ptr<OperatorChain> chain;
  
  public:
    enum Action {NO_SEND,
          SEND,
          SEND_NO_BATCH, //tells cube to do a flush
          SEND_UPDATE} ; //tells cube the update is backfill

    Subscriber (): cube(NULL), exec(1) {};
    virtual ~Subscriber() {};

    //TODO
    bool has_cube() {
      return cube != NULL;
    };

    virtual void process(OperatorChain * chain, std::vector<boost::shared_ptr<Tuple> > &, DataplaneMessage&);
    virtual Action action_on_tuple(OperatorChain * c, boost::shared_ptr<const jetstream::Tuple> const update) = 0;
    virtual bool need_new_value(boost::shared_ptr<const jetstream::Tuple> const update) { return false; }
    virtual bool need_old_value(boost::shared_ptr<const jetstream::Tuple> const update) { return false; }

      //These are the things that should be invoked externally; they call into synchronized code underneath.
      // This helps protect the cube's threads from being consumed by subscribers.
      // They have to be virtual to make the inter-library linkage work right
    virtual void insert_callback(boost::shared_ptr<jetstream::Tuple> const &update,
                                 boost::shared_ptr<jetstream::Tuple> const &new_value);

    virtual void update_callback(boost::shared_ptr<jetstream::Tuple> const &update,
                                 boost::shared_ptr<jetstream::Tuple> const &new_value, 
                                 boost::shared_ptr<jetstream::Tuple> const &old_value);

    virtual void flush_callback(unsigned id);

  /* Note that this may be called OUT OF ORDER with incoming action_on_tuple
   calls. Use a Flush if you need ordering.
  */
    virtual shared_ptr<FlushInfo> incoming_meta(const OperatorChain&,
                                                const DataplaneMessage&) = 0;


    size_t queue_length();
  
    virtual bool is_source() {return true;}

      //adds an OUTGOING chain
    virtual void add_chain(boost::shared_ptr<OperatorChain> c) {chain = c;}


  protected:
        //these are invoked in a synchronized way.
    virtual void post_insert(boost::shared_ptr<jetstream::Tuple> const &update,
                                 boost::shared_ptr<jetstream::Tuple> const &new_value) = 0;

    virtual void post_update(boost::shared_ptr<jetstream::Tuple> const &update,
                                 boost::shared_ptr<jetstream::Tuple> const &new_value, 
                                 boost::shared_ptr<jetstream::Tuple> const &old_value) = 0;

    virtual void post_flush(unsigned id) {}
  
  private:
    void set_cube(DataCube  *c ) {cube = c;} //called from cube.addSubscriber
    CountingExecutor exec;

  
    const static std::string my_type_name;
  public:
    virtual const std::string& typename_as_str() const {
      return my_type_name;
    }
};

} /* cube */



} /* jetsream */

#endif /* end of include guard: SUBSCRIBER_2FAEJ0UJ */
