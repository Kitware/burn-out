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
import math
from subprocess import call
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
# Step 1.5: Compute track stats
########################################
def ComputeTrackStats( gt, frame_count, gsd, aoi ) :
  args=["score_tracks", "--ground-truth-tracks", gt, "--num-frames", frame_count, "--gsd", gsd, "--aoi", aoi, "--computed-tracks", "tracks_unfiltered.kw18", "--hadwav", "--matches", "ct_to_gt.txt"]

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
  args=["pvo_trac_classification_threshold_sweep", "--pvo", "pvo_probs.txt", "--gt-to-ct", "ct_to_gt.txt", "--output-result", "classification_accuracy"]

  logOut = open("ClassificationAccuracy_out.txt", "w")
  logErr = open("ClassificationAccuracy_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  draw_plot( "classification_accuracy_include_unmatched_gt.txt", "Vehicle", "Vechicle_with_unmatch", "1.05" )
  draw_plot( "classification_accuracy_just_computed_tracks.txt", "Vehicle", "Vechicle_just_computed", "1.05" );

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 4: Detect events on tracks
########################################
def DetectEvents( config_file ) :
  args=["detect_events", "-c", config_file]

  logOut = open("detect_events_out.txt", "w")
  logErr = open("detect_events_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

########################################
# Step 4: Detect events on tracks
########################################
def DetectActivities( config_file ) :
  args=["detect_activities", "-c", config_file]

  logOut = open("detect_activities_out.txt", "w")
  logErr = open("detect_activities_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0


def ConvertTrackFormat( tracks ) :
  args=["convert_track_format", "--input-tracks", tracks, "--output-format", "kw18"]

  logOut = open("convert_track_out.txt", "w")
  logErr = open("convert_track_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

def ApplyEventNormalcyModel( ) :
  args=["apply_event_normalcy_model", "-t", "classified_tracks.vsl",  "-e", "detected_events.txt",
                                      "-m", "normalcy_histogram.txt", "-o", "event_normalcy_score.txt"]

  logOut = open("apply_event_norm_out.txt", "w")
  logErr = open("apply_event_norm_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

def TrainActivityNormalcyModel( ) :
  args=["train_activity_normalcy_model", "-t", "classified_tracks.vsl",  "-e", "detected_events.txt",
                                         "-a", "activities_result.txt",  "-o", "activity_model.norm"]

  logOut = open("train_act_norm_out.txt", "w")
  logErr = open("train_act_norm_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

def ApplyActivityNormalcyModel( ) :
  args=["apply_activity_normalcy_model", "-t", "classified_tracks.vsl",  "-e", "detected_events.txt",
                                         "-a", "activities_result.txt",  "-m", "activity_model.norm",
                                         "-o", "activity_normalcy_score.txt"]

  logOut = open("apply_act_norm_out.txt", "w")
  logErr = open("apply_act_norm_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

def TrainEventCountNormalcy( aoi, gsd ) :
  m = re.split("[x,+]", aoi )
  w = int(math.ceil(float(m[0])*float(gsd)))
  h = int(math.ceil(float(m[1])*float(gsd)))
  print(str(w) + " " + str(h))
  event_config = ConfigBlock("global")
  event_config.AddParameter("bounds_min_x", "-5")
  event_config.AddParameter("bounds_min_y", "-5")
  event_config.AddParameter("bounds_max_x", str(w+5))
  event_config.AddParameter("bounds_max_y", str(h+5))
  event_config.AddParameter("cell_size_x", "2")
  event_config.AddParameter("cell_size_y", "2")
  event_config.AddParameter("output_model_file", "normalcy_histogram.txt")
  event_config.AddParameter("event_detector:track_reader:filename", "classified_tracks.vsl")
  event_config.AddParameter("read_tracks_in_one_step", "true")
  event_config.AddParameter("event_detector:following_event_detector:disable", "false")
  event_config.AddParameter("event_detector:aimless_driving_event_detector:disable", "false")
  event_config.AddParameter("event_detector:aimless_driving_event_detector:disable", "false")
  event_config.AddParameter("event_detector:following_event_detector:disable", "false")
  event_config.AddParameter("event_detector:kwsoti2_event_detector:disable", "false")
  event_config.AddParameter("event_detector:kwsoti2_event_detector:gsd", "1.0")
  event_config.AddParameter("event_detector:meeting_event_detector:disable","false")
  ConvertTrackFormat( "classified_tracks.vsl" )
  event_config.AddParameter("event_detector:meeting_event_detector:tracks_file", "classified_tracks.vsl.kw18")
  configLines = []
  TraverseConfig(event_config, "", configLines)
  configFileGenerated = "normalcy_model.conf"
  f = open(configFileGenerated, "w")
  for l in configLines :
    f.write(l + "\n")
  f.close()

  args=["train_event_normalcy_model", "-c", configFileGenerated]

  logOut = open("norm_out.txt", "w")
  logErr = open("norm_err.txt", "w")

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

def convert_events() :
  args=["event_converter", "event_converter:event_reader:events_filename=detected_events.txt",
                           "event_converter:hadwav_format_writer:disabled=false",
                           "event_converter:hadwav_format_writer:filename=hadwav_detected_events.txt",
                           "event_converter:track_reader:filename=classified_tracks.vsl",
                           "read_tracks_in_one_step=true" ]

  logOut = open("convert_events_out.txt", "w")
  logErr = open("convert_events_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

def score_events( gt, gte, frame_count, gsd, aoi ) :
  args=["hadwav_score_events", "--tt", gt,
                               "--ct", "tracks_unfiltered.kw18",
                               "--te", gte,
                               "--ce", "hadwav_detected_events.txt",
                               "--plot-log", "hadwav_event_scores.txt",
                               "--aoi", aoi,
                               "--gsd", gsd,
                               "--num-frames", frame_count ]

  logOut = open("score_events_out.txt", "w")
  logErr = open("score_events_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  draw_plot_event( "hadwav_event_scores.txt", "STARTING", "starting", "3" )
  draw_plot_event( "hadwav_event_scores.txt", "STOPPING", "stopping", "3" )
  draw_plot_event( "hadwav_event_scores.txt", "TURNING", "turning", "3" )
  draw_plot_event( "hadwav_event_scores.txt", "UTURN", "uturn", "3" )
  return proc_ret == 0

def RunExperiment(config_file, base_directory, experiment_name, aoi, adaboost) :
  input_imgs = base_directory + "/frames.txt"
  f = open(base_directory + "/gsd.txt", 'r')
  gsd = f.read()
  gsd = gsd.strip()
  f.close()
  f = open(base_directory + "/num_frames.txt", 'r')
  num_frames = f.read()
  num_frames = num_frames.strip()
  f.close()
  ground_truth = base_directory + "/vehicle_tracks.kw18"
  try:
    if ( not( os.path.isdir(experiment_name) ) ) :
      os.mkdir(experiment_name)
  except:
    print(experiment_name + " seems to already exist")
  old_dir = os.getcwd()
  os.chdir(experiment_name)
  config = ConfigBlock("global")
  config.Parse(config_file)
  config.AddParameter("detect_and_track:src:image_list:file", "frames.txt")
  config.AddParameter("detect_and_track:full_tracking_sp:trans_for_ground:premult_scale", gsd)
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

  print("Tracking")
  RunTracker( "detect_and_track.conf" )
  print("Computing tracking scores")
  ComputeTrackStats( ground_truth, num_frames, gsd, aoi )

  dirOfConfig = os.path.dirname(config_file)
  try:
    shutil.copy(adaboost, "./classifier.ada")
  except OSError:
    pass

  classifiction_config = ConfigBlock("global")

  ada_adaboost = "adaboost_track_classifier:ada_classifier:"
  ada_writer = "adaboost_track_classifier:result_writer:"
  ada_input = "adaboost_track_classifier:track_reader:"

  classifiction_config.AddParameter(ada_adaboost + "adaboost_file", "classifier.ada")
  classifiction_config.AddParameter(ada_adaboost + "classifer_type", "vehicle" )
  classifiction_config.AddParameter(ada_adaboost + "gsd", gsd)
  classifiction_config.AddParameter(ada_adaboost + "min_number_of_detections", "5")
  classifiction_config.AddParameter(ada_adaboost + "min_probablity", "0.0")
  classifiction_config.AddParameter(ada_adaboost + "area_in_gsd", "false")
  classifiction_config.AddParameter(ada_adaboost + "location_in_gsd", "true")

  classifiction_config.AddParameter(ada_writer + "disabled", "false")
  classifiction_config.AddParameter(ada_writer + "filename", "classified_tracks.vsl")
  classifiction_config.AddParameter(ada_writer + "overwrite_existing", "true")
  classifiction_config.AddParameter(ada_writer + "format", "vsl")

  classifiction_config.AddParameter(ada_input + "filename", "tracks_unfiltered.kw18")

  configLines = []
  TraverseConfig(classifiction_config, "", configLines)
  configFileGenerated = "classify_and_filter_tracks.conf"
  f = open(configFileGenerated, "w")
  for l in configLines :
    f.write(l + "\n")
  f.close()

  print("Classifying the tracks")
  ClassifyTracks( "classify_and_filter_tracks.conf" )
  ExtractPVO( )

  print("Generating the accuracy file")
  ClassificationAccuracy( )

  print("Run Event Detector")
  event_config = ConfigBlock("global")
  event_config.AddParameter("event_detector:track_reader:filename", "classified_tracks.vsl")
  event_config.AddParameter("event_detector:read_tracks_in_one_step", "true")
  event_config.AddParameter("event_detector:event_writer:filename", "detected_events.txt")
  event_config.AddParameter("event_detector:following_event_detector:disable", "false")
  event_config.AddParameter("event_detector:aimless_driving_event_detector:disable", "false")
  event_config.AddParameter("event_detector:aimless_driving_event_detector:disable", "false")
  event_config.AddParameter("event_detector:following_event_detector:disable", "false")
  event_config.AddParameter("event_detector:kwsoti2_event_detector:disable", "false")
  event_config.AddParameter("event_detector:kwsoti2_event_detector:gsd", "1.0")
  event_config.AddParameter("event_detector:meeting_event_detector:disable","false")
  ConvertTrackFormat( "classified_tracks.vsl" )
  event_config.AddParameter("event_detector:meeting_event_detector:tracks_file", "classified_tracks.vsl.kw18")
  configLines = []
  TraverseConfig(event_config, "", configLines)
  configFileGenerated = "detect_events.conf"
  f = open(configFileGenerated, "w")
  for l in configLines :
    f.write(l + "\n")
  f.close()
  DetectEvents( "detect_events.conf" )
  TrainEventCountNormalcy( aoi, gsd )
  convert_events( )
  ground_truth_events = base_directory + "/vehicle_p3_events.txt"
  score_events( ground_truth, ground_truth_events, num_frames, gsd, aoi )
  ApplyEventNormalcyModel( )

  print("Run activity Detector")
  act_config = ConfigBlock("global")
  act_config.AddParameter("activity_detector:activity_writer:filename", "activities_result.txt")
  act_config.AddParameter("activity_detector:event_reader:events_filename", "detected_events.txt")
  act_config.AddParameter("activity_detector:track_reader:filename", "classified_tracks.vsl")
  act_config.AddParameter("detector_type", "long_term_following")
  act_config.AddParameter("read_tracks_in_one_step", "true")
  configLines = []
  TraverseConfig(act_config, "", configLines)
  configFileGenerated = "detect_activities.conf"
  f = open(configFileGenerated, "w")
  for l in configLines :
    f.write(l + "\n")
  f.close()
  DetectActivities( "detect_activities.conf" )
  TrainActivityNormalcyModel( )
  ApplyActivityNormalcyModel( )

  os.chdir(old_dir)

def draw_plot( input_file, tag, out, max_y ) :
  resultFile = input_file
  plotFile = resultFile + '.plt'
  gnuFile = open( plotFile, 'w')

  fin = open(input_file, "r")
  fout = open( out+".dat", "w" )
  while 1 :
    line = fin.readline()
    if not line :
      break
    line = line.strip()
    exp_elements = line.split(' ')
    if exp_elements[2] == tag :
      tp = float(exp_elements[4])
      fp = float(exp_elements[6])
      tn = float(exp_elements[8])
      fn = float(exp_elements[10])
      tpr = 0
      if(tp+fn != 0) :
        tpr = tp/(tp+fn)
      fpr = 0
      if(fp+tn != 0) :
        fpr = fp/(fp+tn)
      fout.write( str(fpr) + " " + str(tpr) + "\n" )
  fout.close()
  fin.close()

  gnuFile.write( 'set xrange [-0.05:'+max_y+']\n')
  gnuFile.write( 'set yrange [-0.05:1.05]\n')
  gnuFile.write( 'set xlabel \"fp/(fp+tn)\"\n')
  gnuFile.write( 'set ylabel \"tp/(tp+fn)\"\n')
  gnuFile.write( 'set title \"True Positives vs False Alarms\"\n')
  gnuFile.write( 'set term png size 512, 512; set out "' + out + '.png"\n')
  gnuFile.write( 'plot \"' + out+".dat" + '\" using 1:2 with linespoints\n' )
  gnuFile.close()

  return_code = call( 'gnuplot ' + plotFile, shell=True)

def draw_plot_event( input_file, tag, out, max_y ) :
  resultFile = input_file
  plotFile = resultFile + '.plt'
  gnuFile = open( plotFile, 'w')

  fin = open(input_file, "r")
  fout = open( out+".dat", "w" )
  while 1 :
    line = fin.readline()
    if not line :
      break
    line = line.strip()
    exp_elements = line.split(' ')
    if exp_elements[2] == tag :
      fout.write( exp_elements[14] + " " + exp_elements[12] + "\n" )
  fout.close()
  fin.close()

  gnuFile.write( 'set xrange [-0.05:'+max_y+']\n')
  gnuFile.write( 'set yrange [-0.05:1.05]\n')
  gnuFile.write( 'set xlabel \"far\"\n')
  gnuFile.write( 'set ylabel \"pd\"\n')
  gnuFile.write( 'set title \"True Positives vs False Alarms\"\n')
  gnuFile.write( 'set term png size 512, 512; set out "' + out + '.png"\n')
  gnuFile.write( 'plot \"' + out+".dat" + '\" using 1:2 with linespoints\n' )
  gnuFile.close()

  return_code = call( 'gnuplot ' + plotFile, shell=True)

def run_generate_html(script) :
  args=["python", script ]

  logOut = open("gen_html_out.txt", "w")
  logErr = open("gen_html_err.txt", "w")

  proc = subprocess.Popen(args,stdout=logOut, stderr=logErr)
  proc_ret = proc.wait()

  logOut.close()
  logErr.close()
  return proc_ret == 0

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
  argParser.add_option("-s", dest="html_script", help="The HTML generation script" )

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

  experiments_in = open(options.experiments, 'r')
  while 1 :
    line = experiments_in.readline()
    if not line :
      break
    line = line.strip()
    exp_elements = line.split(' ')
    if( len(exp_elements) != 4 ) :
      continue
    base_director = exp_elements[1]
    experiment_name = exp_elements[0]
    aoi = exp_elements[2]
    adaboost_file = exp_elements[3]
    RunExperiment( options.config_file, base_director, experiment_name, aoi, adaboost_file )
  if options.html_script == None :
    print("Cannot generate website")
    return 0
  os.chdir(testName + "/..")
  run_generate_html(options.html_script)
  return 0

if __name__ == "__main__" :
  sys.exit(main())

