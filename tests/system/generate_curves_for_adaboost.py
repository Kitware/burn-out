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
# Step 1.5: Compute track stats
########################################
def ComputeTrackStats( gt, tracks, frame_count, gsd, aoi ) :
  args=["score_tracks", "--ground-truth-tracks", gt, "--num-frames", frame_count, "--gsd", gsd, "--aoi", aoi, "--computed-tracks", tracks, "--hadwav", "--matches", "ct_to_gt.txt"]

  logOut = open("score_tracks_out.txt", "w")
  logErr = open("score_tracks_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 2: classify tracks
########################################
def ClassifyTracks( config_file ) :
  args=["classify_and_filter_tracks", "-c", config_file]

  logOut = open("classifier_out.txt", "w")
  logErr = open("classifier_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 2.5: extract pvo
########################################
def ExtractPVO( ) :
  args=["vsl_track_to_pvo_text", "--tracks", "classified_tracks.vsl", "--output", "pvo_probs.txt"]

  logOut = open("extract_pvo_out.txt", "w")
  logErr = open("extract_pvo_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 3: Generate Classification Accuracy
########################################
def ClassificationAccuracy( ) :
  args=["pvo_trac_classification_threshold_sweep", "--pvo", "pvo_probs.txt", "--gt-to-ct", "ct_to_gt.txt", "--output-result", "classification_accuracy.txt"]

  logOut = open("ClassificationAccuracy_out.txt", "w")
  logErr = open("ClassificationAccuracy_err.txt", "w")

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

########################################
# Step 1.7: draw_tracks_on_video
########################################
def DrawVideosPVO( input_video, tracks ) :
  f = open("example_tracks.txt", "w")
  f.write( tracks + " 0 0 0 1\n" )
  f.close()
  args=["draw_tracks_on_video", "--track-info-file", "example_tracks.txt", "--src-video", input_video, "--out-video", "training_set.avi",
        "--track_noise_classification", "pvo_probs.txt"]

  logOut = open("draw_out.txt", "w")
  logErr = open("draw_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

def RunExperiment(tracks, adaboost_file, base_directory, experiment_name, aoi) :
  f = open(base_directory + "/gsd.txt", 'r')
  gsd = f.read()
  f.close()
  f = open(base_directory + "/num_frames.txt", 'r')
  num_frames = f.read()
  f.close()
  ground_truth = base_directory + "/vehicle_tracks.kw18"

  ada_adaboost = "adaboost_track_classifier:ada_classifier:"
  ada_writer = "adaboost_track_classifier:result_writer:"
  ada_input = "adaboost_track_classifier:track_reader:"

  old_dir = os.getcwd()
  print os.path.basename(adaboost_file)
  new_dir = os.path.basename(adaboost_file)
  os.mkdir(new_dir)
  os.chdir(new_dir)

  ComputeTrackStats( ground_truth, tracks, num_frames, gsd, aoi )

  classifiction_config = ConfigBlock("global")

  classifiction_config.AddParameter(ada_adaboost + "adaboost_file", adaboost_file)
  classifiction_config.AddParameter(ada_adaboost + "classifer_type", "vehicle" )
  classifiction_config.AddParameter(ada_adaboost + "gsd", gsd)
  classifiction_config.AddParameter(ada_adaboost + "min_number_of_detections", "5")
  classifiction_config.AddParameter(ada_adaboost + "min_probablity", "0.0")

  classifiction_config.AddParameter(ada_writer + "disabled", "false")
  classifiction_config.AddParameter(ada_writer + "filename", "classified_tracks.vsl")
  classifiction_config.AddParameter(ada_writer + "overwrite_existing", "true")
  classifiction_config.AddParameter(ada_writer + "format", "vsl")

  classifiction_config.AddParameter(ada_input + "filename", tracks)

  configLines = []
  TraverseConfig(classifiction_config, "", configLines)
  configFileGenerated = "classify_and_filter_tracks.conf"
  f = open(configFileGenerated, "w")
  for l in configLines :
    f.write(l + "\n")
  f.close()

  print "Classifying the tracks"
  ClassifyTracks( "classify_and_filter_tracks.conf" )
  ExtractPVO( )
  input_video = base_directory + "/raw_video.avi"
  DrawVideosPVO( input_video, tracks )

  print "Generating the accuracy file"
  ClassificationAccuracy( )

  os.chdir(old_dir)

########################################
# Main method
########################################
def main() :

  argParser = OptionParser(usage="%prog [options]")
  argParser.add_option("--test", dest="test",
                       help="(required) File containing the test information")
  argParser.add_option("-t", dest="tag",
                       help="Tag to prefix all test output with",
                       default="")
  argParser.add_option("-b", dest="bin_dir",
                       help="Bin directory containing the VIDTK binaries")

  options, args = argParser.parse_args()

  # Process command line options
  if options.test == None :
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

  experiments_in = open(options.test, 'r')
  line = experiments_in.readline()
  line = line.strip()
  exp_elements = line.split(' ')
  base_director = exp_elements[1]
  experiment_name = exp_elements[0]
  aoi = exp_elements[2]
  tracker_result = exp_elements[3]
  while 1 :
    line = experiments_in.readline()
    if not line :
      break
    line = line.strip()
    adaboost_file = line
    RunExperiment(tracker_result, adaboost_file, base_director, experiment_name, aoi )

  return 0

if __name__ == "__main__" :
  sys.exit(main())

