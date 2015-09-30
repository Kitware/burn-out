/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//multi_dataset_training_test.cxx

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

vidtk::adaboost train(vcl_vector<vidtk::learner_training_data_sptr> const & training_set)
{
  vidtk::train_adaboost_classifier_pipeline trainer("trainer");
  vidtk::config_block config;
  config.add( "max_number_of_weak_learners", "100" );
  config.add( "gsd", "0.5" );
  config.add( "calculate_platt_score", "true" );
  trainer.set_params(config);
  trainer.initialize();
  trainer.set_training_examples(training_set);
  trainer.step();
  vidtk::adaboost ada = trainer.get_resulting_adaboost();
  return ada;
}

enum class_type{ tp_loc = 0, tn_loc = 1, fn_loc = 2, fp_loc = 3 };

vnl_vector< double > test( vidtk::adaboost ada, unsigned up_to,
                           vcl_vector<vidtk::learner_training_data_sptr> const & testing_examples )
{
  double correct_negative = 0;
  double correct_positive = 0;
  double false_negative = 0;
  double false_positive = 0;
  for(unsigned int t = 0; t < testing_examples.size(); ++t)
  {
    int classification = ada.classify( *(testing_examples[t]), up_to );
    if(classification == 1 && (testing_examples[t])->label() == 1)
    {
      correct_positive++;
    }
    else if(classification == -1 && (testing_examples[t])->label() == -1)
    {
      correct_negative++;
    }
    else if(classification == -1 && (testing_examples[t])->label() == 1)
    {
      false_negative++;
    }
    else if(classification == 1 && (testing_examples[t])->label() == -1)
    {
      false_positive++;
    }
  }
  vnl_vector<double> result(4);
  result[tp_loc] = correct_positive;
  result[tn_loc] = correct_negative;
  result[fn_loc] = false_negative;
  result[fp_loc] = false_positive;
  return result;
}

void output_set( vcl_vector<vidtk::learner_training_data_sptr> const & data_set,
                 vcl_string fname);

void
train_and_test_output( vcl_vector<vidtk::learner_training_data_sptr> const & training_examples,
                       vcl_vector<vidtk::learner_training_data_sptr> const & testing_examples,
                       vcl_string prefix, vcl_ofstream & out,
                       vcl_string first_part_header = "", vcl_string first_part = "" )
{
  output_set( training_examples, prefix + "_training_set.txt" );
  output_set( training_examples, prefix + "_testing_set.txt" );
  vidtk::adaboost ada = train(training_examples);
  vnl_vector< double >  result = test( ada, ada.number_of_weak_learners(), testing_examples );
  vcl_ofstream ada_write((prefix+".ada").c_str());
  ada.write(ada_write);
  out << first_part
          << training_examples.size() << "," << testing_examples.size() << ","
          << result[tp_loc] << "," << result[tn_loc] << ","
          << result[fn_loc] << "," << result[fp_loc] << ","
          << (result[tp_loc]+result[tn_loc]) /
              (result[tp_loc]+result[tn_loc]+result[fn_loc]+result[fp_loc]) << ","
          << (result[tp_loc])/(result[tp_loc]+result[fn_loc]) << ","
          << (result[tn_loc])/(result[tn_loc]+result[fp_loc]) << ","
          << (result[tp_loc])/(result[tp_loc]+result[fp_loc]) << ","
          << (result[tn_loc])/(result[tn_loc]+result[fn_loc])
          << vcl_endl;
  {
    vcl_ofstream out2((prefix+".csv").c_str());
    out2 << first_part_header << "number of weak learners used,testing set size,tp,tn,fn,fp,"
            "accuracy,sensitivity,specificity,positive predictive value,Negative predictive value" << vcl_endl;
    for(unsigned int r = 1; r <= ada.number_of_weak_learners(); r++)
    {
      vnl_vector< double >  result = test( ada, r, testing_examples );
      out2 << first_part
           << r << "," << training_examples.size() << "," << testing_examples.size() << ","
           << result[tp_loc] << "," << result[tn_loc] << ","
           << result[fn_loc] << "," << result[fp_loc] << ","
           << (result[tp_loc]+result[tn_loc]) /
              (result[tp_loc]+result[tn_loc]+result[fn_loc]+result[fp_loc]) << ","
           << (result[tp_loc])/(result[tp_loc]+result[fn_loc]) << ","
           << (result[tn_loc])/(result[tn_loc]+result[fp_loc]) << ","
           << (result[tp_loc])/(result[tp_loc]+result[fp_loc]) << ","
           << (result[tn_loc])/(result[tn_loc]+result[fn_loc])
           << vcl_endl;
    }
  }
}

void five_fold_test( vcl_vector<vidtk::learner_training_data_sptr> const & data_set,
                     vcl_vector<vidtk::learner_training_data_sptr> const & extra_testing,
                     unsigned int run,
                     vcl_string prefix, vcl_ofstream & out )
{
  for(unsigned int i = 0; i < 5; ++i)
  {
    vcl_vector<vidtk::learner_training_data_sptr> training_examples;
    vcl_vector<vidtk::learner_training_data_sptr> testing_examples;
    for(unsigned int t = 0; t < data_set.size(); ++t)
    {
      if(t%5 == i)
      {
        testing_examples.push_back(data_set[t]);
      }
      else
      {
        training_examples.push_back(data_set[t]);
      }
    }
    testing_examples.insert(testing_examples.end(),extra_testing.begin(),extra_testing.end());
    vcl_cout << data_set.size() << " " <<  training_examples.size() << " " << testing_examples.size() << vcl_endl;
    vcl_string prefix_5fold;
    {
      vcl_stringstream ss;
      ss << prefix << "_" << run << "_" << i;
      ss >> prefix_5fold;
    }
    vcl_string temp;
    {
      vcl_stringstream ss;
      ss << run << "," << i << ",";
      ss >> temp;
    }
    train_and_test_output( training_examples, testing_examples, prefix_5fold, out, "run,fold,", temp );
  }
}

void output_set( vcl_vector<vidtk::learner_training_data_sptr> const & data_set,
                 vcl_string fname)
{
  vcl_ofstream out( fname.c_str() );
  for(unsigned int i = 0; i < data_set.size(); ++i)
  {
    out << *(data_set[i]) << vcl_endl;
  }
  out.close();
}

void run_five_fold_experiments( vcl_vector<vidtk::learner_training_data_sptr> training_examples_positive,
                                vcl_vector<vidtk::learner_training_data_sptr> training_examples_negative,
                                vcl_string prefix )
{
  vcl_string five_fold_tests_results_prefix = prefix + "_fivefold_results";
  vcl_ofstream five_fold_csv_file((five_fold_tests_results_prefix + ".csv").c_str());
  five_fold_csv_file << "run,fold,training set size,testing set size,tp,tn,fn,fp,"
                        "accuracy,sensitivity,specificity,"
                        "positive predictive value,Negative predictive value" << vcl_endl;
  srand ( unsigned ( 1000 ) );
  for( unsigned int i = 0; i < 10; ++i )
  {
    random_shuffle(training_examples_negative.begin(), training_examples_negative.end());
    random_shuffle(training_examples_positive.begin(), training_examples_positive.end());
    vcl_vector<vidtk::learner_training_data_sptr> used_set;
    vcl_vector<vidtk::learner_training_data_sptr> extra_testing_set;
    unsigned int size = vcl_min(training_examples_positive.size(),training_examples_negative.size());
    for( unsigned int j = 0; j < size; ++j )
    {
      used_set.push_back(training_examples_positive[j]);
      used_set.push_back(training_examples_negative[j]);
    }
    unsigned int extra = (training_examples_positive.size()-size)/5;
    vcl_cout << "postive extra: " << extra << vcl_endl;
    for( unsigned int j = 0; j < extra; ++j )
    {
      extra_testing_set.push_back(training_examples_positive[size+j]);
    }
    extra = (training_examples_negative.size()-size)/5;
    vcl_cout << "negative extra: " << extra << vcl_endl;
    for( unsigned int j = 0; j < extra; ++j )
    {
      extra_testing_set.push_back(training_examples_negative[size+j]);
    }
    random_shuffle(used_set.begin(), used_set.end());
//     vcl_cout << "\t";
//     for( unsigned int j = 0; j < used_set.size(); ++j )
//     {
//       vcl_cout << used_set[j]->label() << " ";
//     }
    {
      vcl_string fname;
      vcl_stringstream ss;
      ss << five_fold_tests_results_prefix << "_data_set_run_" << i << ".txt";
      ss >> fname;
      output_set( used_set, fname);
    }
    five_fold_test( used_set, extra_testing_set, i, five_fold_tests_results_prefix, five_fold_csv_file );
  }
}

struct input_dataset
{
  vcl_string name;
  double gsd;
  bool area_in_gsd_;
  bool loc_in_gsd_;
  vcl_vector<vidtk::learner_training_data_sptr> positive;
  vcl_vector<vidtk::learner_training_data_sptr> negative;
};

void leave_one_dataset_out_experiment(vcl_vector< input_dataset > input_set, vcl_string prefix)
{
  prefix = prefix + "_leave_one_out_";
  vcl_ofstream result((prefix + "result.csv").c_str());
  result << "run,left out set,training set size,testing set size,tp,tn,fn,fp,"
                          "accuracy,sensitivity,specificity"
                          ",positive predictive value,Negative predictive value" << vcl_endl;
  for( unsigned int lo = 0; lo < input_set.size(); ++lo )
  {
    vcl_string lo_prefix = prefix + input_set[lo].name;
    vcl_vector<vidtk::learner_training_data_sptr> training_positive;
    vcl_vector<vidtk::learner_training_data_sptr> training_negative;
    vcl_vector<vidtk::learner_training_data_sptr> testing_set;
    for( unsigned int i = 0; i < input_set.size(); ++i )
    {
      if(i == lo)
      {
        testing_set.insert(testing_set.end(), input_set[i].positive.begin(), input_set[i].positive.end());
        testing_set.insert(testing_set.end(), input_set[i].negative.begin(), input_set[i].negative.end());
      }
      else
      {
        training_positive.insert(training_positive.end(), input_set[i].positive.begin(), input_set[i].positive.end());
        training_negative.insert(training_negative.end(), input_set[i].negative.begin(), input_set[i].negative.end());
      }
    }
    for(unsigned int i = 0; i < 10; ++i)
    {
      random_shuffle(training_positive.begin(), training_positive.end());
      random_shuffle(training_negative.begin(), training_negative.end());
      vcl_vector<vidtk::learner_training_data_sptr> used_set;
      unsigned int size = vcl_min(training_positive.size(),training_negative.size());
      for( unsigned int j = 0; j < size; ++j )
      {
        used_set.push_back(training_positive[j]);
        used_set.push_back(training_negative[j]);
      }
      random_shuffle(used_set.begin(), used_set.end());
      vcl_string tmp_prefix;
      {
        vcl_stringstream ss;
        ss << prefix << "_" << i << "_" << input_set[lo].name;
        ss >> tmp_prefix;
      }
      vcl_string temp;
      {
        vcl_stringstream ss;
        ss << i << "," << input_set[lo].name << ",";
        ss >> temp;
      }
      train_and_test_output( used_set, testing_set, tmp_prefix, result, "run,left out set", temp );
    }
  }
}

void leave_two_dataset_out_experiment(vcl_vector< input_dataset > input_set, vcl_string prefix)
{
  prefix = prefix + "_leave_two_out_";
  vcl_ofstream result((prefix + "result.csv").c_str());
  result << "run,left out set,training set size,testing set size,tp,tn,fn,fp,"
                          "accuracy,sensitivity,specificity"
                          ",positive predictive value,Negative predictive value" << vcl_endl;
  for( unsigned int lo = 0; lo < input_set.size(); ++lo )
  {
    for( unsigned int lo2 = lo+1; lo2 < input_set.size(); ++lo2 )
    {
      vcl_string lo_prefix = prefix + input_set[lo].name + "_" + input_set[lo2].name;
      vcl_vector<vidtk::learner_training_data_sptr> training_positive;
      vcl_vector<vidtk::learner_training_data_sptr> training_negative;
      vcl_vector<vidtk::learner_training_data_sptr> testing_set;
      for( unsigned int i = 0; i < input_set.size(); ++i )
      {
        if(i == lo || i == lo2 )
        {
          testing_set.insert(testing_set.end(), input_set[i].positive.begin(), input_set[i].positive.end());
          testing_set.insert(testing_set.end(), input_set[i].negative.begin(), input_set[i].negative.end());
        }
        else
        {
          training_positive.insert(training_positive.end(), input_set[i].positive.begin(), input_set[i].positive.end());
          training_negative.insert(training_negative.end(), input_set[i].negative.begin(), input_set[i].negative.end());
        }
      }
      for(unsigned int i = 0; i < 10; ++i)
      {
        random_shuffle(training_positive.begin(), training_positive.end());
        random_shuffle(training_negative.begin(), training_negative.end());
        vcl_vector<vidtk::learner_training_data_sptr> used_set;
        unsigned int size = vcl_min(training_positive.size(),training_negative.size());
        for( unsigned int j = 0; j < size; ++j )
        {
          used_set.push_back(training_positive[j]);
          used_set.push_back(training_negative[j]);
        }
        random_shuffle(used_set.begin(), used_set.end());
        vcl_string tmp_prefix;
        {
          vcl_stringstream ss;
          ss << prefix << "_" << i << "_" << input_set[lo].name + "_" + input_set[lo2].name;
          ss >> tmp_prefix;
        }
        vcl_string temp;
        {
          vcl_stringstream ss;
          ss << i << "," << input_set[lo].name + "_" + input_set[lo2].name << ",";
          ss >> temp;
        }
        train_and_test_output( used_set, testing_set, tmp_prefix, result, "run,left out set", temp );
      }
    }
  }
}

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

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> trainingPeopleArg( "--input_file",
                                         "Test input file (pos_fname neg_fname gsd "
                                         "loc_in_gsd(0 or 1) area_in_gsd(0 or 1))" );
  vul_arg<vcl_string> outprefixArg("--outprefix", "The prefix of output of classifying tracks", "out");
  vul_arg_parse( argc, argv );

  if(!trainingPeopleArg.set())
  {
    vcl_cerr << "NEED INPUT" << vcl_endl;
    return 0;
  }
  vcl_ifstream in(trainingPeopleArg().c_str());
  vcl_vector< input_dataset > input_set;
  while(in)
  {
    input_dataset input;
    vcl_string pos, neg;
    in >> input.name;
    if(in)
    {
      in >> pos >> neg >> input.gsd >> input.loc_in_gsd_ >> input.area_in_gsd_;
    }
    else
    {
      break;
    }
    vcl_vector< vidtk::track_sptr > tracks_pos, tracks_neg;
    vcl_cout << pos << vcl_endl;
    read_tracks_vidtk(pos, tracks_pos);
    vcl_cout << "\t" << tracks_pos.size() << vcl_endl;
    vcl_cout << neg << vcl_endl;
    read_tracks_vidtk(neg, tracks_neg);
    vcl_cout << "\t" << tracks_neg.size() << vcl_endl;
    vidtk::track_to_vector_of_descriptors ttvod(input.gsd,input.loc_in_gsd_,input.area_in_gsd_);
    for(unsigned int i = 0; i < tracks_pos.size(); ++i)
    {
      input.positive.push_back(ttvod.generateTrainingData( tracks_pos[i], 1 ));
    }
    for(unsigned int i = 0; i < tracks_neg.size(); ++i)
    {
      input.negative.push_back(ttvod.generateTrainingData( tracks_neg[i], -1 ));
    }
    input_set.push_back(input);
  }
  vcl_cout << input_set.size() << vcl_endl;
  vcl_vector<vidtk::learner_training_data_sptr> training_examples_positive;
  vcl_vector<vidtk::learner_training_data_sptr> training_examples_negative;
  for( unsigned int i = 0; i < input_set.size(); ++i )
  {
    training_examples_positive.insert( training_examples_positive.end(),
                                       input_set[i].positive.begin(),
                                       input_set[i].positive.end());
    training_examples_negative.insert( training_examples_negative.end(),
                                       input_set[i].negative.begin(),
                                       input_set[i].negative.end());
  }
  vcl_cout << training_examples_positive.size() << " " << training_examples_negative.size() << vcl_endl;
  run_five_fold_experiments( training_examples_positive, training_examples_negative,
                             outprefixArg() );
  leave_one_dataset_out_experiment(input_set, outprefixArg());
  leave_two_dataset_out_experiment(input_set, outprefixArg());

  return 0;
}

