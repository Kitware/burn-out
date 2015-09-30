/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_arg.h>
#include <vul/vul_file.h>
#include <vul/vul_sprintf.h>

#include <vnl/vnl_math.h>
#include <vnl/vnl_vector.h>

#include <vil/vil_rgb.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>

#include <vcl_sstream.h>
#include <vcl_algorithm.h>

#include <tracking/vsl_reader.h>
#include <tracking/kw18_reader.h>

#include <vcl_string.h>
#include <vcl_vector.h>
#include <vcl_fstream.h>

#include <tracking/track.h>

#include <vsl/vsl_binary_io.h>

struct stats
{
  stats() : histogram(100,0), max(-1), min(1e23), ave(0), count(0){}
  vnl_vector<double> histogram;
  double max;
  double min;
  double ave;
  double count;
  unsigned int number_of_bins() const
  {
    return histogram.size();
  }
  void update(double length)
  {
    ave += length;
    count++;
    int at = static_cast<int>( (length/max*(double)number_of_bins()) );
    if(at >= static_cast<int>( number_of_bins() ))
    {
      at = number_of_bins()-1;
    }
    histogram[at]++;
  }
  double get_ave() const
  {
    return ave/count;
  }
  void output_hist(vcl_ofstream & out)
  {
    double binsize =  max/number_of_bins();
    for ( unsigned int i = 0; i < number_of_bins(); ++i )
    {
         out << binsize*(i+0.5) << "," << histogram[i] << vcl_endl;
    }
  }
};

double sd(vcl_vector<double> values, stats & s)
{
  double mean = s.get_ave();
  double sum = 0;
  for(unsigned int i = 0; i < values.size(); ++i)
  {
    sum += vnl_math::sqr(values[i]-mean);
  }
  sum = sum / values.size();
  return vcl_sqrt(sum);
}

int compute_states(vcl_vector<double> & sizes, vcl_string prefix)
{
  stats s;
  s.max = *vcl_max_element(sizes.begin(), sizes.end());
  s.min = *vcl_min_element(sizes.begin(), sizes.end());

  for(unsigned int i = 0; i < sizes.size(); ++i)
  {
    s.update(sizes[i]);
  }

  vcl_string fname = prefix + "_stats.txt";
  vcl_ofstream out(fname.c_str());
  out << "NUMBER OF TRACKS:                     " << sizes.size() << vcl_endl;
  out << "AVERAGE TRACK DURATION:               " << s.get_ave() << vcl_endl;
  out << "standard deviation of track duration: " << sd(sizes,s) << vcl_endl;
  out << "longest track:                        " << s.max << vcl_endl;
  out << "shortest track:                       " << s.min << vcl_endl;
  out.close();

  fname = prefix + "_histo.csv";
  {
    vcl_ofstream out(fname.c_str());
    s.output_hist(out);
    out.close();
  }
  for(unsigned int i = 0; i < sizes.size(); ++i)
  {
    if(sizes[i] == s.max) return i;
  }
  // should never get here (?)
  assert( true );
  return -1;
}

void getStartStopFrame(vidtk::track_sptr trk, unsigned int & begin, unsigned int & end)
{
  vcl_vector< vidtk::track_state_sptr > const& hist = trk->history();
  begin = hist[0]->time_.frame_number();
  end = hist[hist.size()-1]->time_.frame_number();
}

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> trackBinaryVSLArg( "--binary-vsl", "use vidtk binary format");
  vul_arg<vcl_string> trackTextArg( "--tracks-kw18", "tracks kw18" );
  vul_arg<vcl_string> manual_labels( "--manual-labels", "A file with the track number and its label");
  vul_arg<vcl_string> manual_labels_2( "--manual-labels-2", "A file with the track number and its label");
  vul_arg<vcl_string> outputPrefixArg( "--out-prefix", "Output prefix", "out" );
  vul_arg<vcl_string> startsArg("--starts", "VSL track sections labeled as starts");
  vul_arg<vcl_string> stopArg("--stops", "VSL track sections label as stops");
  vul_arg_parse( argc, argv );

  /// read the tracks
  vcl_vector< vidtk::track_sptr > tracks;
  if(trackBinaryVSLArg.set())
  {
    vidtk::vsl_reader reader;
    reader.set_filename(trackBinaryVSLArg().c_str());
    vcl_vector< vidtk::track_sptr > tmp;
    while(reader.read(tmp))
    {
      tracks.insert(tracks.end(), tmp.begin(), tmp.end());
    }
  }
  if(trackTextArg.set())
  {
    vidtk::kw18_reader reader;
    reader.set_filename(trackTextArg().c_str());
    vcl_vector< vidtk::track_sptr > tmp;
    reader.read(tmp);
    tracks.insert(tracks.end(), tmp.begin(), tmp.end());
  }

  vcl_map< unsigned int, unsigned int > track_begin_frames;
  vcl_map< unsigned int, unsigned int > track_end_frames;

  if(manual_labels.set() && manual_labels_2.set())
  {
    vcl_map< unsigned int, unsigned int > labeled;
    vcl_ifstream in(manual_labels().c_str());
    while(in){
      unsigned int id, label;
      in >> id >> label;
      labeled[id] = label;
    }
    in.close();
    in.open(manual_labels_2().c_str());
    while(in){
      unsigned int id, label;
      in >> id >> label;
      if(labeled[id]!=1) {
        labeled[id] = label;
      }
    }
    vcl_vector<double> sizes_good;
    vcl_vector<unsigned int> good_track_ids;
    vcl_vector<double> sizes_bad;
    for(unsigned int j = 0; j < tracks.size(); ++j)
    {
      unsigned int id = tracks[j]->id();
      unsigned int begin,end;
      getStartStopFrame(tracks[j], begin, end);
      track_begin_frames[id] = begin;
      track_end_frames[id] = end;
      vcl_map< unsigned int, unsigned int >::const_iterator iter = labeled.find(tracks[j]->id());
      if(iter == labeled.end())
      {
        continue;
      }
      else if(iter->second == 1)
      {
        good_track_ids.push_back(tracks[j]->id());
        sizes_good.push_back((tracks[j]->history()).size());
      }
      else
      {
        sizes_bad.push_back((tracks[j]->history()).size());
      }
    }
//     unsigned int g = compute_states(sizes_good, outputPrefixArg() + "_vechical");
//     vcl_cout << good_track_ids[g] << vcl_endl;
//     for(unsigned int i = 0; i < sizes_good.size(); ++i )
//     {
//       vcl_cout << good_track_ids[i] << "\t" << sizes_good[i] << vcl_endl;
//     }
    compute_states(sizes_bad, outputPrefixArg() + "_other");
  }
  else if(manual_labels.set())
  {
    vcl_map< unsigned int, unsigned int > labeled;
    vcl_ifstream in(manual_labels().c_str());
    while(in){
      unsigned int id, label;
      in >> id >> label;
      labeled[id] = label;
    }
    vcl_vector<double> sizes_good;
    vcl_vector<unsigned int> good_track_ids;
    vcl_vector<double> sizes_bad;
    for(unsigned int j = 0; j < tracks.size(); ++j)
    {
      unsigned int id = tracks[j]->id();
      unsigned int begin,end;
      getStartStopFrame(tracks[j], begin, end);
      track_begin_frames[id] = begin;
      track_end_frames[id] = end;
      vcl_map< unsigned int, unsigned int >::const_iterator iter = labeled.find(id);
      if(iter == labeled.end())
      {
        continue;
      }
      else if(iter->second == 1)
      {
        good_track_ids.push_back(tracks[j]->id());
        sizes_good.push_back((tracks[j]->history()).size());
      }
      else
      {
        sizes_bad.push_back((tracks[j]->history()).size());
      }
    }
    unsigned int g = compute_states(sizes_good, outputPrefixArg() + "_vechical");
    vcl_cout << good_track_ids[g] << vcl_endl;
    for(unsigned int i = 0; i < sizes_good.size(); ++i )
    {
      vcl_cout << good_track_ids[i] << "\t" << sizes_good[i] << vcl_endl;
    }
    compute_states(sizes_bad, outputPrefixArg() + "_other");
  }
  else
  {
    vcl_vector<double> sizes;
    for(unsigned int j = 0; j < tracks.size(); ++j)
    {
      unsigned int id = tracks[j]->id();
      unsigned int begin,end;
      getStartStopFrame(tracks[j], begin, end);
      track_begin_frames[id] = begin;
      track_end_frames[id] = end;
      sizes.push_back((tracks[j]->history()).size());
    }
    compute_states(sizes, outputPrefixArg());
  }
  unsigned int start_end_point_count = 0;
  unsigned int stop_end_point_count = 0;
  unsigned int starts_at_endings = 0;
  unsigned int stops_at_beginings = 0;
  unsigned int start_count = 0;
  unsigned int stop_count = 0;
  unsigned int begin_end_points = track_begin_frames.size();
  unsigned int end_end_points = track_end_frames.size();
  if(startsArg.set())
  {
    vidtk::vsl_reader * reader_starts = NULL;
    reader_starts = new vidtk::vsl_reader;
    reader_starts->set_filename(startsArg().c_str());
    vcl_vector< vidtk::track_sptr > tracks;
    while(reader_starts->read(tracks)){
      start_count = tracks.size();
      for(unsigned int j = 0; j < tracks.size(); ++j)
      {
//         unsigned int id = tracks[j]->id();
        unsigned int begin,end;
        if(tracks[j]->history().size()==0)
        {
          start_count--;
          continue;
        }
        getStartStopFrame(tracks[j], begin, end);
        vcl_map< unsigned int, unsigned int >::const_iterator iter = track_begin_frames.find(tracks[j]->id());
        assert( iter != track_begin_frames.end() );
        if(begin <= iter->second && iter->second <= end)
        {
          start_end_point_count++;
        }
        vcl_map< unsigned int, unsigned int >::const_iterator iter2 = track_end_frames.find(tracks[j]->id());
        assert( iter2 != track_end_frames.end() );
        if(begin <= iter2->second && iter2->second <= end)
        {
          starts_at_endings++;
        }
      }
    }
    vcl_cout << "There are " << begin_end_points << " beginings of tracks." << vcl_endl
             << "There are " << start_count << " starts detected." << vcl_endl
             << "   of these " << start_end_point_count << " are detected at beginings of tracks." << vcl_endl
             << "   of these " << starts_at_endings << " starts at the ends of tracks." << vcl_endl;
  }

  if(stopArg.set())
  {
    vidtk::vsl_reader * reader_stops = NULL;
    reader_stops = new vidtk::vsl_reader;
    reader_stops->set_filename(stopArg().c_str());
    vcl_vector< vidtk::track_sptr > tracks;
    while(reader_stops->read(tracks))
    {
      stop_count = tracks.size();
      for(unsigned int j = 0; j < tracks.size(); ++j)
      {
//         unsigned int id = tracks[j]->id();
        unsigned int begin,end;
        getStartStopFrame(tracks[j], begin, end);
        vcl_map< unsigned int, unsigned int >::const_iterator iter = track_end_frames.find(tracks[j]->id());
        assert( iter != track_end_frames.end() );
        if(begin <= iter->second && iter->second <= end)
        {
          stop_end_point_count++;
        }
        vcl_map< unsigned int, unsigned int >::const_iterator iter2 = track_begin_frames.find(tracks[j]->id());
        assert( iter2 != track_begin_frames.end() );
        if(begin <= iter2->second && iter2->second <= end)
        {
          stops_at_beginings++;
        }
      }
    }
    vcl_cout << "There are " << end_end_points << " endings of tracks." << vcl_endl
             << "There are " << stop_count << " stops detected." << vcl_endl
             << "   of these " << stop_end_point_count << " are detected at endings of tracks." << vcl_endl
             << "   of these " << stops_at_beginings << " are stops at begings of tracks." << vcl_endl;
  }

  return EXIT_SUCCESS;
}
