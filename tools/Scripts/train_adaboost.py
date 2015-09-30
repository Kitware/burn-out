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

# Config file manipulation
from ConfigBlock import *

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
# Train the adaboost classifier
########################################
def TrainAdaboostClassifer( prefix ) :
  args=["train_adaboost_classifer", "--input-file", "trainer_driver.txt", "--output-prefix", prefix ]

  logOut = open("train_adaboost_out.txt", "w")
  logErr = open("train_adaboost_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

def CreateTrainingSet(base_directory, experiment_name, output_file, make_check_video ) :
  input_imgs = base_directory + "/frames.txt"
  f = open(base_directory + "/gsd.txt", 'r')
  gsd = f.read()
  gsd = gsd.strip()
  f.close()
  f = open(base_directory + "/num_frames.txt", 'r')
  num_frames = f.read()
  f.close()
  ground_truth = base_directory + "/vehicle_tracks.kw18"
  old_dir = os.getcwd()
  os.chdir(experiment_name)

  print "Generating for " + experiment_name
  print "    Creating example labels"
  CreateExampleSet( ground_truth, gsd )
  print "    Creating Pos and Neg Sets based on labels"
  CreatePositiveAndNegativeSet( gsd )
  if make_check_video != None and make_check_video != 0 :
    input_video = base_directory + "/raw_video.avi"
    print "    Generating check video"
    DrawVideosPVO( input_video )

  output_file.write( experiment_name + " " + os.getcwd() + "/training_pos.kw18 " + os.getcwd() + "/training_neg.kw18 " + gsd + "\n" )

  os.chdir(old_dir)

########################################
# Main method
########################################
def main() :

  argParser = OptionParser(usage="%prog [options]")
  argParser.add_option("-e", dest="directories",
                       help="(required) Text file containing the directory name and ground truth directory and aoi")
  argParser.add_option("-d", dest="directory",
                        help="The directory to run in",
                        default="." )
  argParser.add_option("-t", dest="tag",
                       help="Tag to prefix all test output with",
                       default="")
  argParser.add_option("-b", dest="bin_dir",
                       help="Bin directory containing the VIDTK binaries")
  argParser.add_option("-v", dest="gen_video",
                       help="enables the eneration of check videos")
  argParser.add_option("-a", dest="adaboost_prefix",
                       help="adaboost prefix",
                       default="adaboost")
  options, args = argParser.parse_args()

  # Process command line options
  if options.directories == None :
    argParser.print_help()
    return 1
  if options.bin_dir != None :
    if platform.system() == "Windows" :
      os.environ["PATH"] = options.bin_dir + ";" + os.environ["PATH"]
    else :
      os.environ["PATH"] = options.bin_dir + ":" + os.environ["PATH"]

  # Generate a config file
  os.chdir(options.directory)

  output_experiment_driver = open("trainer_driver.txt", 'w')

  experiments_in = open(options.directories, 'r')
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
    CreateTrainingSet( base_director, experiment_name,
                       output_experiment_driver, options.gen_video )
  output_experiment_driver.close()
  TrainAdaboostClassifer( options.adaboost_prefix )

  return 0

if __name__ == "__main__" :
  sys.exit(main())
