#!/usr/bin/env python
# -*- coding: utf-8 -*-
#ckwg +4
# Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
# KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
# Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.


# System modules
import sys
import os
import shutil
import time
import re
import subprocess
import platform
from optparse import OptionParser

# Config file manipulation
from ConfigBlock import *

########################################
# Step 1: run tracker
########################################
def RunTracker( config_file ) :
  args=["detect_and_track", "-c", config_file]

  logOut = open("tracker_out.txt", "w")
  logErr = open("tracker_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 1.5:Create Example Set
########################################
def CreateExampleSet( gt, gsd ) :
  args=["create_example_set", "--ground-truth-tracks", gt, "--gsd", gsd, "--computed-tracks", "tracks_unfiltered.kw18"]

  logOut = open("CreateExampleSet_out.txt", "w")
  logErr = open("CreateExampleSet_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 1.6: create_positive_and_negative_set
########################################
def CreatePositiveAndNegativeSet( gsd ) :
  args=["create_positive_and_negative_set", "--input-tracks", "tracks_unfiltered.kw18", "--output-track-prefix", "training", "--labeled-set", "trainset.txt", "--mmdfe", "5", "--gsd", gsd, "--mtl", "5"]

  logOut = open("CreatePositiveAndNegativeSet_out.txt", "w")
  logErr = open("CreatePositiveAndNegativeSet_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 1.7: draw_tracks_on_video
########################################
def DrawVideosPVO( input_video ) :
  f = open("example_tracks.txt", "w")
  f.write( "training_pos.kw18 0 0 0 1\n" )
  f.write( "training_neg.kw18 0 0 0 1\n" )
  f.close()
  args=["draw_tracks_on_video", "--track-info-file", "example_tracks.txt", "--src-video", input_video, "--out-video", "training_set.avi"]

  logOut = open("draw_out.txt", "w")
  logErr = open("draw_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0


########################################
# Recurse through the config block
########################################
def TraverseConfig(block, prefix, values) :
  for k in block.Values :
    values.append(prefix + k + " = " + block.Values[k])

  values.append("")

  for sub in block.SubBlocks :
    TraverseConfig(block.SubBlocks[sub],
                   prefix + block.SubBlocks[sub].Name + ":",
                   values)

  for b in block.Blocks :
    values.append("")
    values.append("block " + b)
    values.append("")
    TraverseConfig(block.Blocks[b], prefix, values)
    values.append("")
    values.append("endblock")
    values.append("")

def CreateTrainingSet(config_file, base_directory, experiment_name, output_file) :
  input_imgs = base_directory + "/frames.txt"
  f = open(base_directory + "/gsd.txt", 'r')
  gsd = f.read()
  gsd = gsd.strip()
  f.close()
  f = open(base_directory + "/num_frames.txt", 'r')
  num_frames = f.read()
  f.close()
  ground_truth = base_directory + "/vehicle_tracks.kw18"
  os.mkdir(experiment_name)
  old_dir = os.getcwd()
  os.chdir(experiment_name)
  config = ConfigBlock("global")
  config.Parse(config_file)
  config.AddParameter("tracker:src:image_list:file", "frames.txt");
  configLines = []
  TraverseConfig(config, "", configLines)
  configFileGenerated = "detect_and_track.conf"
  f = open(configFileGenerated, "w")
  for l in configLines :
    f.write(l + "\n")
  f.close()

  frames_in = open(input_imgs, 'r')
  frames_out = open("frames.txt", 'w')
  while 1 :
    line = frames_in.readline()
    if not line :
      break
    frames_out.write(base_directory + "/" + line)

  frames_in.close()
  frames_out.close()

  print("Generating for " + experiment_name)
  print("    Tracking")
  RunTracker( "detect_and_track.conf" )
  print("    Creating example labels")
  CreateExampleSet( ground_truth, gsd )
  print("    Creating Pos and Neg Sets based on labels")
  CreatePositiveAndNegativeSet( gsd )
  input_video = base_directory + "/raw_video.avi"
  print("    Generating check video")
  DrawVideosPVO( input_video )

  output_file.write( experiment_name + " " + os.getcwd() + "/training_pos.kw18 " + os.getcwd() + "/training_neg.kw18 " + gsd + "\n" )

  os.chdir(old_dir)

########################################
# Main method
########################################
def main() :

  argParser = OptionParser(usage="%prog [options]")
  argParser.add_option("-c", dest="config_file",
                       help="(required) The master config file to edit" )
  argParser.add_option("-e", dest="experiments",
                       help="(required) File containing the set of experiments to run")
  argParser.add_option("-t", dest="tag",
                       help="Tag to prefix all test output with",
                       default="")
  argParser.add_option("-b", dest="bin_dir",
                       help="Bin directory containing the VIDTK binaries")

  options, args = argParser.parse_args()

  # Process command line options
  if options.config_file == None :
    argParser.print_help()
    return 1
  if options.experiments == None :
    argParser.print_help()
    return 1
  if options.bin_dir != None :
    if platform.system() == "Windows" :
      os.environ["PATH"] = options.bin_dir + ";" + os.environ["PATH"]
    else :
      os.environ["PATH"] = options.bin_dir + ":" + os.environ["PATH"]

  # Generate the test name
  strtime = time.strftime("%Y_%m_%d__%H_%M_%S", time.localtime())
  testName = options.tag + strtime

  # Generate a config file
  os.mkdir(testName)
  os.chdir(testName)

  output_experiment_driver = open("driver.txt", 'w')

  experiments_in = open(options.experiments, 'r')
  while 1 :
    line = experiments_in.readline()
    if not line :
      break
    line = line.strip()
    exp_elements = line.split(' ')
    if( len(exp_elements) != 3 ) :
      continue
    base_director = exp_elements[1]
    experiment_name = exp_elements[0]
    aoi = exp_elements[2]
    CreateTrainingSet( options.config_file, base_director, experiment_name, output_experiment_driver )

  return 0

if __name__ == "__main__" :
  sys.exit(main())

