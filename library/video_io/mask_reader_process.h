/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef kw_MASK_READER_PROCESS_H_
#define kw_MASK_READER_PROCESS_H_


#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/config_block.h>

#include <vil/vil_image_view.h>

namespace vidtk  {

// ----------------------------------------------------------------
/**
 * @brief Read mask image.
 *
 * This process reads a single mask image and fails if the mask can
 * not be found. The mask image is converted into a boolean image and
 * provided to the output.
 *
 * Previous implementations used the generic image reader, which would
 * not generate an error if the mask image was not found. Also, more
 * configuration parameters were necessary than one would expect were
 * needed to read a single mask image.
 */
class mask_reader_process
: public process
{
public:
  typedef mask_reader_process self_type;

  mask_reader_process(std::string const& name );
  virtual ~mask_reader_process();

  // -- process interface --
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool step();

  // -- process outputs --
  VIDTK_OUTPUT_PORT ( vil_image_view< bool >, mask_image );
  vil_image_view< bool > mask_image() const;

private:
  config_block config_;
  bool disabled_;

  vil_image_view< bool > mask_image_;
}; // end class mask_reader_process

} // end namespace

#endif /* kw_MASK_READER_PROCESS_H_ */
