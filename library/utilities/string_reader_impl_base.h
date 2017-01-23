/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_STRING_READER_IMPL_BASE_H_
#define _VIDTK_STRING_READER_IMPL_BASE_H_

#include <utilities/config_block.h>
#include <string>

namespace vidtk {
namespace string_reader_impl {


// ----------------------------------------------------------------
/*! \brief Abstract base class for string reader implementations.
 *
 * This class defines the interface and common methods for string
 * reader classes. You will notice that it looks somewhat like a
 * process interface.
 */
class string_reader_impl_base
{
public:
  string_reader_impl_base( std::string const& n)
    : name_ (n) {}
  virtual ~string_reader_impl_base() {}

  virtual config_block params() const { return config_; }
  virtual bool set_params( config_block const& blk ) = 0;
  virtual bool initialize() = 0;
  virtual bool step() = 0;

  std::string const& name() const { return name_; }


  config_block config_;
  std::string str_;

private:
  std::string name_; ///< implementation name
};



} // end namespace string_reader_impl
} // end namespace vidtk

#endif /* _VIDTK_STRING_READER_IMPL_BASE_H_ */
