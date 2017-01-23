/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "vil_gdal_image.h"
#include "cpl_vsi_vil.h"
#include "vil_nitf_metadata_parser.h"
#include "vil_nitf_engrda.h"

#include <vil/vil_property.h>

// GDAL includes
#include <gdal_priv.h>
#include <cpl_vsi.h>

// needed for memcpy
#include <cstring>
#include <vector>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <vil_plugins/file_formats/vil_nitf_engrda.h>

static const char gdal_str[] = "GDALReadable";

//*********************************************************************
char const *
vil_gdal_file_format
::tag() const
{
  return gdal_str;
}

vil_image_resource_sptr
vil_gdal_file_format
::make_input_image(vil_stream *vs)
{
  vil_gdal_image *img = new vil_gdal_image(vs);
  if(!img->is_good())
  {
    delete img;
    return NULL;
  }
  return img;
}


vil_image_resource_sptr
vil_gdal_file_format
::make_output_image(
  vil_stream* vs, unsigned nx, unsigned ny, unsigned nplanes,
  enum vil_pixel_format)
{
  return NULL;
}


//*********************************************************************
class vil_gdal_image_impl
{
public:
  vil_gdal_image_impl(vil_gdal_image* parent)
  : parent_(parent)
  {
  }

  ~vil_gdal_image_impl(void)
  {
  }

  void get_block_size(int* i_size, int* j_size)
  {
    unsigned no_of_planes = this->parent_->nplanes();
    for(unsigned i = 1; i <= no_of_planes; ++i)
    {
      GDALRasterBand* rasterBand =
        this->parent_->gdal_data_->GetRasterBand(i);
      rasterBand->GetBlockSize(i_size, j_size);
      return;
    }
  }

  unsigned get_block_size_i()
  {
    int i_size, j_size;
    this->get_block_size(&i_size, &j_size);

    return static_cast<unsigned>(i_size);
  }

  unsigned get_block_size_j()
  {
    int i_size, j_size;
    this->get_block_size(&i_size, &j_size);

    return static_cast<unsigned>(j_size);
  }

  template <typename TPixel>
  vil_image_view_base_sptr read_data(
    vil_memory_chunk_sptr chunk, unsigned i0, unsigned ni,
    unsigned j0, unsigned nj, unsigned np)
  {
    vil_image_view<TPixel> *view = new vil_image_view<TPixel>(
      chunk, reinterpret_cast<TPixel*>(chunk->data()),
      ni, nj, np, 1, ni, ni*nj);

    // Possible bands
    GDALRasterBand* redBand = 0;
    GDALRasterBand* greenBand = 0;
    GDALRasterBand* blueBand = 0;
    GDALRasterBand* alphaBand = 0;
    GDALRasterBand* greyBand = 0;
    GDALDataType target_data_type = GDT_Unknown;

    for (unsigned i = 1; i <= np; ++i)
    {
      GDALRasterBand* rasterBand = this->parent_->gdal_data_->GetRasterBand(i);

      if(target_data_type == GDT_Unknown)
      {
        target_data_type = rasterBand->GetRasterDataType();
      }

      if ((rasterBand->GetColorInterpretation() == GCI_RedBand) ||
          (rasterBand->GetColorInterpretation() == GCI_YCbCr_YBand))
      {
        redBand = rasterBand;
      }
      else if ((rasterBand->GetColorInterpretation() == GCI_GreenBand) ||
               (rasterBand->GetColorInterpretation() == GCI_YCbCr_CbBand))
      {
        greenBand = rasterBand;
      }
      else if ((rasterBand->GetColorInterpretation() == GCI_BlueBand) ||
               (rasterBand->GetColorInterpretation() == GCI_YCbCr_CrBand))
      {
        blueBand = rasterBand;
      }
      else if (rasterBand->GetColorInterpretation() == GCI_AlphaBand)
      {
        alphaBand = rasterBand;
      }
      else if (rasterBand->GetColorInterpretation() == GCI_GrayIndex)
      {
        greyBand = rasterBand;
      }
    }

    int dest_width = static_cast<int>(ni);
    int dest_height = static_cast<int>(nj);

    // GDAL top left is at 0,0
    const int window_x = static_cast<int>(i0);
    const int window_y = static_cast<int>(j0);
    const int window_width = dest_width;
    const int window_height = dest_height;

    unsigned bypes_per_pixel =
      vil_pixel_format_sizeof_components(this->parent_->pixel_format());
    int pixel_space = static_cast<int>(bypes_per_pixel);
    int line_space = dest_width * pixel_space;
    int band_space = dest_width * dest_height * pixel_space;
    CPLErr err;

    // TODO: Support other band types
    if (redBand && greenBand && blueBand)
    {
      if(alphaBand)
      {
        err = redBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 0*band_space),
          dest_width, dest_height, target_data_type, pixel_space, line_space);

        err = greenBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 1*band_space),
          dest_width, dest_height, target_data_type, pixel_space, line_space);

        err = blueBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 2*band_space),
          dest_width, dest_height, target_data_type, pixel_space, line_space);

        err = alphaBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 3 * band_space),
          dest_width, dest_height, target_data_type, pixel_space, line_space);
      }
      else
      {
        err = redBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 0*band_space),
          dest_width, dest_height, target_data_type, 0, 0);

        err = greenBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 1*band_space),
          dest_width, dest_height, target_data_type, 0,0);

        err = blueBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 2*band_space),
          dest_width, dest_height, target_data_type, 0,0);
      }
    }
    else if (greyBand)
    {
      if (alphaBand)
      {
        // Luminance alpha
        err = greyBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 0*band_space),
          dest_width, dest_height, target_data_type, pixel_space, line_space);
        err =  alphaBand->RasterIO(GF_Read,
          window_x, window_y,  window_width, window_height,
          (void*)((GByte*)chunk->data() + 1*band_space),
          dest_width, dest_height, target_data_type, pixel_space, line_space);
      }
      else
      {
        // Luminance
        err = greyBand->RasterIO(GF_Read,
          window_x, window_y, window_width, window_height,
          (void*)((GByte*)chunk->data() + 0*band_space),
          dest_width, dest_height, target_data_type, pixel_space, line_space);
      }
    }
    else
    {
      std::cerr << "Unknown raster band type \n";
      return NULL;
    }

    assert(err == CE_None);
    return view;
  }

  vil_gdal_image *parent_;
  VSILFILE* vfile_;
  std::string vfilename_;
};

//*********************************************************************
vil_gdal_image
::vil_gdal_image(vil_stream *s)
  : impl_(new vil_gdal_image_impl(this))
{
  // Initialize all variables
  this->gdal_data_ = NULL;

  // identify the file from the first 4 bytes
  char id[4];
  s->read(id, 4);
  s->seek(0);

  // only handle NITF in GDAL, for now.
  if(std::strncmp(id, "NITF", 4) != 0)
  {
    return;
  }

  // Enable all drivers
  GDALAllRegister();

  // Create virtual file
  std::ostringstream out;
  // GDAL requires a valid file extension
  out << "/vsivil/tmp_" << this << ".ntf";
  this->gdal_filename_ = out.str();

  VSIInstallVILFileHandler();
  VSIRegisterVILStream(this->gdal_filename_.c_str(), s);

  this->gdal_data_ = (GDALDataset*) GDALOpen(
    this->gdal_filename_.c_str(), GA_ReadOnly);
  if(!this->gdal_data_)
  {
    return;
  }

  char** metaData = GDALGetMetadata(this->gdal_data_, NULL);
  for (int i = 0; metaData[i] != NULL; ++i)
  {
    std::vector<std::string> temp;
    boost::split(temp, metaData[i], boost::is_any_of("="));
    this->parsed_data_[temp[0]] = temp[1];
  }
}


vil_gdal_image
::~vil_gdal_image(void)
{
  if(this->gdal_data_)
  {
    GDALClose( (GDALDatasetH) this->gdal_data_ );
    this->gdal_data_ = NULL;
  }
  VSIUnRegisterVILStream(this->gdal_filename_.c_str());
  delete this->impl_;
}

//: Dimensions:  Planes x ni x nj.
// This concept is treated as a synonym to components.
unsigned
vil_gdal_image
::nplanes() const
{
  return static_cast<unsigned>(this->gdal_data_->GetRasterCount());
}

//: Dimensions:  Planes x ni x nj.
// The number of pixels in each row.
unsigned
vil_gdal_image
::ni() const
{
  return static_cast<unsigned>(this->gdal_data_->GetRasterXSize());
}

//: Dimensions:  Planes x ni x nj.
// The number of pixels in each column.
unsigned
vil_gdal_image
::nj() const
{
  return static_cast<unsigned>(this->gdal_data_->GetRasterYSize());
}

//: Pixel Format.
//  A standard RGB RGB RGB of chars image has
// pixel_format() == VIL_PIXEL_FORMAT_BYTE
enum vil_pixel_format
vil_gdal_image
::pixel_format() const
{
  unsigned no_of_planes = this->nplanes();
  for (unsigned i = 1; i <= no_of_planes; ++i)
  {
    GDALRasterBand* rasterBand = this->gdal_data_->GetRasterBand(i);
    GDALDataType target_data_type = rasterBand->GetRasterDataType();
    switch (target_data_type)
    {
      case (GDT_Byte): return VIL_PIXEL_FORMAT_BYTE;
      case (GDT_UInt16): return VIL_PIXEL_FORMAT_UINT_16;
      case (GDT_Int16): return VIL_PIXEL_FORMAT_INT_16;
      case (GDT_UInt32): return VIL_PIXEL_FORMAT_UINT_32;
      case (GDT_Int32): return VIL_PIXEL_FORMAT_INT_32;
      case (GDT_Float32): return VIL_PIXEL_FORMAT_FLOAT;
      case (GDT_Float64): return VIL_PIXEL_FORMAT_DOUBLE;
      default: return VIL_PIXEL_FORMAT_UNKNOWN;
    }
  }

  return VIL_PIXEL_FORMAT_BYTE;
}

//: TODO Add comment
vil_image_view_base_sptr
vil_gdal_image::get_copy_view(unsigned i0, unsigned ni,
                              unsigned j0, unsigned nj) const
{
  vil_pixel_format fmt = this->pixel_format();
  unsigned int np = this->nplanes();
  unsigned int Bpp = vil_pixel_format_sizeof_components(fmt);
  vil_memory_chunk_sptr chunk = new vil_memory_chunk(ni*nj*np*Bpp, fmt);
  std::memset(chunk->data(), 0, chunk->size());
  switch(this->pixel_format())
  {
    case VIL_PIXEL_FORMAT_BYTE    :
      return this->impl_->read_data<vxl_byte>(chunk, i0, ni, j0, nj, np);
    case VIL_PIXEL_FORMAT_SBYTE:
      return this->impl_->read_data<vxl_sbyte>(chunk, i0, ni, j0, nj, np);
    case VIL_PIXEL_FORMAT_UINT_16:
      return this->impl_->read_data<vxl_uint_16>(chunk, i0, ni, j0, nj, np);
    case VIL_PIXEL_FORMAT_INT_16:
      return this->impl_->read_data<vxl_int_16>(chunk, i0, ni, j0, nj, np);
    case VIL_PIXEL_FORMAT_UINT_32:
      return this->impl_->read_data<vxl_uint_32>(chunk, i0, ni, j0, nj, np);
    case VIL_PIXEL_FORMAT_INT_32:
      return this->impl_->read_data<vxl_int_32>(chunk, i0, ni, j0, nj, np);
    case VIL_PIXEL_FORMAT_FLOAT:
        return this->impl_->read_data<float>(chunk, i0, ni, j0, nj, np);
    case VIL_PIXEL_FORMAT_DOUBLE:
        return this->impl_->read_data<double>(chunk, i0, ni, j0, nj, np);
    default:
      return NULL;
  }

  return NULL;
}

//: Return a string describing the file format.
// Only file images have a format, others return 0
char const*
vil_gdal_image
::file_format() const
{
  return gdal_str;
}

//: Extra property information
bool
vil_gdal_image
::get_property(char const* tag, void* property_value) const
{
  if(!property_value)
  {
    return false;
  }

  std::string tag_str(tag);
  std::string tag_full = map_property_name(tag);

  if(tag_str == vil_property_size_block_i)
  {
    unsigned* value = reinterpret_cast<unsigned*>(property_value);
    *value = this->impl_->get_block_size_i();
    return true;
  }
  if(tag_str == vil_property_size_block_j)
  {
    unsigned* value = reinterpret_cast<unsigned*>(property_value);
    *value = this->impl_->get_block_size_j();
    return true;
  }

  std::string metadata;
  if(tag_full == "NITF::IM001::IDATIM")
  {
    std::time_t *value = reinterpret_cast<std::time_t*>( property_value );
    if (!get_data("NITF_IDATIM", metadata))
    {
      std::cerr << "Could not get NITF_IDATIM value" << std::endl;
    }
    return vil_nitf_metadata_parser::parse_datetime( metadata.c_str(), *value );
  }

  if(tag_full == "NITF::IM001::TRE_ENGRDA")
  {
    vil_nitf_engrda *value = reinterpret_cast<vil_nitf_engrda*>(property_value);
    char** tre_data = GDALGetMetadata(this->gdal_data_, "TRE");
    for (int i = 0; tre_data[i] != NULL; ++i)
    {
      std::vector<std::string> temp;
      boost::split(temp, tre_data[i], boost::is_any_of("="));
      if ( temp[0] == "ENGRDA" )
      {
        return vil_nitf_metadata_parser::parse_engrda( temp[1], *value );
      }
    }
    return false;
  }
  double *value = reinterpret_cast<double*>(property_value);
  if( tag_full == "NITF::IM001::TRE_BLOCKA_FRFC_LOC"  )
  {
    if (!get_data("NITF_BLOCKA_FRFC_LOC_01", metadata))
    {
      std::cerr << "Could not get FRFC value" << std::endl;
    }
    return vil_nitf_metadata_parser::parse_latlon(metadata, value[0], value[1]);
  }

  if( tag_full == "NITF::IM001::TRE_BLOCKA_FRLC_LOC" )
  {
    if (!get_data("NITF_BLOCKA_FRLC_LOC_01", metadata))
    {
      std::cerr << "Could not get FRLC value" << std::endl;
    }
    return vil_nitf_metadata_parser::parse_latlon(metadata, value[0], value[1]);
  }

  if( tag_full == "NITF::IM001::TRE_BLOCKA_LRLC_LOC")
  {
    if (!get_data("NITF_BLOCKA_LRLC_LOC_01", metadata))
    {
      std::cerr << "Could not get LRLC value" << std::endl;
    }
    return vil_nitf_metadata_parser::parse_latlon(metadata, value[0], value[1]);
  }

  if( tag_full == "NITF::IM001::TRE_BLOCKA_LRFC_LOC")
  {
    if (!get_data("NITF_BLOCKA_LRFC_LOC_01", metadata))
    {
      std::cerr << "Could not get LRFC value" << std::endl;
    }
    return vil_nitf_metadata_parser::parse_latlon(metadata, value[0], value[1]);
  }

  if( tag_full == "NITF::IM001::MILLI_BAE" )
  {
    if (!get_data("NITF_IMAGE_COMMENTS", metadata))
    {
      std::cerr << "could not get the image metadata comments" << std::endl;
    }
    return vil_nitf_metadata_parser::parse_bae_comment(metadata, *value);
  }

  //Have a single keyword to support multiple possible millisecond formats.
  //Hide the source of milliseconds from the client code
  if(tag_full == "NITF::GET::MILLI")
  {
    vil_nitf_engrda engrda;
    double *value = reinterpret_cast<double*>(property_value);
    *value = 0;
    if(this->get_property("NITF::IM001::TRE_ENGRDA", &engrda))
    {
      std::string t_ms;
      vil_nitf_engrda_element_sptr element;
      element = engrda.get("milliseconds");
      if(element && element->get_data(t_ms))
      {
        std::stringstream ss;
        ss << t_ms;
        ss >> *value;
        return true;
      }
    }
    if(this->get_property("NITF::IM001::MILLI_BAE", value))
    {
      return true;
    }
  }

  return false;
}

bool
vil_gdal_image
::get_data(char const* tag, std::string &metadata) const
{
  std::map<std::string,std::string>::const_iterator i;
  i = this->parsed_data_.find(tag);
  if( i == this->parsed_data_.end() )
  {
    return false;
  }
  metadata = i->second;
  return true;
}

//: Convert short form property names into long form names
// Example, "FRLC_LOC" converts to "NITF::IM001::TRE_BLOCKA_FRLC_LOC"
std::string
vil_gdal_image
::map_property_name(const std::string& tag)
{
  if     ( tag == "FRFC_LOC" ) { return "NITF::IM001::TRE_BLOCKA_FRFC_LOC"; }
  else if( tag == "FRLC_LOC" ) { return "NITF::IM001::TRE_BLOCKA_FRLC_LOC"; }
  else if( tag == "LRLC_LOC" ) { return "NITF::IM001::TRE_BLOCKA_LRLC_LOC"; }
  else if( tag == "LRFC_LOC" ) { return "NITF::IM001::TRE_BLOCKA_LRFC_LOC"; }
  else if( tag == "BLOCKA" ) { return "NITF::IM001::TRE_BLOCKA"; }
  else if( tag == "ENGRDA" ) { return "NITF::IM001::TRE_ENGRDA"; }
  else if( tag == "IDATIM" ) { return "NITF::IM001::IDATIM"; }
  else if( tag == "MILLI_BAE" ) { return "NITF::IM001::MILLI_BAE"; }
  else if( tag == "MILLISECONDS" ) { return "NITF::GET::MILLI"; }
  return tag;
}

//: Check the status of the created image
bool
vil_gdal_image
::is_good(void) const
{
  if (this->gdal_data_)
  {
    return true;
  }
  return false;
}
