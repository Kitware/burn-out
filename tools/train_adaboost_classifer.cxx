/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <learning/adaboost.h>
#include <learning/histogram_weak_learners.h>
#include <learning/learner_data_class_vector.h>
#include <learning/learner_data_class_single_vector.h>
#include <learning/linear_weak_learners.h>
#include <learning/gaussian_weak_learners.h>
#include <learning/stump_weak_learners.h>

#include <classifier/track_to_vector_of_descriptors.h>

#include <vul/vul_arg.h>

#include <vnl/vnl_random.h>
#include <vnl/vnl_math.h>

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_sstream.h>
#include <vcl_iomanip.h>
#include <vcl_fstream.h>
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>
#include <ctime>
#include <cstdlib>

#include <tracking/track_reader_process.h>

#include <classifier/train_adaboost_classifier_pipeline.h>

namespace
{

void read_tracks_vidtk(vcl_string fname, vcl_vector< vidtk::track_sptr > & tracks)
{
  vidtk::track_reader_process track_reader("reader");
  vidtk::config_block config;
  config.add("disabled","false");
  config.add("format","");
  config.add("filename", fname);
  config.add("ignore_occlusions","true");
  config.add("ignore_partial_occlusions","true");
  config.add("frequency",".5"); /*MIT Reader Only*/
  track_reader.set_batch_mode(true);
  track_reader.set_params(config);
  track_reader.initialize();
  track_reader.step();
  tracks = track_reader.tracks();
}

vidtk::adaboost train(vcl_vector<vidtk::learner_training_data_sptr> const & training_set)
{
  vidtk::train_adaboost_classifier_pipeline trainer("trainer");
  vidtk::config_block config;
  config.add( "max_number_of_weak_learners", "100" );
  config.add( "gsd", "0.5" ); //This one is not being used
  config.add( "calculate_platt_score", "true" );
  trainer.set_params(config);
  trainer.initialize();
  trainer.set_training_examples(training_set);
  trainer.step();
  vidtk::adaboost ada = trainer.get_resulting_adaboost();
  return ada;
}

}

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> input_file( "--input-file", "txt file that contains: name positive_examples negative_examples gsd track_world update_area");
  vul_arg<vcl_string> output_prefix( "--output-prefix", "The output prefix" );
  vul_arg_parse( argc, argv );

  if(!input_file.set())
  {
    vcl_cerr << "NEED INPUT" << vcl_endl;
    return 0;
  }
  vcl_ifstream in(input_file().c_str());
  if( !in )
  {
    vcl_cerr << "could not open: " << input_file() << vcl_endl;
  }
  vcl_vector<vidtk::learner_training_data_sptr> positive;
  vcl_vector<vidtk::learner_training_data_sptr> negative;
  while(in)
  {
    vcl_string pos, neg, name;
    double gsd;
    in >> name;
    bool world, area;
    if(in)
    {
      in >> pos >> neg >> gsd >> world >> area;
    }
    else
    {
      break;
    }
    vcl_vector< vidtk::track_sptr > tracks_pos, tracks_neg;
    read_tracks_vidtk(pos, tracks_pos);
    read_tracks_vidtk(neg, tracks_neg);
    vidtk::track_to_vector_of_descriptors ttvod(gsd, world, area);
    for(unsigned int i = 0; i < tracks_pos.size(); ++i)
    {
      positive.push_back(ttvod.generateTrainingData( tracks_pos[i], 1 ));
    }
    for(unsigned int i = 0; i < tracks_neg.size(); ++i)
    {
      negative.push_back(ttvod.generateTrainingData( tracks_neg[i], -1 ));
    }
  }

  random_shuffle(positive.begin(), positive.end());
  random_shuffle(negative.begin(), negative.end());
  vcl_vector<vidtk::learner_training_data_sptr> used_set;
  vcl_vector<vidtk::learner_training_data_sptr> unused_set;
  unsigned int size = vcl_min(positive.size(), negative.size());
  unsigned int j;
  for( j = 0; j < size; ++j )
  {
    used_set.push_back(positive[j]);
    used_set.push_back(negative[j]);
  }
  ///Only one of these two for loops should happen
  for( ; j < positive.size(); ++j)
  {
    unused_set.push_back(positive[j]);
  }
  for( ; j < negative.size(); ++j)
  {
    unused_set.push_back(negative[j]);
  }
  vidtk::adaboost ada = train(used_set);

  {
    vcl_string out_name = output_prefix() + "_classifer.ada";
    vcl_ofstream out(out_name.c_str());
    ada.write(out);
    out.close();
  }
  {
    vcl_string out_name = output_prefix() + "_used_desc.txt";
    vcl_ofstream out(out_name.c_str());
    for( unsigned int i = 0; i < used_set.size(); ++i )
    {
      out << *(used_set[i]) << vcl_endl;
    }
  }
  {
    vcl_string out_name = output_prefix() + "_unused_desc.txt";
    vcl_ofstream out(out_name.c_str());
    for( unsigned int i = 0; i < unused_set.size(); ++i )
    {
      out << *(unused_set[i]) << vcl_endl;
    }
  }

  return 0;
}
