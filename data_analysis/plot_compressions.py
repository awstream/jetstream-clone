from collections import defaultdict
import re
import sys
#import numpy
#import numpy.linalg
#from numpy import array
#from math import sqrt



OUT_TO_FILE = True
OUT_FILE = "key_stats.pdf" 
import matplotlib
if OUT_TO_FILE:
    matplotlib.use('Agg')
import matplotlib.pyplot as plt
#import numpy as np

#    matplotlib.rcParams['ps.useafm'] = True
matplotlib.rcParams['pdf.use14corefonts'] = True


def main():
  infile = sys.argv[1]
  data = parse_data(infile)
  plot_compressions(data)
  print "Done. Output in",OUT_FILE
  
INTERVAL_NAMES = {5: "5s", 60:"minute", 300:"5 m", 3600:"hour", 86400:"day"}

def plot_compressions(data):

  colors = ["#5555FF", "#aa0000"]
  t_labels, url_series, dom_series = [], [], []
  for t, (u,d) in sorted(data.items()):
    url_series.append(u)
    dom_series.append(d)
    t_labels.append( INTERVAL_NAMES[t])
  
  fig = plt.figure(figsize=(6.25,3.5))
  fig.subplots_adjust(bottom=0.13)
  fig.subplots_adjust(top=0.99)
  fig.subplots_adjust(left=0.1)
  fig.subplots_adjust(right=0.98)
  
  ax = fig.add_subplot(111)
  ax.set_yscale('log',basey=2)
  ticks = [2 ** x for x in range(9)]
  plt.yticks(ticks, ticks)
  
  plt.ylim((1, 500))
      
  offset = 0.5
  for series,series_name,c in zip([dom_series, url_series], ["Domains", "URLs"], colors):
    positions = [x * 1.2 + offset for x in range(len(data))]
    plt.bar(positions, series, width = 0.5, color = c, label=series_name, bottom=1) #log=True)
    offset += 0.5
  plt.xticks(positions, t_labels, fontsize = 10, rotation = 0)
  plt.xlabel("Aggregation time period")
  plt.ylabel("Compression factor")

  
  ax.legend(frameon=False, loc = "upper left")
  if OUT_TO_FILE:
    plt.savefig(OUT_FILE)
    plt.close(fig)  

#  print data

def parse_data(infile):
  f = open(infile, 'r')
  data = {}
  for ln in f:
    t, url,dom = ln.strip().split()
    t,url, dom = int(t), float(url), float(dom)
    data[t] = (url,dom)
  f.close()
  return data
  
  
  
  

if __name__ == '__main__':
  main()
