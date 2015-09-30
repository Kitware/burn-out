//circular_features_test.cxx

#include <classifier/track_to_vector_of_descriptors.h>

#include <learning/learner_data_class_vector.h>

#include <descriptors/calculate_descriptors.h>
#include <descriptors/function_calcuate_circle.h>
#include <descriptors/kwsoti_simple_stats_functions.h>
#include <event_detectors/kwsoti2_process.h>

#include <vul/vul_arg.h>
#include <vul/vul_awk.h>
#include <vul/vul_string.h>

#include <tracking/track_reader_process.h>

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_sstream.h>
#include <vcl_iomanip.h>
#include <vcl_fstream.h>

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
  vul_arg<vcl_string> trainingPeopleArg( "--input_file", "Test input file" );
  vul_arg<vcl_string> outprefixArg("--output", "The output", "out.txt");
  vul_arg_parse( argc, argv );

  if(!trainingPeopleArg.set())
  {
    vcl_cerr << "NEED INPUT" << vcl_endl;
    return 0;
  }
  vcl_ifstream in(trainingPeopleArg().c_str());
  vcl_ofstream out(outprefixArg().c_str());
  out << "track id,uid,type,radius,SumCirleArcLength,Circumference,"
         "ErrorSTD,ErrorMean,getErrorMedian,ErrorMin,ErrorMax,"
         "LengthRatio,PercentageLengthInside,PercentageLengthOutside,"
         "ErrorMidpointSTD,ErrorMidpointMean,ErrorMidpointMedian,ErrorMidpointMin,"
         "MidpointMax,Non Norm Gt1,Non Norm Lt -1,ratio sum arc length with circumference" << vcl_endl;
  int uid = 0;
  while(in)
  {
    vcl_vector< vidtk::track_sptr > tracks_pos;
    vcl_vector< vidtk::track_sptr > tracks_neg;
    vcl_map< unsigned int, unsigned int > id_to_index;
    vcl_string tracks_name, desired_ones;
    double gsd;
    //What is on each line in the input file
    in >> tracks_name >> desired_ones >> gsd;
    if(!in || tracks_name == "")
      break;
    vcl_cout << tracks_name << " " << gsd << vcl_endl;
    vcl_vector< vidtk::track_sptr > tracks;
    read_tracks_vidtk(tracks_name, tracks);
    for( unsigned int i = 0; i < tracks.size(); ++i )
    {
      id_to_index[tracks[i]->id()] = i;
    }
    vcl_ifstream probStream( desired_ones.c_str() );
    if ( !probStream )
    {
      vcl_cerr << "Couldn't read probability file " << desired_ones << ";\n Exiting\n";
      return EXIT_FAILURE;
    }
    vul_awk awk2(probStream);
    for( ; awk2; ++awk2)
    {
      unsigned int id = static_cast<unsigned int>(vul_string_atoi(awk2[0]));
      //int classification = vul_string_atoi(awk2[1]);
      int l = vul_string_atoi(awk2[1]);
      if( l == 0 )
        continue;
      else
      {
        out << id << "," << uid <<","<< l << ",";
        vcl_vector<vidtk::KWSOTI2FrameData> track_kwsoti_fd;
        track_to_KWSOTI2FrameData( tracks[id_to_index[id]], track_kwsoti_fd, gsd );
        vidtk::function_fit_circle fun( gsd );
        vidtk::compute_descriptor( track_kwsoti_fd, fun );
        vidtk::function_calculate_descriptor & tf = fun;
        vnl_vector<double> d = tf.get_value();
        for(unsigned int j = 0; j < fun.size(); ++j)
        {
          out << d[j] << ",";
        }
        out << d[1]/d[2] << vcl_endl;
        {
          vcl_stringstream ss;
          ss << "circle_" << id << "_" << uid << ".txt";
          vcl_string fname;
          ss >> fname;
          vcl_ofstream fout(fname.c_str());
          fout << fun.getLocation() << " " << fun.getRadius() << vcl_endl;
        }
        {
          vcl_stringstream ss;
          ss << "path_" << id << "_" << uid++ << ".txt";
          vcl_string fname;
          ss >> fname;
          vcl_ofstream fout(fname.c_str());
          for( unsigned int i = 0; i < track_kwsoti_fd.size(); ++i )
          {
            fout << track_kwsoti_fd[i].pos.X << " " << track_kwsoti_fd[i].pos.Y << vcl_endl;
          }
        }
      }
    }
  }

  return 0;
}


