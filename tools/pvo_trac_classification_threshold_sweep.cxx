/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//pvo_trac_classification_roc_input_tile_generator.cxx

#include <vcl_iostream.h>
#include <vcl_ostream.h>
#include <vcl_istream.h>
#include <vcl_fstream.h>
#include <vcl_algorithm.h>
#include <vcl_map.h>
#include <vcl_sstream.h>

#include <vul/vul_arg.h>

struct track_pvo_score
{
  unsigned int track_id_;
  double       probablity_;
};

bool operator<(track_pvo_score const & l, track_pvo_score const & r)
{
  return l.probablity_ < r.probablity_;
}

vcl_istream& operator>>(vcl_istream &is,track_pvo_score &obj)
{
  double p, o;
  is >> obj.track_id_ >> p >> obj.probablity_ >> o;
  vcl_cout << "\t" << obj.track_id_ << " " << p << " " << obj.probablity_ << " " << o << vcl_endl;
  return is;
}

//NOTE TODO right now we are only generating on vehicles

// Main
int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> classified_in_arg("--pvo", 
    "File containing the track_id prob_person prob_vehicle prob_other");
  vul_arg<vcl_string> prob_class_in_arg("--probs",
    "input in the old hadwav format");
  vul_arg<vcl_string> output_result_prefix_arg( "--output-result",
    "prefix to output file", "out" );
  vul_arg<vcl_string> gt_ct_in_arg("--gt-to-ct", "Ground truth to computed track" );
  vul_arg<vcl_string> label("--label", "Label for graph", "Vehicle" );

  vul_arg_parse( argc, argv );

  //read in plat score file
  vcl_vector< track_pvo_score > plat_scores;
  vcl_map< unsigned int, bool > has_match;
  unsigned int gt_no_match = 0;
  if(classified_in_arg.set())
  {
    vcl_cout << "Opening pvo file" << vcl_endl;
    vcl_ifstream in(classified_in_arg().c_str());
    while(in)
    {
      track_pvo_score tps;
      in >> tps;
      plat_scores.push_back(tps);
      has_match[tps.track_id_] = false;
    }
  }

  if(prob_class_in_arg.set())
  {
    vcl_cout << "Opening probs file" << vcl_endl;
    vcl_ifstream in(prob_class_in_arg().c_str());
    while(in)
    {
      track_pvo_score tps;
      in >> tps.track_id_;
      if(!in) break;
      in >> tps.probablity_;
      plat_scores.push_back(tps);
      has_match[tps.track_id_] = false;
    }
  }

  vcl_cout << "Opening gt match ct" << vcl_endl;
  {
    vcl_ifstream in(gt_ct_in_arg().c_str());
    while(in)
    {
      vcl_string tmp;
      vcl_getline( in, tmp );
      vcl_stringstream ss(tmp);
      vcl_string tag;
      int track_id;
      char dk;
      int count;
      ss >> tag >> track_id >> dk >> count;
      if( tag == "ct" && count )
      {
        //vcl_cout << tag << " " << track_id << " " << count << vcl_endl;
        has_match[track_id] = true;
      }
      if( tag == "gt"  && count == 0 )
      {
        gt_no_match++;
      }
    }
  }

  //vcl_cout << "Sort" << vcl_endl;
  //vcl_sort(plat_scores.begin(), plat_scores.end());

  vcl_cout << "Output" << vcl_endl;
  vcl_ofstream out((output_result_prefix_arg()+"_just_computed_tracks.txt").c_str());
  vcl_ofstream out_2((output_result_prefix_arg()+"_include_unmatched_gt.txt").c_str());
  if(!out)
  {
    vcl_cout << "ERROR: COULD NOT OPEN WRITE FILE" << vcl_endl;
    return 1;
  }
  for( double p = 0.0; p < 1.0; p += 0.05 )
  {
    unsigned int true_positive = 0;
    unsigned int true_negative = 0;
    unsigned int false_positive = 0;
    unsigned int false_negative = 0;
    for( vcl_vector< track_pvo_score >::const_iterator c = plat_scores.begin();
         c != plat_scores.end(); ++c )
    {
      if(c->probablity_ >= p && has_match[c->track_id_])
      {
        true_positive++;
      }
      else if(c->probablity_ >= p && !has_match[c->track_id_])
      {
        false_positive++;
      }
      else if(c->probablity_ < p && !has_match[c->track_id_] )
      {
        true_negative++;
      }
      else if(c->probablity_ < p && has_match[c->track_id_] )
      {
        false_negative++;
      }
    }
    out << "roc " << p
        << " " << label()
        << " tp: " << true_positive
        << " fp: " << false_positive
        << " tn: " << true_negative
        << " fn: " << false_negative << vcl_endl;
    out_2 << "roc " << p
          << " " << label()
          << " tp: " << true_positive
          << " fp: " << false_positive
          << " tn: " << true_negative
          << " fn: " << false_negative + gt_no_match << vcl_endl;
  }

  return 0;
}

