/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_arg.h>
#include <vcl_string.h>
#include <vcl_fstream.h>

#include <tracking/track.h>
#include <vpgl/algo/vpgl_fm_compute_8_point.h>


vcl_vector< vidtk::track_sptr >
load_tracks(const vcl_string& filename)
{
  vcl_vector< vidtk::track_sptr > tracks;
  vcl_ifstream ifs(filename.c_str());
  if (!ifs)
  {
    return tracks;
  }

  unsigned last_id=0;

  vidtk::track_sptr track = NULL;
  unsigned track_id, count, frame_num, is_good;
  while (ifs >> track_id)
  {
    // make a new track if needed
    if (!track || track_id != last_id)
    {
      track = new vidtk::track;
      track->set_id(track_id);
      tracks.push_back(track);
    }
    last_id = track_id;

    vidtk::track_state_sptr state = new vidtk::track_state;
    track->add_state(state);

    ifs >> count >> frame_num;
    state->time_.set_frame_number(frame_num);
    ifs >> state->loc_(0) >> state->loc_(1);
    ifs >> state->vel_(0) >> state->vel_(1);
    ifs >> is_good; // ignore this value
  }

  return tracks;
}


/// compute the fundamental matrix between these frames
vpgl_fundamental_matrix<double>
compute_F_matrix(const vcl_vector<vidtk::track_sptr>& tracks,
                 const vcl_vector<unsigned>& pt_indices,
                 unsigned frame1, unsigned frame2)
{
  vcl_vector<vgl_homg_point_2d<double> > pr, pl;

  for(vcl_vector<unsigned>::const_iterator i = pt_indices.begin();
      i != pt_indices.end();  ++i )
  {
    const vcl_vector<vidtk::track_state_sptr>& hist = tracks[*i]->history();
    const vnl_vector_fixed<double,3>& p1 = hist[frame1]->loc_;
    const vnl_vector_fixed<double,3>& p2 = hist[frame2]->loc_;
    pr.push_back(vgl_homg_point_2d<double>(p1[0],p1[1]));
    pl.push_back(vgl_homg_point_2d<double>(p2[0],p2[1]));
  }

  vpgl_fundamental_matrix<double> f_matrix;
  vpgl_fm_compute_8_point fm8;
  fm8.compute(pr, pl, f_matrix);
  return f_matrix;
}



int main(int argc, char *argv[])
{
  vul_arg<vcl_string> input_tracks( "--input-tracks", "input file containing KLT point tracks", "" );
  vul_arg<vcl_string> output_config( "--output-matrices", "output file to write fundamental matrices", "" );

  vul_arg_parse( argc, argv );

  if (!input_tracks.set())
  {
    vcl_cout << "An input track file is required.  "
             << "Run with -? for details."<<vcl_endl;
    return -1;
  }

  vcl_vector< vidtk::track_sptr > tracks = load_tracks(input_tracks());

  vcl_cout << "loaded "<<tracks.size() << " tracks"<<vcl_endl;

  unsigned int max_length = 0;
  for(unsigned i=0; i<tracks.size(); ++i)
  {
    if (max_length < tracks[i]->history().size())
    {
      max_length = tracks[i]->history().size();
    }
  }

  vcl_cout << "maximum number of frames is "<< max_length <<vcl_endl;

  vcl_vector<unsigned> pt_indices;
  for(unsigned i=0; i<tracks.size(); ++i)
  {
    if (max_length == tracks[i]->history().size())
    {
      pt_indices.push_back(i);
    }
  }

  vcl_cout << "There are "<<pt_indices.size()
           <<" tracks with "<<max_length<<" frames"<<vcl_endl;


  vcl_vector<vpgl_fundamental_matrix<double> > f_matrices;
  for (unsigned int i=1; i<max_length; ++i)
  {
    f_matrices.push_back(compute_F_matrix(tracks, pt_indices, 0 , i));
    vcl_cout << "F matrix "<<i<<":\n"<<f_matrices.back()<<vcl_endl;
  }


  return 0;
}

