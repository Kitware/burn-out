#include "interpolate_gps_track_state_process.h"

namespace vidtk
{

interpolate_gps_track_state_process
::interpolate_gps_track_state_process( const vcl_string& name )
  : process( name, "interpolate_gps_track_state_process")
{
  reset();
}

interpolate_gps_track_state_process
::~interpolate_gps_track_state_process()
{

}

config_block
interpolate_gps_track_state_process
::params() const
{
  config_block r;
  return r;
}

bool
interpolate_gps_track_state_process
::reset()
{
  tracks = NULL;
  output_tracks.clear();
  return true;
}

bool
interpolate_gps_track_state_process
::set_params( const config_block& /*blk*/ )
{
  return true;
}

bool
interpolate_gps_track_state_process
::initialize()
{
  return reset();
}

bool
interpolate_gps_track_state_process
::step()
{
  if(!tracks)
    return false;
  output_tracks.clear();
  for( unsigned int i = 0; i < tracks->size(); ++i )
  {
    track_sptr nt = new track();
    track_sptr ct = (*tracks)[i];
    nt->set_id(ct->id());
    unsigned int j = 0;
    vcl_vector< track_state_sptr > const& cthist = ct->history();
    vnl_vector_fixed<double,3> vel;
    for(; j < cthist.size()-1; ++j)
    {
      double d = cthist[j+1]->time_.diff_in_secs( cthist[j]->time_ );
      vel = (cthist[j+1]->loc_-cthist[j]->loc_)/d;
      cthist[j]->vel_ = vel;
    }
    cthist[j]->vel_ = vel;
    for(j = 0; j < cthist.size()-1; ++j)
    {
      track_state_sptr ctsj = cthist[j];
      track_state_sptr ctsjp1 = cthist[j+1];
      nt->add_state(ctsj);
      double d = ctsjp1->time_.diff_in_secs( ctsj->time_ );
      double f = ctsjp1->time_.frame_number() - ctsj->time_.frame_number() -1;
      double ts = d/(f+1);
      for(double k = 0; k < f; ++k)
      {
        //track_state_sptr ls = nt->last_state();
        track_state_sptr ns = new track_state();
        double t = (k+1)/(f+1);
        double ctime = (k+1)*ts;
        //ns->time_ = timestamp( (ls->time_.time_in_secs()+ts)*1e6, ls->time_.frame_number()+1 );
        ns->time_ = timestamp( (ctsj->time_.time_in_secs()+ctime)*1e6, ctsj->time_.frame_number()+1+k );
        ns->loc_ = (ctsj->loc_ + ctime*ctsj->vel_)*(1-t) + (ctsjp1->loc_ + (ctime-d)*ctsjp1->vel_)*(t); //ls->loc_ + ts*ls->vel_;
        ns->vel_ = ctsj->vel_*(1-t)+ctsjp1->vel_*t;
        image_object_sptr io = new image_object();
        unsigned int pt1[2], pt2[2];
        pt1[0] = ns->loc_[0]-5;
        pt1[1] = ns->loc_[1]+5;
        pt2[0] = ns->loc_[0]-5;
        pt2[1] = ns->loc_[1]+5;
        io->bbox_ = vgl_box_2d<unsigned>(pt1, pt2);
        io->img_loc_ = vnl_vector_fixed<double,2>(ns->loc_[0],ns->loc_[1]);
        io->world_loc_ = vnl_vector_fixed<double,3>(ns->loc_[0],ns->loc_[1],0);
        assert(!io->bbox_.is_empty());
        ns->set_image_object( io );
        nt->add_state(ns);
      }
//       if(ctsj->loc_!=ctsjp1->loc_)
//       vcl_cout << "TEST\n\t"<< ctsj->loc_ << "\t" << ctsjp1->loc_
//                << "\n\t" << ctsjp1->loc_-ctsjp1->vel_*d << "\t" << ctsj->loc_+ctsj->vel_*d
//                << "\n\t" << ctsj->vel_ << "\t" << ctsjp1->vel_
//                << "\n\t" << ctsj->loc_-ctsjp1->loc_-ctsjp1->vel_*d << "\t" << ctsjp1->loc_-ctsj->loc_+ctsj->vel_*d
//                /*<< "\n\t" << (ctsjp1->loc_-ctsj->loc_)/d << " --> " << ((ctsjp1->loc_-ctsj->loc_)/d)*.5 */ << vcl_endl;
    }
    nt->add_state(cthist[j]);
    {
      vcl_vector< track_state_sptr > const& nthist = nt->history();
      for(unsigned int j = 0; j < nthist.size()-1; ++j)
      {
        track_state_sptr ctsj = nthist[j];
        track_state_sptr ctsjp1 = nthist[j+1];
        assert(ctsj);
        assert(ctsjp1);
        assert(ctsj->time_<ctsjp1->time_);
      }
    }
    output_tracks.push_back(nt);
  }
  tracks = NULL;
  return true;
}

void
interpolate_gps_track_state_process
::set_input_tracks( const vcl_vector<track_sptr> & tl )
{
  tracks = & tl;
}

const vcl_vector<track_sptr> &
interpolate_gps_track_state_process
::output_track_list() const
{
  return output_tracks;
}


}
