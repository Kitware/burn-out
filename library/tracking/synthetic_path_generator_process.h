/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef synthetic_path_generator_process_h
#define synthetic_path_generator_process_h

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/image_object.h>

#include <vcl_vector.h>

#include <vnl/vnl_double_2.h>
#include <vnl/vnl_random.h>

#include <vgl/algo/vgl_h_matrix_2d.h>

#include <utilities/timestamp.h>

namespace vidtk
{
///This process generates objects for the templated path function
template < class PathFunction >
class synthetic_path_generator_process
  : public process
{
  public:
    typedef synthetic_path_generator_process self_type;
    synthetic_path_generator_process( vcl_string const& name );

    ~synthetic_path_generator_process();

    virtual config_block params() const;

    virtual bool set_params( config_block const& );

    virtual bool initialize();

    virtual bool step();

    bool next_loc(vnl_double_2 & l);

    unsigned int ni() const
    { return image_.ni(); }
    unsigned int nj() const
    { return image_.nj(); }

    vcl_vector< image_object_sptr > const& objects() const;
    VIDTK_OUTPUT_PORT( vcl_vector< image_object_sptr > const&, objects );

    vidtk::timestamp timestamp() const;
    VIDTK_OUTPUT_PORT( vidtk::timestamp, timestamp );

    vil_image_view<vxl_byte> const& image();
    VIDTK_OUTPUT_PORT( vil_image_view<vxl_byte> const&, image );

    vgl_h_matrix_2d<double> const& image_to_world_homography() const;
    VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, image_to_world_homography );

    vgl_h_matrix_2d<double> const& world_to_image_homography() const;
    VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double> const&, world_to_image_homography );

    double world_units_per_pixel( ) const
    { return 1.0; }
    VIDTK_OUTPUT_PORT( double, world_units_per_pixel );

    bool current_correct_location(vnl_double_2 & l) const;
    bool current_noisy_location(vnl_double_2 & l) const;

    void set_std_dev(double s)
    { std_dev_ = s; }

    void set_number_of_generations(unsigned int nog)
    {
      number_of_time_steps_ = nog;
    }
    unsigned int get_number_of_generations() const
    {
      return number_of_time_steps_;
    }
    void set_frame_rate(double fr)
    {
      frame_rate_set_ = true;
      frame_rate_ = fr;
    }

    vnl_double_2 get_current_velocity() const;
    double get_current_speed() const;
  protected:
    config_block config_;

    vnl_random rand_;

    unsigned int time_step_;
    unsigned int number_of_time_steps_;
    unsigned int current_time_;
    double std_dev_;
    PathFunction path_;
    vcl_vector< image_object_sptr > objects_;
    vil_image_view<vxl_byte> image_;

    vgl_h_matrix_2d< double > identity_;
    vnl_double_2 current_noisy_location_;
    vnl_double_2 current_correct_location_;

    bool frame_rate_set_;
    double frame_rate_;
};
} //namespace vidtk

#endif
