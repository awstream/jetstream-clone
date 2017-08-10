#ifndef __JetStream__awstream_operators__
#define __JetStream__awstream_operators__

#include "chain_ops.h"
namespace jetstream {
/**
An adaptive source that allows setting the output data level.
*/
class AwsSource: public TimerSource {
 public:
  AwsSource();
  virtual int emit_data();
  virtual operator_err_t configure(std::map<std::string, std::string> &config);

  virtual void set_congestion_policy(boost::shared_ptr<CongestionPolicy> p) {
    congest_policy = p;
  }

 protected:
  GENERIC_CLNAME
};
}
#endif  // __JetStream__awstream_operators__
