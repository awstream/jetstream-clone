#include <stdlib.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time.hpp>
#include <glog/logging.h>

#include "node.h"
#include "dataplane_comm.h"
#include "jetstream_types.pb.h"


using namespace jetstream;
using namespace std;
using namespace boost;

Node::Node (const NodeConfig &conf, boost::system::error_code &error)
  : config (conf),
    iosrv (new asio::io_service()),
    connMgr (new ConnectionManager(iosrv)),
    livenessMgr (iosrv, conf.heartbeat_time),
    webInterface (conf.webinterface_port, *this),

    // XXX This should get set through config files
    operator_loader ("src/dataplane/") //NOTE: path must end in a slash
{
  LOG(INFO) << "creating node" << endl;

  // Set up the network connection
  asio::io_service::work work(*iosrv);

  if (conf.heartbeat_time > 0) {
    // XXX Can't dynamically modify heartbeat_time. Pass pointer to config?
    for (u_int i=0; i < config.controllers.size(); i++) {

      LOG(INFO) << "possible controller: " << config.controllers[i].first 
		<< ":" << config.controllers[i].second << endl;

      pair<string, port_t> cntrl = config.controllers[i];
      connMgr->create_connection(cntrl.first, cntrl.second,
				 bind(&Node::controller_connected, 
				      this, _1, _2));
    }
  }

  // Setup incoming connection listener
  asio::ip::tcp::endpoint listen_port (asio::ip::tcp::v4(), 
				       config.dataplane_myport);

  listeningSock = shared_ptr<ServerConnection>
    (new ServerConnection(iosrv, listen_port, error));

  if (error) {
    LOG(ERROR) << "Node: Error creating server socket: " << error.message() << endl;
    return;
  }

  listeningSock->accept(bind(&Node::incoming_conn_handler, this, _1, _2), error);

  if (error) {
    LOG(ERROR) << "Node: Error accepting server socket: " << error.message() << endl;
    return;
  }
}


Node::~Node () 
{
  if (!iosrv->stopped()) {
    stop();
  }
}



/***
* Run the node. This method starts several threads and does not return until
* the node process is terminated.
*/
void
Node::run ()
{
  for (u_int i=0; i < config.thread_pool_size; i++) {
    shared_ptr<thread> t (new thread(bind(&asio::io_service::run, iosrv)));
    threads.push_back(t);
  }
  
  webInterface.start();

  // Wait for all threads in pool to exit
  for (u_int i=0; i < threads.size(); i++)
    threads[i]->join();

  iosrv->run();

  VLOG(1) << "Finished node::run" << endl;
}


void
Node::stop ()
{
  livenessMgr.stop_all_notifications();
  iosrv->stop();
  
  std::map<operator_id_t, shared_ptr<DataPlaneOperator> >::iterator iter;

    //need to stop operators before deconstructing because otherwise they may
    //keep pointers around after destruction.
  for (iter = operators.begin(); iter != operators.end(); iter++) {
    iter->second->stop();
  }
  //  LOG(INFO) << "io service stopped" << endl;

  // Optional:  Delete all global objects allocated by libprotobuf.
  // Probably unwise here since we may have multiple Nodes in a unit test.
  //  google::protobuf::ShutdownProtobufLibrary();

  LOG(INFO) << "Finished node::stop" << endl;
}


void
Node::controller_connected (shared_ptr<ClientConnection> conn,
			    boost::system::error_code error)
{
  if (error) {
    LOG(WARNING) << "Node: Monitoring connection failed: " << error.message() << endl;
    return;
  }

  controllers.push_back(conn);

  LOG(INFO) << "Node: Connected to controller: " 
	    << conn->get_remote_endpoint() << endl;
  livenessMgr.start_notifications(conn);

  // Start listening on messages from controller
  boost::system::error_code e;
  conn->recv_control_msg(bind(&Node::received_ctrl_msg, this, conn,  _1, _2), e);
}


void
Node::received_ctrl_msg (shared_ptr<ClientConnection> c, 
			 const jetstream::ControlMessage &msg,
			 const boost::system::error_code &error)
{
 // VLOG(1) << "got message: " << msg.Utf8DebugString() <<endl;
  
  boost::system::error_code send_error;
  switch (msg.type ()) {
  case ControlMessage::ALTER:
    {
      const AlterTopo &alter = msg.alter();
      ControlMessage response = handle_alter(alter);
      c->send_msg(response, send_error);

      if (send_error != boost::system::errc::success) {
        LOG(WARNING) << "Node: failure sending response: " << send_error.message() << endl;
      }
        
      break;
    }

   default:
     break;
  }
  
}

void
Node::incoming_conn_handler (boost::shared_ptr<ConnectedSocket> sock, 
			     const boost::system::error_code &error)
{
  if (error) {
    if (!iosrv->stopped())
      LOG(WARNING) << "error receiving incoming connection: " << error.value()
		   << "(" << error.message() << ")" << endl;
    return;
  }
  boost::system::error_code e;
  
  // Need to convert the connected socket to a client_connection
  LOG(INFO) << "incoming dataplane connection received ok";
  
  boost::shared_ptr<ClientConnection> conn(new ClientConnection(sock) );
  conn->recv_data_msg(bind(&Node::received_data_msg, this, conn,  _1, _2), e);  

}


/**
 * This is only invoked for a "new" connection. We change the signal handler
 *  here to the appropriate handler object.
 */
void
Node::received_data_msg (shared_ptr<ClientConnection> c,
			 const DataplaneMessage &msg,
			 const boost::system::error_code &error)
{
  boost::system::error_code send_error;

  VLOG(1) << "node received data message of type " << msg.type();

  switch (msg.type ()) {
  case DataplaneMessage::CHAIN_CONNECT:
    {
      const Edge& e = msg.chain_link();
      //TODO can sanity-check that e is for us here.
      if (e.has_dest()) {
      
        operator_id_t dest_operator_id (e.computation(), e.dest());
        shared_ptr<DataPlaneOperator> dest = get_operator(dest_operator_id);
        
        LOG(INFO) << "Chain request for operator " << dest_operator_id.to_string();
        if (dest) {        // Operator exists so we can report "ready"
          // Note that it's important to put the connection into receive mode
          // before sending the READY.
         
          dataConnMgr.enable_connection(c, dest_operator_id, dest);

          //TODO do we log the error or ignore it?
        }
        else {
          LOG(INFO) << "Chain request for operator that isn't ready yet";
          dataConnMgr.pending_connection(c, dest_operator_id);
        }
      }
      else {
        LOG(WARNING) << "Got remote chain connect without a dest";
        DataplaneMessage response;
        response.set_type(DataplaneMessage::ERROR);
        response.mutable_error_msg()->set_msg("got connect with no dest");
        c->send_msg(response, send_error);

      }
    }
    break;
  default:
     // Everything else is an error
     LOG(WARNING) << "unexpected dataplane message: "<<msg.type() << 
        " from " << c->get_remote_endpoint();
        
     break;
  }
}

operator_id_t 
unparse_id (const TaskID& id)
{
  operator_id_t parsed;
  parsed.computation_id = id.computationid ();
  parsed.task_id = id.task();
  return parsed;
}


ControlMessage
Node::handle_alter (const AlterTopo& topo)
{
  map<operator_id_t, map<string, string> > operator_configs;
  for (int i=0; i < topo.tostart_size(); ++i) {
    const TaskMeta &task = topo.tostart(i);
    operator_id_t id = unparse_id(task.id());
    const string &cmd = task.op_typename();
    map<string,string> config;
    for (int j=0; j < task.config_size(); ++j) {
      const TaskMeta_DictEntry &cfg_param = task.config(j);
      config[cfg_param.opt_name()] = cfg_param.val();
    }
    operator_configs[id] = config;
    create_operator(cmd, id);
     //TODO: what if this returns a null pointer, indicating create failed?
  }
  
  // Create cubes
  for (int i=0; i < topo.tocreate_size(); ++i) {
    const CubeMeta &task = topo.tocreate(i);
    cubeMgr.create_cube(task.name(), task.schema());
  }
  
  //TODO remove cubes and operators if specified.
  
  // add edges
  for (int i=0; i < topo.edges_size(); ++i) {
    const Edge &edge = topo.edges(i);
    operator_id_t src (edge.computation(), edge.src());
    shared_ptr<DataPlaneOperator> srcOperator = get_operator(src);
    
    assert(srcOperator);

    //TODO check if src doesn't exist
    if (edge.has_cube_name()) { 
      // connect to local table
      shared_ptr<DataCube> c = cubeMgr.get_cube(edge.cube_name());
      if (c)
        srcOperator->set_dest(c);
      else {
        LOG(WARNING) << "DataCube unknown: " << edge.cube_name() << endl;
      }
    } 
    else if (edge.has_dest_addr()) {   //remote network operator
      shared_ptr<TupleReceiver> xceiver(
          new OutgoingConnAdaptor(*connMgr, edge) );
      srcOperator->set_dest(xceiver);
    } 
    else {
      assert(edge.has_dest());
      operator_id_t dest (edge.computation(), edge.dest());
      shared_ptr<DataPlaneOperator> destOperator = get_operator(dest);
      if (destOperator)
        srcOperator->set_dest(destOperator);
      else {
        LOG(WARNING) << "Dest Operator unknown: " << dest.to_string() << endl;
      }
    }
  }
  
    //now start the operators
  map<operator_id_t, map<string,string> >::iterator iter;
  for (iter = operator_configs.begin(); iter != operator_configs.end(); iter++) {
    shared_ptr<DataPlaneOperator> op = get_operator(iter->first);
    op->start(iter->second);
  }
  
  ControlMessage msg;
  msg.set_type(ControlMessage::OK);
  return msg;
}


shared_ptr<DataPlaneOperator>
Node::create_operator (string op_typename, operator_id_t name) 
{
  shared_ptr<DataPlaneOperator> d (operator_loader.newOp(op_typename));

   //TODO logging
  /*
  if (d.get() != NULL) {
    cout << "creating operator of type " << op_typename <<endl;
  else 
  {
    cout <<" failed to create operator. Type was "<<op_typename <<endl;
  } */
//  d->operID = name.task_id;
  operators[name] = d;
  return d;
}


