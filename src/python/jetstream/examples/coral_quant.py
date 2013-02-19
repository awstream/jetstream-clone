

from collections import defaultdict
import csv
from optparse import OptionParser 
import random
import sys
import time

from jetstream_types_pb2 import *
from remote_controller import *
import query_graph as jsapi
from query_planner import QueryPlanner

from coral_parse import coral_fnames,coral_fidxs, coral_types

def main():

  parser = OptionParser()

  parser.add_option("-a", "--controller", dest="controller",
                  help="controller address", default="localhost:3456")
  parser.add_option("-d", "--dry-run", dest="DRY_RUN", action="store_true", 
                  help="shows PB without running", default=False)
  parser.add_option("-r", "--rate", dest="rate",help="the rate to use per source (instead of rate schedule)")
  parser.add_option("-u", "--union_root_node", dest="root_node",help="address of union/aggregator node")
  parser.add_option("-f", "--file_name", dest="fname",help="name of input file")

  parser.add_option("-g", "--generate-at-union", dest="generate_at_union", action="store_false",help="generate data at union node", default=True)
  parser.add_option("-l", "--latency_log_file", dest="latencylog", 
  default="latencies.out", help="file to log latency into")
  parser.add_option("--start-time", dest="start_ts", 
  default="0", help="unix timestamp to start simulation at")
  parser.add_option("--timewarp", dest="warp_factor", 
  default="1", help="simulation speedup")

  (options, args) = parser.parse_args()

  if not options.fname:
    print "you must specify the input file name [with -f]"
    sys.exit(1)

  if options.DRY_RUN:
    id = NodeID()
    id.address ="somehost"
    id.portno = 12345
    all_nodes = [id]
  else:    
    serv_addr, serv_port = normalize_controller_addr(options.controller)
    server = RemoteController()
    server.connect(serv_addr, serv_port)
    all_nodes = server.all_nodes()


  root_node = find_root_node(options, all_nodes)

  g = get_graph(all_nodes, root_node,  options)

  req = g.get_deploy_pb()
  if options.DRY_RUN:
    planner = QueryPlanner( {("somehost", 12345): ("somehost", 12346) } )
    planner.take_raw_topo(req.alter)
    planner.get_assignments(1)
    print req
  else:
   server.deploy_pb(req)
    

def find_root_node(options, all_nodes):
  if options.root_node:
    found = False
    for node in all_nodes:
      if node.address == options.root_node:
        root_node = node
        found = True 
        break
    if not found:
      print "Node with address: ",options.root_node," not found for use as the aggregator node"
      sys.exit()
  else:
    root_node = all_nodes[0]  #TODO randomize
  return root_node


def define_cube(cube, ids = [0,1,2,3]):
  cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, ids[0])
  cube.add_dim("response_code", Element.INT32, ids[1])
  cube.add_agg("sizes", jsapi.Cube.AggType.HISTO, ids[2])
  cube.add_agg("latencies", jsapi.Cube.AggType.HISTO, ids[3])
#  cube.add_agg("count", jsapi.Cube.AggType.COUNT, ids[4])


def parse_ts(start_ts):
  if start_ts.isdigit():
    return int(start_ts)
  else:
    #todo could allow more formats here
    assert False

def get_graph(all_nodes, root_node, options):
  g= jsapi.QueryGraph()


  start_ts = parse_ts(options.start_ts)

  central_cube = g.add_cube("global_coral")
  central_cube.instantiate_on(root_node)
  define_cube(central_cube)
  pull_q = jsapi.TimeSubscriber(g, {}, 2000) #every two seconds
  pull_q.set_cfg("ts_field", 0)
  pull_q.set_cfg("start_ts", start_ts)
  pull_q.set_cfg("rollup_levels", "8,1")
  pull_q.set_cfg("simulation_rate", options.warp_factor)
  pull_q.set_cfg("window_offset", 5000) #but trailing by a few
  
  q_op = jsapi.Quantile(g, 0.95, 3)
  q_op2 = jsapi.Quantile(g, 0.95,2)
  echo = jsapi.Echo(g)
  
  g.chain([central_cube, pull_q, q_op, q_op2, echo] )
  
  
  latency_measure_op = jsapi.LatencyMeasureSubscriber(g, time_tuple_index=4, hostname_tuple_index=5, interval_ms=100);
      #use field 
  echo_op = jsapi.Echo(g);
  echo_op.set_cfg("file_out", options.latencylog)
  echo_op.instantiate_on(root_node)
  g.chain([central_cube, latency_measure_op, echo_op])

  parsed_field_offsets = [coral_fidxs['timestamp'], coral_fidxs['HTTP_stat'],\
      coral_fidxs['nbytes'], coral_fidxs['dl_utime'] ]
      
  for node, i in zip(all_nodes, range(0, len(all_nodes))):
    local_cube = g.add_cube("local_coral_%d" %i)
    define_cube(local_cube, parsed_field_offsets)
    print "cube output dimensions:", local_cube.get_output_dimensions()

    f = jsapi.FileRead(g, options.fname, skip_empty=True)
    csvp = jsapi.CSVParse(g, coral_types)
    round = jsapi.TRoundOperator(g, fld=1, round_to=1)
    to_summary1 = jsapi.ToSummary(g, field=parsed_field_offsets[2], size=100)
    to_summary2 = jsapi.ToSummary(g, field=parsed_field_offsets[3], size=100)
    g.chain( [f, csvp, round, to_summary1, to_summary2, local_cube] )
    
    f.instantiate_on(node)
    pull_from = jsapi.TimeSubscriber(g, {}, 2000) #every two seconds
    pull_from.instantiate_on(node)
    pull_from.set_cfg("simulation_rate", options.warp_factor)
    pull_from.set_cfg("ts_field", 0)
    pull_from.set_cfg("start_ts", start_ts)
    pull_from.set_cfg("window_offset", 2000) #but trailing by a few

    local_cube.instantiate_on(node)

    timestamp_op= jsapi.TimestampOperator(g, "ms") 
    count_extend_op = jsapi.ExtendOperator(g, "i", ["1"])
    count_extend_op.instantiate_on(node)
   
    g.chain([local_cube, pull_from,timestamp_op, count_extend_op, central_cube])
#  g.chain([local_cube, pull_q, q_op, q_op2, echo] )

    
  
  return g

if __name__ == '__main__':
    main()
    