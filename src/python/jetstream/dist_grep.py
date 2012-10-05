#
# Graph data structure and algorithms for representing JetStream queries/computations.
#

from jetstream_types_pb2 import *

from optparse import OptionParser 

import random
import socket
import time


from remote_controller import RemoteController
import operator_graph as jsapi
from jetstream_types_pb2 import *


def main():

  parser = OptionParser()
  parser.add_option("-C", "--config", dest="config_file",
                  help="read config from FILE", metavar="FILE")

  (options, args) = parser.parse_args()
  pattern = ".*" + args[0] + ".*"
  file_to_grep = args[1]

  serv_addr = "localhost"
  serv_port = 3456
  
  ### Define the graph abstractly, without a computation
  g = jsapi.OperatorGraph()
  reader = jsapi.FileRead(g, file_to_grep)
  grepper = jsapi.StringGrep(g, pattern)
  host_extend = jsapi.ExtendOperator(g, "s", ["${HOSTNAME}"]
  
  cube = g.cube("local_results")
  cube.add_dim("log_line", Element.STRING)
  cube.add_dim("hostname", Element.STRING)
  cube.add_agg("count", jsapi.Cube.COUNT)
  cube.set_overwrite(True)  #fresh results

  g.connect(reader,grepper)
  g.connect(grepper, host_extend)
  
  //TODO should do a clone here.
  
  g.connect(host_extend, cube)
  
  #### Finished building in memory, now to join
  server = RemoteController()
  server.connect(serv_addr, serv_port)
  n = server.get_a_node()
  assert isinstance(n, NodeID)
  nodes = server.all_nodes()
  
  cube.instantiate_on(n)

  server.deploy(g)
    


if __name__ == '__main__':
    main()
