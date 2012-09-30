#ifndef _dataplaneoperator_H_
#define _dataplaneoperator_H_

#include <sys/types.h>
#include <boost/shared_ptr.hpp>

#include "js_utils.h"
#include "jetstream_types.pb.h"
#include <map>
#include <iostream>


namespace jetstream {

struct operator_id_t {
  int32_t computation_id; // which computation
  int32_t task_id;        // which operator in the computation

  bool operator< (const operator_id_t& rhs) const {
    return computation_id < rhs.computation_id 
      || task_id < rhs.task_id;
  }
  
  std::string to_string () {
    std::ostringstream buf;
    buf << "(" << computation_id << "," << task_id << ")";
    return buf.str();
  }
    
  operator_id_t (int32_t c, int32_t t) : computation_id (c), task_id (t) {}
  operator_id_t () : computation_id (0), task_id (0) {}
};

inline std::ostream& operator<<(std::ostream& out, operator_id_t id) {
  out << "(" << id.computation_id << "," << id.task_id << ")";
  return out;
}

class TupleReceiver {
 public:
  virtual void process (boost::shared_ptr<Tuple> t) = 0;
  virtual ~TupleReceiver() {}
  virtual std::string as_string() = 0; //return a description
};

typedef std::map<std::string,std::string> operator_config_t;



class DataPlaneOperator : public TupleReceiver {
 private:
  operator_id_t operID; // TODO: when is this set???  -Ari
  boost::shared_ptr<TupleReceiver> dest;
  const static std::string my_type_name;
  int tuplesEmitted;

 protected:
  void emit (boost::shared_ptr<Tuple> t); // Passes the tuple along the chain
    
 public:
  DataPlaneOperator ():tuplesEmitted(0)  {}
  virtual ~DataPlaneOperator ();
  
  virtual const std::string& get_type() {return my_type_name;}
  
  virtual void process (boost::shared_ptr<Tuple> t); // NOT abstract here
  void set_dest (boost::shared_ptr<TupleReceiver> d) { dest = d; }
  boost::shared_ptr<TupleReceiver> get_dest () { return dest; }
  
  operator_id_t & id() {return operID;}
  std::string as_string() { return operID.to_string(); }
  int emitted_count() { return tuplesEmitted;}

  /** This method will be called on every operator, before start() and before
  * any tuples will be received. This method must not block or emit tuples
  */ 
  virtual void configure (std::map<std::string, std::string> &) {};


  /**
   * An operator must not start emitting tuples until start() has been called or
   * until it has received a tuple.
   * This function ought not block. If asynchronous processing is required (e.g.,
   * in a source operator, launch a thread to do this).
   * Special dispensation for test code.
   */
  virtual void start () {};


  /**
   * An operator should stop processing tuples before this returns.
   * This function must not block.
   */
  virtual void stop () {};
};

typedef DataPlaneOperator *maker_t();
}

#endif /* _dataplaneoperator_H_ */
