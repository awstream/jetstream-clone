import threading

import BaseHTTPServer

SERV_PORT = 8001
def start_web_interface(js_server):
  web_interface = JSHttpServer(js_server)
  iface_thread = threading.Thread(group = None, target = web_interface.serve_forever, args = ())
  iface_thread.daemon = True
  iface_thread.start()

class JSHttpServer(BaseHTTPServer.HTTPServer):
  def __init__(self, js_server):
    self.js_server = js_server
    self.allow_reuse_address = True
    BaseHTTPServer.HTTPServer.__init__(self, ('', SERV_PORT), JSWebInterface)

class  JSWebInterface(BaseHTTPServer.BaseHTTPRequestHandler):
  """A simple HTTP status page for the server"""
      
  def do_GET(self):
    print "received GET"
    self.send_response(200)
    self.send_header("Content-type", "text/html")
    self.end_headers()
    self.wfile.write("<html><head><title>JetStream Status.</title></head>")
    self.wfile.write("<body>")
    self.print_node_list()
    self.wfile.write("</body></html>")
    
    
  def print_node_list(self):
    nodes = self.server.js_server.get_nodes()
    self.wfile.write("<p>Total of %d nodes. <ul>" % len(nodes))

    for n in nodes:
      self.wfile.write("<li>%s</li>" % str(n))
    self.wfile.write("</ul>")
