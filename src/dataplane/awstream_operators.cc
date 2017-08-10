#include "awstream_operators.h"

#include <glog/logging.h>
#include <boost/filesystem/fstream.hpp>
#include "node.h"
#include <algorithm>
#include <numeric>

using namespace ::std;
using namespace boost;

namespace jetstream {

AwsSource::AwsSource() {
}

int AwsSource::emit_data() {
  return 10;
}

operator_err_t AwsSource::configure(std::map<std::string, std::string> &config) {
  return NO_ERR;
}
const string AwsSource::my_type_name("AWStream source");

}
