/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <track_oracle/scoring_framework/score_frames_aipr.h>

#include <tracking/track_reader_process.h>
#include <vcl_iostream.h>
#include <vcl_fstream.h>
#include <vul/vul_arg.h>

using namespace vidtk;

int main(int argc, char** argv)
{
  vul_arg<vcl_string> computed_tracks_arg( "--computed-tracks", "Path to the computed track file", "");
  vul_arg<vcl_string> ground_truth_tracks_arg( "--ground-truth-tracks", "Path to the ground truth track file", "");
  vul_arg<vcl_string> computed_track_format_arg("--computed-format", "Format computed track file is in", "");
  vul_arg<vcl_string> ground_truth_track_format_arg("--ground-truth-format", "Format ground truth track file is in", "");
  vul_arg<vcl_string> min_overlap_ratio_arg("--min-overlap-ratio", "Ignore overlap if overlap score less than", "");
  vul_arg<bool> output_all_arg("--output-all-frames", "Outputs the score of every frame instead of just the average", false);
  vul_arg_parse( argc, argv );

  if(!computed_tracks_arg.set() || !ground_truth_tracks_arg.set())
  {
    vcl_cerr << "ERROR: Track files not all specified\n";
    return EXIT_FAILURE;
  }

    //Read in computed tracks
  vcl_vector<vidtk::track_sptr> comp_tracks;
  track_reader_process trp("track_reader");
  config_block blk = trp.params();
  blk.set("filename",computed_tracks_arg());
  if(computed_track_format_arg.set())
  {
    blk.set("format", computed_track_format_arg());
  }
  trp.set_params(blk);
  trp.set_batch_mode(true);
  trp.initialize();
  trp.step();
  comp_tracks = trp.tracks();

  //read in ground truth tracks
  vcl_vector<vidtk::track_sptr> gt_tracks;
  blk.set("filename",ground_truth_tracks_arg());
  if(ground_truth_track_format_arg.set())
  {
    blk.set("format", ground_truth_track_format_arg());
  }
  trp.set_params(blk);
  trp.set_batch_mode(true);
  trp.initialize();
  trp.step();
  gt_tracks = trp.tracks();

  //create metrics
  vcl_vector<frame_based_metrics> fbm;

  //compute metric values
  compute_metrics(fbm,
    gt_tracks,
    comp_tracks,
    min_overlap_ratio_arg.set() ? atof(min_overlap_ratio_arg().c_str()) : 0);

  if(fbm.size() == 0)
  {
    vcl_cerr << "Error: no metrics able to be computed\n";
    return EXIT_FAILURE;
  }
  frame_based_metrics avg_fbm;
  avg_fbm.detection_probability_count = 0;
  avg_fbm.detection_probability_presence = 0;
  avg_fbm.false_negatives_count = 0;
  avg_fbm.false_positives_count = 0;
  avg_fbm.merge_fraction = 0;
  avg_fbm.num_merge = 0;
  avg_fbm.num_merge_mult = 0;
  avg_fbm.num_split = 0;
  avg_fbm.num_split_mult = 0;
  avg_fbm.precision_count = 0;
  avg_fbm.precision_presence = 0;
  avg_fbm.split_fraction = 0;
  avg_fbm.true_positives_count = 0;
  avg_fbm.true_positives_presence = 0;
  avg_fbm.gt_with_associations = 0;
  avg_fbm.gt_without_associations = 0;

  for(unsigned i = 0; i < fbm.size(); ++i)
  {
    avg_fbm.detection_probability_count += fbm[i].detection_probability_count;
    avg_fbm.detection_probability_presence += fbm[i].detection_probability_presence;
    avg_fbm.false_negatives_count += fbm[i].false_negatives_count;
    avg_fbm.false_positives_count += fbm[i].false_positives_count;
    avg_fbm.merge_fraction += fbm[i].merge_fraction;
    avg_fbm.num_merge += fbm[i].num_merge;
    avg_fbm.num_merge_mult += fbm[i].num_merge_mult;
    avg_fbm.num_split += fbm[i].num_split;
    avg_fbm.num_split_mult += fbm[i].num_split_mult;
    avg_fbm.precision_count += fbm[i].precision_count;
    avg_fbm.precision_presence += fbm[i].precision_presence;
    avg_fbm.split_fraction += fbm[i].split_fraction;
    avg_fbm.true_positives_count += fbm[i].true_positives_count;
    avg_fbm.true_positives_presence += fbm[i].true_positives_presence;
    avg_fbm.gt_with_associations += fbm[i].gt_with_associations;
    avg_fbm.gt_without_associations += fbm[i].gt_without_associations;
  }

  vcl_cout << "Total Number Frame Based Metrics" << vcl_endl;
  vcl_cout << "  Detection Probability Count:     " << avg_fbm.detection_probability_count << vcl_endl;
  vcl_cout << "  Detection Probability Presence:  " << avg_fbm.detection_probability_presence << vcl_endl;
  vcl_cout << "  False Negative Count:            " << avg_fbm.false_negatives_count << vcl_endl;
  vcl_cout << "  False Positives Count:           " << avg_fbm.false_positives_count << vcl_endl;
  vcl_cout << "  GT with associations:            " << avg_fbm.gt_with_associations << vcl_endl;
  vcl_cout << "  Total GT with associations:      " << avg_fbm.gt_with_associations << vcl_endl;
  vcl_cout << "  GT without associations:         " << avg_fbm.gt_without_associations << vcl_endl;
  vcl_cout << "  Merge Fraction:                  " << avg_fbm.merge_fraction << vcl_endl;
  vcl_cout << "  Num Merge:                       " << avg_fbm.num_merge << vcl_endl;
  vcl_cout << "  Num Merge Mult:                  " << avg_fbm.num_merge_mult << vcl_endl;
  vcl_cout << "  Num Split:                       " << avg_fbm.num_split << vcl_endl;
  vcl_cout << "  Num Split Mult:                  " << avg_fbm.num_split_mult << vcl_endl;
  vcl_cout << "  Precision Count:                 " << avg_fbm.precision_count << vcl_endl;
  vcl_cout << "  Precision Presence:              " << avg_fbm.precision_presence << vcl_endl;
  vcl_cout << "  Split Fraction:                  " << avg_fbm.split_fraction << vcl_endl;
  vcl_cout << "  True Positive Count:             " << avg_fbm.true_positives_count << vcl_endl;
  vcl_cout << "  True Positive Presence:          " << avg_fbm.true_positives_presence << vcl_endl;
  vcl_cout << "  Number of Frames:                " << fbm.size() << vcl_endl;
  vcl_cout << vcl_endl;
  vcl_cout << "-----------------------------------" << vcl_endl;
  vcl_cout << vcl_endl;

  avg_fbm.detection_probability_count /= fbm.size();
  avg_fbm.detection_probability_presence /= fbm.size();
  avg_fbm.false_negatives_count /= fbm.size();
  avg_fbm.false_positives_count /= fbm.size();
  avg_fbm.merge_fraction /= fbm.size();
  avg_fbm.num_merge /= fbm.size();
  avg_fbm.num_merge_mult /= fbm.size();
  avg_fbm.num_split /= fbm.size();
  avg_fbm.num_split_mult /= fbm.size();
  avg_fbm.precision_count /= fbm.size();
  avg_fbm.precision_presence /= fbm.size();
  avg_fbm.split_fraction /= fbm.size();
  avg_fbm.true_positives_count /= fbm.size();
  avg_fbm.true_positives_presence /= fbm.size();
  avg_fbm.gt_with_associations /= fbm.size();
  avg_fbm.gt_without_associations /= fbm.size();

  vcl_cout << "Average Frame Based Metrics" << vcl_endl;
  vcl_cout << "  Detection Probability Count:     " << avg_fbm.detection_probability_count << vcl_endl;
  vcl_cout << "  Detection Probability Presence:  " << avg_fbm.detection_probability_presence << vcl_endl;
  vcl_cout << "  False Negative Count:            " << avg_fbm.false_negatives_count << vcl_endl;
  vcl_cout << "  False Positives Count:           " << avg_fbm.false_positives_count << vcl_endl;
  vcl_cout << "  GT with associations:            " << avg_fbm.gt_with_associations << vcl_endl;
  vcl_cout << "  GT without associations:         " << avg_fbm.gt_without_associations << vcl_endl;
  vcl_cout << "  Merge Fraction:                  " << avg_fbm.merge_fraction << vcl_endl;
  vcl_cout << "  Num Merge:                       " << avg_fbm.num_merge << vcl_endl;
  vcl_cout << "  Num Merge Mult:                  " << avg_fbm.num_merge_mult << vcl_endl;
  vcl_cout << "  Num Split:                       " << avg_fbm.num_split << vcl_endl;
  vcl_cout << "  Num Split Mult:                  " << avg_fbm.num_split_mult << vcl_endl;
  vcl_cout << "  Precision Count:                 " << avg_fbm.precision_count << vcl_endl;
  vcl_cout << "  Precision Presence:              " << avg_fbm.precision_presence << vcl_endl;
  vcl_cout << "  Split Fraction:                  " << avg_fbm.split_fraction << vcl_endl;
  vcl_cout << "  True Positive Count:             " << avg_fbm.true_positives_count << vcl_endl;
  vcl_cout << "  True Positive Presence:          " << avg_fbm.true_positives_presence << vcl_endl;
  vcl_cout << vcl_endl;
  vcl_cout << "Frame Based Metrics" << vcl_endl;
  for(unsigned i = 0 ; i < fbm.size() && output_all_arg(); i++)
  {
    vcl_cout << "  Frame " << i << ": "<< vcl_endl;
    vcl_cout << "    Detection Probability Count:     " << fbm[i].detection_probability_count << vcl_endl;
    vcl_cout << "    Detection Probability Presence: " << fbm[i].detection_probability_presence << vcl_endl;
    vcl_cout << "    False Negative Count:            " << fbm[i].false_negatives_count << vcl_endl;
    vcl_cout << "    False Positives Count:           " << fbm[i].false_positives_count << vcl_endl;
    vcl_cout << "    GT with associations:            " << fbm[i].gt_with_associations << vcl_endl;
    vcl_cout << "    GT without associations:         " << fbm[i].gt_without_associations << vcl_endl;
    vcl_cout << "    Merge Fraction:                  " << fbm[i].merge_fraction << vcl_endl;
    vcl_cout << "    Num Merge:                       " << fbm[i].num_merge << vcl_endl;
    vcl_cout << "    Num Merge Mult:                  " << fbm[i].num_merge_mult << vcl_endl;
    vcl_cout << "    Num Split:                       " << fbm[i].num_split << vcl_endl;
    vcl_cout << "    Num Split Mult:                  " << fbm[i].num_split_mult << vcl_endl;
    vcl_cout << "    Precision Count:                 " << fbm[i].precision_count << vcl_endl;
    vcl_cout << "    Precision Presence:              " << fbm[i].precision_presence << vcl_endl;
    vcl_cout << "    Split Fraction:                  " << fbm[i].split_fraction << vcl_endl;
    vcl_cout << "    True Positive Count:             " << fbm[i].true_positives_count << vcl_endl;
    vcl_cout << "    True Positive Presence:          " << fbm[i].true_positives_presence << vcl_endl;
  }

  return EXIT_SUCCESS;
}
