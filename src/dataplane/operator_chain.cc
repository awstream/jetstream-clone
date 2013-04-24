//
//  operator_chain.cpp
//  JetStream
//
//  Created by Ariel Rabkin on 4/5/13.
//  Copyright (c) 2013 Ariel Rabkin. All rights reserved.
//

#include <glog/logging.h>
#include "operator_chain.h"
#include "js_utils.h"

using namespace ::std;

namespace jetstream {

const string&
OperatorChain::chain_name() {

  if (cached_chain_name.size() == 0) {
    ostringstream buf;
//    for (int i= 0; i < ops.size(); ++i)
    if (ops[0])
      buf << "chain starting at " << ops[0]->id_as_str();
    else
      buf << "Empty chain";
    
    cached_chain_name = buf.str();
  }
  return cached_chain_name;
}

void
OperatorChain::start() {

  running = true;
  if (ops.size() > 0 && ops[0]) {
    boost::shared_ptr<COperator> first_op = boost::dynamic_pointer_cast<COperator>(ops[0]);
    LOG_IF(FATAL,!first_op)<< "chain can't start if head op is " << ops[0]->id_as_str();
      
    LOG(INFO) << "Starting head-of-chain; " << first_op->id_as_str();
    first_op->start();
  }
  
//  for (int i = 1; i < ops.size(); ++i) {
//    ops[i]->start();
//  }
}



void
OperatorChain::stop() {
  LOG(INFO) << "Stopping chain.";
  // << typename_as_str() << " operator " << id() << ". Running is " << running;
  if (running) {
    running = false;
    boost::barrier b(1);

    stop_async( boost::bind(&boost::barrier::wait, &b) );
    b.wait();
    
  }
}

void
OperatorChain::stop_async(close_cb_t cb) {
  if (!strand) {
    LOG(WARNING) << "Can't stop, chain was never started";
  } else
    strand->wrap(boost::bind(&OperatorChain::do_stop, this, cb));
}

void
OperatorChain::do_stop(close_cb_t cb) {
/*
  if (ops.size() > 0 && ops[0])
    ops[0]->stop();

  for (int i = 0; i < ops.size(); ++i) {
    ops[i]->stop();
  }*/
  LOG(INFO) << " called stop everywhere; invoking cb";
  cb();
}



void
OperatorChain::process(std::vector<boost::shared_ptr<Tuple> > & data_buf, DataplaneMessage& maybe_meta) {

   for (int i = 1; i < ops.size(); ++i) {
     ChainMember * op = ops[i].get();
     op->process(this, data_buf, maybe_meta);
   }
}

void
OperatorChain::clone_from(boost::shared_ptr<OperatorChain> source) {
  for (int i = 0; i < source->ops.size(); ++i) {
    ops.push_back(source->ops[i]);
  }
}

}