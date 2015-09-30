#!/usr/bin/env python
# -*- coding: utf-8 -*-

# System modules
import sys
import os
import shutil
import time
import re
import subprocess
import platform
from optparse import OptionParser
from math import sqrt

class extracted_data :
  def __init__(self, time, scale, loc, zone ) :
    self.time_ = time
    self.zone_ = zone
    self.loc_ = loc
    self.scale_ = scale
    
def get_frame_number(fname) :
  exp_elements = fname.split('_')
  exp_elements = exp_elements[-1].split('.')
  return float(exp_elements[0])

########################################
# Step 1: run tracker
########################################
def exif_tool( fname, field ) :
  args=["exiftool", fname, "-d", '%s', "-a", field, "-T"]

  logOut = open("temp.txt", "w")
  logErr = open("tracker_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  
  f = open("temp.txt", 'r')
  result = f.read()
  result = result.strip()
  f.close()
  return result
  
def extract(fname) :
  return extracted_data( exif_tool( fname, "-ModifyDate"), exif_tool( fname, "-PixelScale"),  exif_tool( fname, "-ModelTiePoint"), exif_tool( fname, "-ProjectedCSType") )

def main() :

  argParser = OptionParser(usage="%prog [options]")
  argParser.add_option("-f", dest="files",
                       help="(required) The input images" )
  argParser.add_option("-o", dest="output_file",
                       help="The output file",
                       default="out_seq.meta")

  options, args = argParser.parse_args()

  # Process command line options
  if options.files == None :
    argParser.print_help()
    return 1
  
  fin = open(options.files, "r")
  images = []
  while 1 :
    line = fin.readline()
    if not line :
      break
    line = line.strip()
    if line == "" :
      continue
    images.append(line)
    #print line
  fin.close()
  
  #print images[0]
  exp_elements = images[0].split('_')
  #print exp_elements[-1]
  exp_elements = exp_elements[-1].split('.')
  #print exp_elements[0]
  #print float(exp_elements[0])
  #print images[-1]
  frame_rate = (get_frame_number(images[-1])-get_frame_number(images[0])+1)/(float(exif_tool( images[-1], "-ModifyDate")) - float(exif_tool( images[0], "-ModifyDate")));
  #print frame_rate
  
  current_time = float(exif_tool( images[0], "-ModifyDate"))
  
  frame_rate_error = []
  
  for i in images :
    ith_time = float(exif_tool( i, "-ModifyDate"))
    error = current_time - ith_time
    #print get_frame_number(i), error
    frame_rate_error.append(error)
    current_time += 1.0/frame_rate
  n = len(frame_rate_error)
  mean = sum(frame_rate_error)/n
  print mean
  std = sqrt(sum((x-mean)**2 for x in frame_rate_error) / n)
  print std
  
  meta = extract(images[0])
  fout = open(options.output_file, "w")
  fout.write( "start_time:  " + meta.time_ + "\n" )
  fout.write( "fps:         " + str(frame_rate) + "\n" )
  fout.write( "num_frames:  " + str(get_frame_number(images[-1])-get_frame_number(images[0])+1) + "\n" )
  fout.write( "frame_range: " + str(get_frame_number(images[0])) + " " + str(get_frame_number(images[-1])) + "\n" )
  fout.write( "zone:        " + meta.zone_ + "\n" )
  fout.write( "tie:         " + meta.loc_ + "\n" )
  fout.write( "gsd_scale:   " + meta.scale_+ "\n" )
  fout.write( "error_range: " + str(min(frame_rate_error)) + " " + str(max(frame_rate_error)) + "\n" )
  fout.write( "error_mean:  " + str(mean) + "\n")
  fout.write( "error_stdev: " + str(std) + "\n")

  return 0

if __name__ == "__main__" :
  sys.exit(main())

