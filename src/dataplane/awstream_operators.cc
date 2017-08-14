#include "awstream_operators.h"

#include "node.h"
#include <algorithm>
#include <boost/filesystem/fstream.hpp>
#include <fstream>
#include <glog/logging.h>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

using namespace ::std;
using namespace boost;

namespace jetstream {

int skip_to_wait_time_in_ms(int skip) {
  return 33;
  // return (int)(1000.0 / 30.0 * (skip + 1.0));
}

class CSVRow {
 public:
  string const& operator[](size_t index) const {
    return m_data[index];
  }
  size_t size() const {
    return m_data.size();
  }
  void readNextRow(istream& str) {
    string         line;
    getline(str, line);

    stringstream   lineStream(line);
    string         cell;

    m_data.clear();
    while(getline(lineStream, cell, ',')) {
      m_data.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty()) {
      // If there was a trailing comma then add an empty element.
      m_data.push_back("");
    }
  }
 private:
  vector<string> m_data;
};

istream& operator>>(istream& str, CSVRow& data) {
  data.readNextRow(str);
  return str;
}   

ostream& operator<<(ostream &o, const VideoConfig& vc) {
  return o << vc.width
	   << ", " << vc.skip
	   << ", " << vc.quant;
}

VideoSource::VideoSource() : cur_frame_(0) {
}

// Depend on the current configuration, we emit an array of data with
// specific size and return the expected time for next data item.
int VideoSource::emit_data() {
  // If congestion policy set, try to adapt
  if (congest_policy) {
    int delta = congest_policy->get_step(id(), levels_.data(), levels_.size(), cur_level_);
    if (delta != 0) {
      cur_level_ += delta;
      LOG(INFO) << "VideoSource adjusting itself cur_level="<< cur_level_ << " delta=" << delta;
    }
  }

  VideoConfig vc = profile_[cur_level_];
  map<VideoConfig, size_t>::iterator it = source_[cur_frame_].find(vc);
  if (it == source_[cur_frame_].end()) {
    LOG(FATAL) << "Please check the source and profile matches!";
  }

  int len = it->second;

  LOG(INFO) << "Emitting " << cur_frame_ << " (frame) "
	    << len << " (size) "
	    << cur_level_ << " (level) with configuration " << vc;

  // Allocate buffer with size it->second and send it using the tuples
  char* data_buf = new char[len];
  int wait = skip_to_wait_time_in_ms(vc.skip);

  boost::shared_ptr<Tuple> t = boost::shared_ptr<Tuple>(new Tuple);
  extend_tuple(*t, "");
  t->mutable_e(0)->set_i_val(cur_frame_);
  Element *e = t->add_e();
  e->set_blob(data_buf, len);
  delete []data_buf;

  vector<boost::shared_ptr<Tuple> > buf;
  buf.push_back(t);

  // Prepare to send messages
  DataplaneMessage end_msg;
  end_msg.set_type(DataplaneMessage::END_OF_WINDOW);
  end_msg.set_window_length_ms(wait);
  chain->process(buf, end_msg);

  cur_frame_++;
  if (cur_frame_ >= total_frame_) {
    cur_frame_ = 0;
  }
  return wait;
}

operator_err_t VideoSource::configure(map<string, string> &config) {
  // First, we load a csv file that contains <bw, width, skip, quant, acc>
  // and use it to populate `levels` and `profiles`
  CSVRow row;
  VideoConfig vc;
  double bw;
  string profile_file = config["profile"];
  {
    ifstream file(profile_file.c_str());
    while (file >> row) {
      bw = atof(row[0].c_str());
      vc.width = atoi(row[1].c_str());
      vc.skip = atoi(row[2].c_str());
      vc.quant = atoi(row[3].c_str());
      profile_.push_back(vc);
      levels_.push_back(bw);
    }
    file.close();
  }

  cur_level_ = levels_.size() - 1;

  // Second, we load the source file
  // <width, skip, quant, frame_no, bytes>
  string source_file = config["source"];
  total_frame_ = atoi(config["total_frame"].c_str());

  for (size_t i = 0; i < total_frame_; i++) {
    map<VideoConfig, size_t> m;
    source_.push_back(m);
  }

  {
    ifstream file(source_file.c_str());
    unsigned bytes;
    while (file >> row) {
      vc.width = atoi(row[0].c_str());
      vc.skip = atoi(row[1].c_str());
      vc.quant = atoi(row[2].c_str());
      int i = atoi(row[3].c_str());
      bytes = atoi(row[4].c_str());

      // LOG(INFO) << "inserting into " << i - 1 << " with size " << bytes << " and " << vc ;
      source_[i - 1].insert(make_pair(vc, bytes));
    }
  }

  cur_frame_ = 0;
  return NO_ERR;
}
const string VideoSource::my_type_name("Video source");

}
