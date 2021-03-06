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
from client_reader import ClientDataReader,tuple_str
from operator_schemas import OpType
from coral_parse import coral_fnames,coral_fidxs, coral_types
from coral_util import *   #find_root_node, standard_option_parser,
import regions

logger = logging.getLogger('JetStream')
logger.setLevel(logging.INFO)
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)

UNION = True
MULTI_LEVEL = False


#  NOTES
#  Raw cube uses source timestamps.
# We then apply a time-warp operator and all other cubes use warped timestamps.
# Should probably specify a start time. For our coral logs, --start-ts=1379306270

MODE_LIST = "quantiles, domains, urls, slow_reqs, bad_domains"

def main():

  parser = standard_option_parser()
  parser.add_option("--mode", dest="mode",
  action="store", help="query to run. Should be one of %s" % MODE_LIST)
  parser.add_option("--wait", dest="wait",
  action="store", help="how long to wait for results")
  (options, args) = parser.parse_args()
  
  if options.mode:
    mode = options.mode
    if len(args) > 0:
      print "Can't specify mode as both an arg and an option."
      sys.exit(0)
  else:
    if len(args) == 0:
      print "Must specify a mode. Should be one of %s" % MODE_LIST
      sys.exit(0)
    mode = args[0]
  
  if mode == "quantiles":
    define_internal_cube = quant_cube
    src_to_internal = src_to_quant
    process_results = process_quant
    final_rollup_levels = "8,1"
  elif mode == "urls":
    define_internal_cube = url_cube
    src_to_internal = src_to_url
    process_results = lambda x,y,z: y
    final_rollup_levels = "8,1,1" #rollup time slightly, rest is unrolled.
  elif mode == "domains":
    define_internal_cube = url_cube
    src_to_internal = src_to_domain
    process_results = lambda x,y,z: y
    final_rollup_levels = "8,1,1" #rollup time slightly, rest is unrolled.
  elif mode == "domains_all":
    define_internal_cube = dom_notime
    src_to_internal = drop_time_from_doms
    process_results = lambda x,y,z: y
    final_rollup_levels = "1,1" #rollup time slightly, rest is unrolled.    
  elif mode == "slow_reqs":
    define_internal_cube = url_cube
    src_to_internal = src_slow_reqs
    process_results = lambda x,y,z: y
    final_rollup_levels = "9,1,1" #nothing rolled up.
  elif mode == "bad_domains":
    define_internal_cube = bad_doms_cube
    src_to_internal = src_to_bad_doms
    process_results = bad_doms_postprocess
    final_rollup_levels = "8,1,1" #rollup time slightly, rest is unrolled.
  elif mode == "total_bw":
    define_internal_cube = bw_cube
    src_to_internal = src_to_bw
    process_results = lambda x,y,z: y
    final_rollup_levels = "8,1" #rollup time slightly, rest is unrolled.
  elif mode == "bad_referers":
    define_internal_cube = badreferrer_cube
    src_to_internal = src_to_badreferrer
    process_results = badreferrer_out
    final_rollup_levels = "8,1" #rollup time slightly, rest is unrolled.    
  else:
    print "Unknown mode %s" % mode
    sys.exit(0)

  all_nodes,server = get_all_nodes(options)
  if len(all_nodes) < 1:
    print "FATAL: no nodes"
    sys.exit(0)
  
  g= jsapi.QueryGraph()

  ops = []
  union_node = find_root_node(options, all_nodes)

  for node in all_nodes:
    if node == union_node and not options.generate_at_union:
      continue
    raw_cube = define_raw_cube(g, "local_records", node, overwrite=False)
    raw_cube_sub = jsapi.TimeSubscriber(g, {}, 1000)
    raw_cube_sub.set_cfg("simulation_rate", options.warp_factor)
    raw_cube_sub.set_cfg("ts_field", 0)
    if options.start_ts:
      raw_cube_sub.set_cfg("start_ts", options.start_ts)      
#    time_shift = jsapi.TimeWarp(g, field=0, warp=options.warp_factor)
    
    last_op = g.chain([raw_cube, raw_cube_sub]) #, time_shift]) 
    last_op = src_to_internal(g, last_op, node, options)
    last_op.instantiate_on(node)
    ops.append(last_op)
    
  if len(ops) == 0:
    print "can't run, no [non-union] nodes"
    sys.exit(0) 
    
  union_cube = define_internal_cube (g, "union_cube", union_node)

  g.agg_tree(ops, union_cube, start_ts =options.start_ts, sim_rate=options.warp_factor)

  if options.bw_cap:
    union_cube.set_inlink_bwcap(float(options.bw_cap))

      #This is the final output subscriber
  pull_q = jsapi.TimeSubscriber(g, {}, 1000) #only for UI purposes
  pull_q.set_cfg("ts_field", 0)
#  pull_q.set_cfg("latency_ts_field", 7)
  if options.start_ts:
    pull_q.set_cfg("start_ts", options.start_ts)
  pull_q.set_cfg("rollup_levels", final_rollup_levels)
  pull_q.set_cfg("simulation_rate", options.warp_factor)
  pull_q.set_cfg("window_offset", 8* 1000) #...trailing by a few

  g.connect(union_cube, pull_q)
  last_op = process_results(g, pull_q, options)  

  echo = jsapi.Echo(g)
  echo.instantiate_on(union_node)
  g.connect(last_op, echo)
    
  deploy_or_dummy(options, server, g)
  

def src_to_quant(g, raw_cube_sub, node, options):
  to_summary1 = jsapi.ToSummary(g, field=2, size=5000)
  to_summary2 = jsapi.ToSummary(g, field=3, size=5000)
  return g.chain([raw_cube_sub, jsapi.Project(g, 3), jsapi.Project(g, 2), to_summary1, to_summary2] )

def quant_cube(g, cube_name, cube_node):  
  cube = g.add_cube(cube_name)
  cube.instantiate_on(cube_node)
  cube.set_overwrite(True)
  cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, 0)
  cube.add_dim("response_code", Element.INT32, 1)
  cube.add_agg("sizes", jsapi.Cube.AggType.HISTO, 2)
  cube.add_agg("latencies", jsapi.Cube.AggType.HISTO, 3)
  cube.add_agg("count", jsapi.Cube.AggType.COUNT, 4)
  return cube

def process_quant(g, union_sub, options):
  count_op = jsapi.SummaryToCount(g, 2)
  q_op = jsapi.Quantile(g, 0.95, 3)
  q_op2 = jsapi.Quantile(g, 0.95,2)
  g.chain([union_sub, count_op, q_op, q_op2] )
  return q_op2

def url_cube(g, cube_name, cube_node):  
  return define_raw_cube(g, cube_name, cube_node, overwrite=True)


def src_to_url(g, data_src, node, options):
  return data_src

def src_to_domain(g, data_src, node, options):
  url2dom = jsapi.URLToDomain(g, 2)
  g.chain([data_src, jsapi.Project(g, 3), url2dom])
  return url2dom
  
def src_slow_reqs(g, data_src, node, options):
    #units are usec/byte, or sec/mb. So bound of 100 ==> < 10 kb/sec
  filter = jsapi.RatioFilter(g, numer=4, denom=3, bound = 100)
  return g.connect(data_src, filter)


def drop_time_from_doms(, data_src, node, options):
  return g.chain([raw_cube_sub, jsapi.Project(g, 4), jsapi.Project(g, 3), jsapi.Project(g, 1), jsapi.Project(g, 0)] )
  

def dom_notime(g, cube_name, cube_node):
  cube = g.add_cube(cube_name)
  cube.instantiate_on(cube_node)
  cube.set_overwrite(True)
  cube.add_dim("domain", CubeSchema.Dimension.STRING, 0)
  cube.add_agg("count", jsapi.Cube.AggType.COUNT, 1)
  return cube


def src_to_bw(g, data_src, node, options):
  hostname_extend_op = jsapi.ExtendOperator(g, "s", ["${HOSTNAME}"]) 
  return   g.chain([data_src, jsapi.Project(g, 5), jsapi.Project(g, 4), \
    jsapi.Project(g, 1), jsapi.Project(g, 1), hostname_extend_op])

def bw_cube(g, cube_name, cube_node):
  cube = g.add_cube(cube_name)
  cube.instantiate_on(cube_node)
  cube.set_overwrite(True)
  cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, 0)
  cube.add_agg("sizes", jsapi.Cube.AggType.COUNT, 1)  
  cube.add_dim("hostname", CubeSchema.Dimension.STRING, 2)
  return cube


def src_to_bad_doms(g, data_src, node, options):
  return g.chain([data_src, jsapi.Project(g, 5), jsapi.Project(g, 4), \
      jsapi.Project(g, 3), jsapi.URLToDomain(g, 2)])

def bad_doms_cube(g, cube_name, cube_node):
  cube = g.add_cube(cube_name)
  cube.instantiate_on(cube_node)
  cube.set_overwrite(True)
  cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, 0)
  cube.add_dim("response_code", CubeSchema.Dimension.INT32, 1)
  cube.add_dim("url", CubeSchema.Dimension.STRING, 2)
  cube.add_agg("count", jsapi.Cube.AggType.COUNT, 3)
  return cube

def bad_doms_postprocess(g, union_sub, options):
  ratio_cube = g.add_cube("global_badreq_ratios")
  ratio_cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, 0)
  ratio_cube.add_dim("response_code", CubeSchema.Dimension.INT32, 1)
  ratio_cube.add_dim("domain", CubeSchema.Dimension.STRING, 2)
  ratio_cube.add_agg("count", jsapi.Cube.AggType.COUNT, 3)  
  ratio_cube.add_agg("ratio", jsapi.Cube.AggType.MIN_D, 4)
  ratio_cube.set_overwrite(True)
  compute_ratio = jsapi.SeqToRatio(g, url_field = 2, total_field = 3, respcode_field = 1)
  pull_q = jsapi.TimeSubscriber(g, {}, 1000, num_results= 5, sort_order="-ratio")
  pull_q.set_cfg("ts_field", 0)
  pull_q.set_cfg("start_ts", options.start_ts)
#  pull_q.set_cfg("rollup_levels", "8,1,1")
  pull_q.set_cfg("simulation_rate", options.warp_factor)
  pull_q.set_cfg("window_offset", 5* 1000) #but trailing by a few
  return  g.chain( [union_sub, compute_ratio, ratio_cube, pull_q] )


def src_to_badreferrer(g, data_src, node, options):
  only404s = jsapi.EqualsFilter(g, field= 1, targ=404)
  return g.chain([data_src, only404s, jsapi.Project(g, 5), jsapi.Project(g, 4), \
      jsapi.Project(g, 2), jsapi.Project(g, 1), jsapi.URLToDomain(g, 1)])

def badreferrer_cube(g, cube_name, cube_node):
  cube = g.add_cube(cube_name)
  cube.instantiate_on(cube_node)
  cube.set_overwrite(True)
  cube.add_dim("time", CubeSchema.Dimension.TIME_CONTAINMENT, 0)
  cube.add_dim("referer", CubeSchema.Dimension.STRING, 1)
  cube.add_agg("count", jsapi.Cube.AggType.COUNT, 2)
  return cube

def badreferrer_out(g, union_sub, options):
  union_sub.set_cfg("sort_order", "-count")  
  return union_sub

if __name__ == '__main__':
    main()

