

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

desc = """----
This script stores a histogram for all-nodes bandwidth on the root node
----"""
def main():
  print desc

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


def define_cube(cube, ids = [0,1,2,3,4]):
  cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, ids[0])
  cube.add_agg("sizes", jsapi.Cube.AggType.HISTO, ids[1])

  cube.set_overwrite(True)


def get_graph(source_nodes, root_node, options):
  g= jsapi.QueryGraph()

  ANALYZE = not options.load_only
  LOADING = not options.analyze_only
  ECHO_RESULTS = not options.no_echo

  if not LOADING and not ANALYZE:
    print "can't do neither load nor analysis"
    sys.exit(0)

  start_ts = parse_ts(options.start_ts)

  central_cube = g.add_cube("global_coral_bw")
  central_cube.instantiate_on(root_node)
  define_cube(central_cube)

  if ECHO_RESULTS:
    pull_q = jsapi.TimeSubscriber(g, {}, 1000) #every two seconds
    pull_q.set_cfg("ts_field", 0)
    pull_q.set_cfg("start_ts", start_ts)
#    pull_q.set_cfg("rollup_levels", "8,1")
    pull_q.set_cfg("simulation_rate",1)
    pull_q.set_cfg("window_offset", 4* 1000) #but trailing by a few
  
    q_op = jsapi.Quantile(g, 0.95, 1)
    echo = jsapi.Echo(g)
    echo.instantiate_on(root_node)
  
    g.chain([central_cube, pull_q, q_op, echo] )

  congest_logger = jsapi.AvgCongestLogger(g)
  congest_logger.instantiate_on(root_node)
  g.connect(congest_logger, central_cube)

  parsed_field_offsets = [coral_fidxs['timestamp'], coral_fidxs['nbytes'] ]

  for node, i in numbered(source_nodes, not LOADING):
    local_cube = g.add_cube("local_coral_quant_%d" %i)
    define_cube(local_cube, parsed_field_offsets)
    print "cube output dimensions:", local_cube.get_output_dimensions()

    if LOADING:
      f = jsapi.FileRead(g, options.fname, skip_empty=True)
      csvp = jsapi.CSVParse(g, coral_types)
      csvp.set_cfg("discard_off_size", "true")
      round = jsapi.TimeWarp(g, field=1, warp=options.warp_factor)
      to_summary1 = jsapi.ToSummary(g, field=parsed_field_offsets[1], size=100)
      g.chain( [f, csvp, round, to_summary1, local_cube] )
      f.instantiate_on(node)
    else:
       local_cube.set_overwrite(False)
  
    query_rate = 1000 if ANALYZE else 3600 * 1000
    pull_from_local = jsapi.TimeSubscriber(g, {}, query_rate)
      
    pull_from_local.instantiate_on(node)
    pull_from_local.set_cfg("simulation_rate", 1)
    pull_from_local.set_cfg("ts_field", 0)
    pull_from_local.set_cfg("start_ts", start_ts)
    pull_from_local.set_cfg("window_offset", 2000) #but trailing by a few
#    pull_from_local.set_cfg("rollup_levels", "8,1")
#    pull_from_local.set_cfg("window_size", "5000")

    local_cube.instantiate_on(node)

    g.chain([local_cube, pull_from_local, congest_logger])

  return g

if __name__ == '__main__':
    main()

