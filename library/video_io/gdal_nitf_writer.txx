/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "gdal_nitf_writer.h"

#include <gdal_priv.h>
#include <cpl_conv.h>
#include <ogr_spatialref.h>

#include <sstream>
#include <ctime>
#include <iomanip>
#include <stdio.h>

#define COMPRESS

#include <logger/logger.h>
VIDTK_LOGGER("gdal_nitf_writer_impl");

namespace vidtk
{

struct gdal_nitf_writer_impl
{
public:
  gdal_nitf_writer_impl()
  :dataset_(NULL), po_driver_(NULL),options_(NULL)
  {}
  ~gdal_nitf_writer_impl()
  {
    close();
  }
  void close()
  {
    if(dataset_)
    {
#ifdef COMPRESS
      {
        //re-ending the image to take up MUCH less harddrive space
        GDALDataset *compressed;
        //Add JPEG Compression
        //CSLDestroy( options_ );
        //options_ = NULL;
        options_ = CSLSetNameValue( options_, "IC", "C3" );

        compressed = po_driver_->CreateCopy( fname_.c_str(), dataset_, FALSE,
                                          options_, GDALTermProgress, NULL );

        //Close output
        if( compressed != NULL )
          GDALClose( static_cast<GDALDatasetH>(compressed) );
      }
#endif
      //Clearing out memory
      GDALClose( static_cast<GDALDatasetH>( dataset_ ) );
      dataset_ = NULL;
      CSLDestroy( options_ );
      options_ = NULL;
#ifdef COMPRESS
      //Delete temporary file.
      remove((fname_+"_temp").c_str());
#endif
    }
  }
  GDALDataset * dataset_;
  GDALDriver *po_driver_;
  char **options_;
  std::string fname_;
  std::string ms_str_;
};

template <class PIX_TYPE>
gdal_nitf_writer<PIX_TYPE>
::gdal_nitf_writer()
  : impl_(new gdal_nitf_writer_impl())
{
  GDALAllRegister();
}

template <class PIX_TYPE>
gdal_nitf_writer<PIX_TYPE>
::~gdal_nitf_writer()
{
  delete impl_;
}

template <class PIX_TYPE>
bool
gdal_nitf_writer<PIX_TYPE>
::open( std::string const& fname, unsigned int ni, unsigned int nj, unsigned int nplanes,
        timestamp const& ts, video_metadata const& vm,
        unsigned int block_x, unsigned int block_y)
{
  if(nplanes != 1) //currently we are only supporting single plane images.
  {
    LOG_ERROR("Currently we are only supporting grayscale image writing");
    return false;
  }
  vidtk::geo_coord::geo_lat_lon const& ul = vm.corner_ul();
  vidtk::geo_coord::geo_lat_lon const& ur = vm.corner_ur();
  vidtk::geo_coord::geo_lat_lon const& lr = vm.corner_lr();
  vidtk::geo_coord::geo_lat_lon const& ll = vm.corner_ll();
#define ADD( NAME, LOC ) \
  {\
    double lat = LOC.get_latitude();\
    double lon = LOC.get_longitude();\
    std::string lat_ss = (lat>=0)?"+":"-";\
    std::string lon_ss = (lon>=0)?"+":"-";\
    lat = std::abs(lat); \
    lon = std::abs(lon); \
    std::string lat_s; \
    {\
      std::stringstream ss;\
      ss << std::setprecision(8) << lat_ss << lat;\
      ss >> lat_s;\
      while (lat_s.size()<10) {lat_s += "0";}\
    }\
    std::string lon_s; \
    {\
      std::stringstream ss;\
      ss <<std::setprecision((lon<100)?9:8) << lon_ss << ((lon<100)?"0":"") << lon;\
      ss >> lon_s;\
      while (lon_s.size()< 11 ) {lon_s += "0";} \
    }\
    impl_->options_ = CSLSetNameValue( impl_->options_, NAME, (lat_s+lon_s).c_str() );\
  }
  //Setup the BLOCK_A for corner points
  impl_->options_ = CSLSetNameValue( impl_->options_, "BLOCKA_BLOCK_COUNT", "01");
  impl_->options_ = CSLSetNameValue( impl_->options_, "BLOCKA_BLOCK_INSTANCE_01", "01");
  {
    std::stringstream ss;
    ss << nj;
    impl_->options_ = CSLSetNameValue( impl_->options_, "BLOCKA_L_LINES_01", ss.str().c_str() );
  }
  impl_->options_ = CSLSetNameValue( impl_->options_, "BLOCKA_N_GRAY_01", "00000" );
  //Add the lat lon location to the correct parameter
  ADD( "BLOCKA_FRFC_LOC_01", ul )
  ADD( "BLOCKA_FRLC_LOC_01", ur )
  ADD( "BLOCKA_LRLC_LOC_01", lr )
  ADD( "BLOCKA_LRFC_LOC_01", ll )
#undef ADD
  //Setup block size of image
  if(block_x < ni && block_y < nj )
  {
    {
      std::stringstream ss;
      ss << block_x;
      impl_->options_ = CSLSetNameValue( impl_->options_, "BLOCKXSIZE", ss.str().c_str());
    }
    {
      std::stringstream ss;
      ss << block_y;
      impl_->options_ = CSLSetNameValue( impl_->options_, "BLOCKYSIZE", ss.str().c_str());
    }
  }

  //add timestamp to the metadata
  double time = ts.time_in_secs() + 0.001; //Force rounding up because time is at a higher
                                           //resolution than our readers can support.  If
                                           //we do not round up,the track heads will not
                                           //line up in vpView.
  { //Milliseconds
    std::string ms = "ENGRDA=Kitware             00112milliseconds00030001A1NA00000003";
    double mil = time - std::floor(time);
    mil *= 1e3;
    int mil_i = static_cast<int>(mil);
    std::stringstream ss;
    ss << std::setw(3) << std::setfill('0') << mil_i;
    ms += ss.str();
    impl_->ms_str_ = ms;
    impl_->options_ = CSLSetNameValue( impl_->options_, "FILE_TRE", ms.c_str() );
  }

  { //Seconds
    std::stringstream ss;
    time_t tt = static_cast< time_t >(time);
    tm *ltm = gmtime(&tt);
#define WRITE(VALUE) std::setfill('0') << std::setw(2) <<  VALUE
    ss << (ltm->tm_year + 1900) << WRITE((ltm->tm_mon + 1)) << WRITE(ltm->tm_mday) << WRITE(ltm->tm_hour) << WRITE(ltm->tm_min) << WRITE( ltm->tm_sec );
    impl_->options_ = CSLSetNameValue( impl_->options_, "IDATIM", ss.str().c_str() );
    assert(ss.str().size() == 14);
#undef WRITE
  }

  //Get the driver for NITF images
  impl_->po_driver_ = GetGDALDriverManager()->GetDriverByName("NITF");

  impl_->fname_ = fname;
#ifdef COMPRESS
  //We have to use a temporary file since GDAL built in compression is only supported when copying a file.
  //This temporary file will be deleted after compression.
  impl_->dataset_ = impl_->po_driver_->Create( (fname+"_temp").c_str(),
#else
  impl_->dataset_ = impl_->po_driver_->Create( (fname).c_str(),
#endif  //GDT_Byte will have to be change for support of 16bit images.
                                               ni, nj, nplanes, GDT_Byte, impl_->options_ );
  return is_open();
}

template <class PIX_TYPE>
bool
gdal_nitf_writer<PIX_TYPE>
::is_open() const
{
  return impl_->dataset_ != NULL;
}

template <class PIX_TYPE>
bool
gdal_nitf_writer<PIX_TYPE>
::write_block(vil_image_view<PIX_TYPE> & img, unsigned int ux, unsigned int uy)
{
  if(img.nplanes() != 1) //currently we are only supporting single plane images.
  {
    LOG_ERROR("Currently we are only supporting grayscale image writing");
    return false;
  }
  GDALRasterBand * band = impl_->dataset_->GetRasterBand(1);
  PIX_TYPE * top_ptr = img.top_left_ptr();
  band->RasterIO( GF_Write, ux, uy, img.ni(),img.nj(),
                  static_cast<void*>(top_ptr),
                  img.ni(), img.nj(), GDT_Byte, 0, 0 ); //GDT_Byte will have to be change for support of 16bit images.
  return true;
}

template <class PIX_TYPE>
void
gdal_nitf_writer<PIX_TYPE>
::close()
{
  if(impl_)
  {
    impl_->close();
  }
}

} //namespace vidtk
