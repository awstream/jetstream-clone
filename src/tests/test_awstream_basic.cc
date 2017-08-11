/**
 *  Tests AWStream operators, VideoSource
 */

#include "awstream_operators.h"
#include "base_operators.h"
#include "chain_ops.h"
#include "experiment_operators.h"
#include "node.h"
#include "queue_congestion_mon.h"
#include <boost/date_time.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/regex.hpp>
#include <boost/thread/thread.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <map>

using namespace jetstream;
using boost::shared_ptr;
using std::string;
using std::cout;
using std::endl;
using std::map;
using std::vector;

class COperatorTest : public ::testing::Test {
 public:
  boost::shared_ptr<Node> node;
  virtual void SetUp() {
    if (!node) {
      NodeConfig cfg;
      cfg.heartbeat_time = 2000;
      boost::system::error_code err;
      node = boost::shared_ptr<Node>(new Node(cfg, err));
    }
    node->start();
  }
  
  virtual void TearDown() {
    node->stop();
  }
};


TEST_F(COperatorTest, VideoSourceConfigure) {
  boost::shared_ptr<OperatorChain> chain(new OperatorChain);

  boost::shared_ptr<VideoSource> source(new VideoSource);
  map<string,string> config;
  config["source"] =  "/tmp/jetstream/darknet.source.csv";
  config["profile"] =  "/tmp/jetstream/darknet.profile.csv";
  config["total_frame"] =  "1800";
  source->configure(config);
  source->set_node(node.get());
  source->add_chain(chain);

  boost::shared_ptr<CongestionPolicy> policy(new CongestionPolicy);
  boost::shared_ptr<QueueCongestionMonitor> mockCongest(new QueueCongestionMonitor(256, "dummy"));
  mockCongest->set_downstream_congestion(1.0, get_msec());
  policy->set_congest_monitor(mockCongest);
  policy->add_operator(source->id());
  source->set_congestion_policy(policy);

  boost::shared_ptr<DummyReceiver> recv(new DummyReceiver);
  chain->add_member(source);
  chain->add_member(recv);
  chain->start();

  boost::this_thread::sleep(boost::posix_time::milliseconds(200));
  mockCongest->set_downstream_congestion(0.5, get_msec());
  boost::this_thread::sleep(boost::posix_time::milliseconds(200));

  chain->stop();
}
