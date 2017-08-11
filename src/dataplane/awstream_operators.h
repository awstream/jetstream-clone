#ifndef __JetStream__awstream_operators__
#define __JetStream__awstream_operators__

#include "chain_ops.h"
namespace jetstream {

class VideoConfig {
 public:
  size_t width;
  size_t skip;
  size_t quant;
  bool operator<(const VideoConfig& src) const  {
    return (this->width < src.width) ||
      (this->skip < src.skip) ||
      (this->quant < src.quant);
  }
};

/**
An adaptive source that allows setting the output data level.
*/
class VideoSource: public TimerSource {
 public:
  VideoSource();
  virtual int emit_data();
  virtual operator_err_t configure(std::map<std::string, std::string> &config);

  virtual void set_congestion_policy(boost::shared_ptr<CongestionPolicy> p) {
    congest_policy = p;
  }

 private:
  size_t cur_level_;

  // levels keeps track of bandwidth consumption of each one
  std::vector<double> levels_;

  // profiles correspons to the video configuration to use
  std::vector<VideoConfig> profile_;

  size_t cur_frame_;
  size_t total_frame_;

  // an array whose index is the frame: each element maps the current
  // video config to a specific byte size
  std::vector<std::map<VideoConfig, size_t> > source_;

 protected:
  GENERIC_CLNAME
};
}
#endif  // __JetStream__awstream_operators__
