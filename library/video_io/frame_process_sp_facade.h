/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef vidtk_frame_process_sp_facade_h_
#define vidtk_frame_process_sp_facade_h_

#include <video_io/frame_process.h>
#include <video_io/frame_process_sp_facade_interface.h>


namespace vidtk
{

/// \brief Generic wrapper that allows the use of a super_process as a
/// frame_process via facade.
///
/// This facade tempalate makes use of a super_process class implementing the
/// frame_process_sp_facade_interface abstract interface. We require that
/// \a spProcess be a class that inherits from frame_process_sp_facade_interface
/// \e and super_process, both of which are complilation enforced
/// here.
///
/// A frame_process_sp_facade is properly instantiated like the following:
/// \code
///     frame_process_sp_facade< PixType, SpClass >( "name" );
/// \endcode
/// where \a PixType is the frame process pixel type and \a SpClass is the
/// signature of a valid class, including template types if needed.
///
/// A more specific example, using filename_frame_metadata2_super_process, would
/// look like:
/// \code
///     frame_process_sp_facade< bool, filename_frame_metadata2_super_process< bool > >( "node" );
/// \endcode
/// Note the template definition included with
/// filename_frame_metadata2_super_process ("< bool >").
///
template< class PixType, class spProcess >
class frame_process_sp_facade
  : public frame_process<PixType>
{
public:
  typedef frame_process_sp_facade_interface<PixType> fpspf_interface_t;

  frame_process_sp_facade( std::string const& _name )
    : frame_process<PixType>( _name, "frame_process_sp_facade" )
    , sp_impl_( new spProcess( _name ) )
  {
    if (0) // optimized out to not affect run-time performance
    {
      // enforce that the spProcess inherits from super_process as well as the
      // required interface.
      //
      // If one or both of these checks fail, you will receive an error at
      // compilation time, saying something along the lines that it
      // "cannot convert ..." between the type given to one of these two types
      // "at instantiation."
      super_process     *check_1 = sp_impl_.ptr(); (void) check_1;
      fpspf_interface_t *check_2 = sp_impl_.ptr(); (void) check_2;
    }
  }

  virtual ~frame_process_sp_facade(){}


  //////////////////////////////////////////////////////////////////////////////
  // Pass-through methods required by frame_process.
  // NOTE: For method descriptions, see frame_process.h

  virtual config_block params() const { return sp_impl_->params(); }
  virtual bool set_params( config_block const& blk ) { return sp_impl_->set_params( blk ); }
  virtual bool initialize() { return sp_impl_->initialize(); }
  virtual bool step() { return sp_impl_->process::step(); }
  virtual process::step_status step2() { return sp_impl_->step2(); }
  virtual bool seek( unsigned frame_number ) { return sp_impl_->seek( frame_number ); }
  virtual unsigned nframes() const { return sp_impl_->nframes(); }
  virtual double frame_rate() const { return sp_impl_->frame_rate(); }

  virtual vidtk::timestamp timestamp() const { return sp_impl_->timestamp(); }
  virtual vidtk::video_metadata metadata() const { return sp_impl_->metadata(); }
  virtual vil_image_view<PixType> image() const { return sp_impl_->image(); }

  virtual unsigned ni() const { return sp_impl_->ni(); }
  virtual unsigned nj() const { return sp_impl_->nj(); }


private:
  process_smart_pointer< spProcess > sp_impl_;

};


} //end namespace vidtk


#endif // vidtk_frame_process_sp_facade_h_
