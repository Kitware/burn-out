/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_track_reader_process_h_
#define vidtk_track_reader_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <tracking/track.h>
#include <tracking/track_reader.h>

/// This process reads entire tracks and as such, it is suitable for reading
/// terminated tracks but not for active tracks
namespace vidtk
{
  class track_reader_process
    : public process
    {
    public:
      typedef track_reader_process self_type;
      track_reader_process(vcl_string const& name);
      ~track_reader_process();

      virtual config_block params() const;
      virtual bool set_params( config_block const& );
      virtual bool initialize();
      virtual bool step();

      void set_filename(vcl_string const& name);
      VIDTK_INPUT_PORT ( set_filename, vcl_string const&);

      vcl_vector<track_sptr> const& tracks() const;

      VIDTK_OUTPUT_PORT(vcl_vector <track_sptr> const&, tracks);

      bool is_disabled() const;

      // batch reading mode
      bool is_batch_mode() const;
      void set_batch_mode( bool batch_mode );

    private:
      vcl_string filename_;
      config_block config_;
      unsigned format_;
      bool disabled_;
      bool ignore_occlusions_;
      bool ignore_partial_occlusions_;
      vcl_string path_to_images_; /*KW18 Only*/
      double frequency_; /*MIT Only for now*/

      vcl_vector<track_sptr> tracks_;
      track_reader* reader_;

      // Default false: read tracks in steps. True: read all tracks together.
      bool batch_mode_;

    };
}
#endif

