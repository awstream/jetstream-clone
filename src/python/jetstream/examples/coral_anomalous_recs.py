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
from coral_util import *   #find_root_node, standard_option_parser,

def main():

  parser = standard_option_parser()

  (options, args) = parser.parse_args()

  if not options.fname:
    print "you must specify the input file name [with -f]"
    sys.exit(1)

  all_nodes,server = get_all_nodes(options)
  root_node = find_root_node(options, all_nodes)
  source_nodes = get_source_nodes(options, all_nodes, root_node)

  g = get_graph(source_nodes, root_node,  options)
  deploy_or_dummy(options, server, g)


def define_quant_cube(cube, ids = [0,1]):
  cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, ids[0])
  cube.add_agg("sizes", jsapi.Cube.AggType.HISTO, ids[1])
  cube.set_overwrite(True)


def get_graph(source_nodes, root_node, options):
  g= jsapi.QueryGraph()

  start_ts = parse_ts(options.start_ts)

  central_cube = g.add_cube("global_coral_quant")
  central_cube.instantiate_on(root_node)
  define_quant_cube(central_cube)

  pull_q = jsapi.TimeSubscriber(g, {}, 1000)
  pull_q.set_cfg("ts_field", 0)
  pull_q.set_cfg("start_ts", start_ts)
#    pull_q.set_cfg("rollup_levels", "8,1")
#    pull_q.set_cfg("simulation_rate",1)
  pull_q.set_cfg("window_offset", 6* 1000) #but trailing by a few

  q_op = jsapi.Quantile(g, 0.95,field=1)

  g.chain([central_cube, pull_q, q_op] )


  thresh_cube = g.add_cube("coral_thresh")
  thresh_cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, ids[0])
  thresh_cube.add_agg("thresh", jsapi.Cube.AggType.INT, ids[1])
  thresh_cube.set_overwrite(True)

  if ECHO_RESULTS:
    echo = jsapi.Echo(g)
    echo.instantiate_on(root_node)
    g.chain( [q_op, echo, thresh_cube] )
  else:
    g.chain(  [q_op, thresh_cube] )  


#   latency_measure_op = jsapi.LatencyMeasureSubscriber(g, time_tuple_index=4, hostname_tuple_index=5, interval_ms=100);
#   echo_op = jsapi.Echo(g);
#   echo_op.set_cfg("file_out", options.latencylog)
#   echo_op.instantiate_on(root_node)
#   g.chain([central_cube, latency_measure_op, echo_op])

  parsed_field_offsets = [coral_fidxs['timestamp'], coral_fidxs['HTTP_stat'],\
      coral_fidxs['nbytes'], coral_fidxs['dl_utime'], len(coral_types) ]

  for node, i in numbered(source_nodes, not LOADING):
  
################ First do the data loading part
    local_q_cube = g.add_cube("local_coral_quant_%d" %i)
    define_quant_cube(local_cube, [coral_fidxs['timestamp'],  coral_fidxs['nbytes'] ])

    local_raw_cube = g.add_cube("local_coral_all_%d" % i)
    define_schema_for_raw_cube(local_raw_cube)

    f = jsapi.FileRead(g, options.fname, skip_empty=True)
    csvp = jsapi.CSVParse(g, coral_types)
    csvp.set_cfg("discard_off_size", "true")
    round = jsapi.TimeWarp(g, field=1, warp=options.warp_factor)
    f.instantiate_on(node)
    
    
    to_summary = jsapi.ToSummary(g, field='?', size=100)
    g.chain( [f, csvp, round, local_raw_cube, pass_raw,to_summary,local_q_cube] 
  
    pull_from_local = jsapi.TimeSubscriber(g, {}, 1000)      
    pull_from_local.instantiate_on(node)
    pull_from_local.set_cfg("simulation_rate", 1)
    pull_from_local.set_cfg("ts_field", 0)
    pull_from_local.set_cfg("start_ts", start_ts)
    pull_from_local.set_cfg("window_offset", 2000) #but trailing by a few

    local_q_cube.instantiate_on(node)

    g.chain([local_q_cube, pull_from_local, central_cube])

################ Now do the second phase  
#   Pass-thru at thresh_cube
#   Filtering Pass-thru at local_raw_cube
#   link from a to b

  return g

if __name__ == '__main__':
    main()

