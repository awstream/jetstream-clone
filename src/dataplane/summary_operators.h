#ifndef JetStream_quantile_operators_h
#define JetStream_quantile_operators_h


#include "dataplaneoperator.h"

// #include <boost/thread/thread.hpp>


namespace jetstream {



class QuantileOperator: public DataPlaneOperator {
 public:
  QuantileOperator(): q(0.5) {}


  virtual void process(boost::shared_ptr<Tuple> t);
  virtual operator_err_t configure(std::map<std::string,std::string> &config);

 private:
  double q;
  unsigned field;

GENERIC_CLNAME
};  



}


#endif
