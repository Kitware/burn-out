/*ckwg +5
 * Copyright 2011-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#ifndef opencv_raw_color_histogram_functions
#define opencv_raw_color_histogram_functions


// Standard C/C++
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <functional>
#include <numeric>
#include <limits>
#include <stdexcept>

// OpenCV
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>


// Misc helper functions used for OpenCV based color descriptors. At a later date,
// these classes/functions should be sorted and cleaned up to match the VIDTK
// coding standards, many can be simplified / eliminated.


namespace vidtk
{


//------------------------------------------------------------------------------
//        Multi-Feature Histogram for People and Vehicle Descriptors
//------------------------------------------------------------------------------

/**
 * Class to construct a color filter class, a class which generates different
 * summary properties for a color region around a track over time including:
 *
 *  - A color histogram for the area defined by some mask over time around the track
 *  - The mapping of the above region onto a color BoW model
 *  - A single summary value for the object shown in the track incorporating multiple
 *    measurements (hue, r/g, b/g, rgb)
 *  - Additional features aimed specifically for cases when the object is a person
 *
 * Note: This class is optimized for, and only supports 3-channeled 8-bit
 * [0,255] range images.
 */
class MultiFeatureHist
{
public:

  /// ---------------------- HELPER TYPEDEFS ---------------------------

  typedef int BoWWordType;
  typedef double HistType;
  typedef std::vector< std::vector< BoWWordType > > Vocabulary;
  typedef std::vector< double > Descriptor;

  /// ------------------- PUBLIC MEMBER FUNCTIONS ----------------------

  /**
   * Declares a new mfeature class, but does not allocate an internal hist
   */
  MultiFeatureHist();

  /**
   * Destructor, deallocates histogram if present
   */
  ~MultiFeatureHist();

  /**
   * Allocates a new histogram with total bins: SideLength^3
   *
   * @param SideLength resolution of each channel
   */
  MultiFeatureHist( const int& SideLength );

  /**
   * Allocates a new histogram of the given resolution
   *
   * @param SideLength resolution of each channel
   */
  void AllocateHistogram( const int& SideLength );

  /**
   * Update internal state
   *
   * @param img Input image
   * @param mask Binary or floating-point mask of same size of image
   */
  void UpdateModelFromImage( IplImage *img, IplImage *mask = NULL );

  /**
   * Smoothes internal histogram via simple convolution with a 3D kernel
   */
  void SmoothHistogram();

  /**
   * Outputs a new IplImage representing the commonality of colors
   * in an image based off of the stored histogram
   *
   * @param img Input image
   * @param scaling Whether or not to scale based on contributors
   * @return A new OpenCV floating point image of the same size as the
   * input, where lower values correspond to colors which are less common
   * (as denoted by the currently stored histogram)
   */
  IplImage * CreateColorCommonalityImage( IplImage * img, bool cscaling = false );

  /**
   * Normalizes and then dumps a snapshot of the current feature contents
   * into each preallocated vector.
   */
  void DumpOutputs( std::vector<double>& HistVector,
                    std::vector<double>& ValueVector,
                    std::vector<double>& ObjVector );

  /**
   * Resets all internal values to defaults
   */
  void Reset();

  /**
   * Puts more emphasis on non-black colours in the stored RGB histogram. Weights
   * all pixels within some normalized distance of pure black according to some
   * specified nth degree polynomial function.
   */
  void BlackFilter( double base = 0.25, int polynomial = 2, double distance = 0.25 );

  /**
   * Get the total stored weight in the histogram
   */
  double StoredWeight() { return TopWeight + BotWeight; }

private:

  /// ----------------- INTERNAL HELPER FUNCTIONS ----------------------

  /**
   * Finds the value of the specified color in the current histogram
   *
   * @param Color A pointer to an 8-bit 3 channel color (3 unsigned chars)
   * @return Value the color has in the current histogram
   */
  HistType ClassifyColor3D( unsigned char* Color );

  /**
   * Convert a single RGB value into multiple features
   *
   * @param [in/out] SVFeatures Size 6 output vector
   */
  void CalculateSVFeatures( Descriptor& SVFeatures );

  /**
   * Calculate simple shirt-pants color approximation feature vector
   *
   * @param [in/out] SVFeatures Size 12 output vector
   */
  void CalculateObjFeatures( Descriptor& ObjFeatures );

  /**
   * Adds a pixel value to the histogram with some given weight
   *
   * @param Color A pointer to an pixel in the image
   * @param Weight Weight of said point
   */
  void AddValueToHist( unsigned char* Color, const double& Weight, const bool& InTopHalf );


  /// -------------------- HISTOGRAM VARIABLES -------------------------

  /// Continuous buffer to hold histogram
  HistType *Hist;

  /// Is the histogram allocated and valid?
  bool IsHistValid;

  /// The number of bins per each channel of the histogram (Resolution)
  unsigned int BinsPerDim;

  /// Total # of bins in histogram
  unsigned int HistSize;

  /// Total # of regions used to form the current histogram
  unsigned int HistContributors;

  /// Width Step for Hist Buffer (Ch1)
  unsigned int WidthStepCh1;

  /// Width Step for Hist Buffer (Ch2)
  unsigned int WidthStepCh2;

  /// ----------------------- OBJ VARIABLES ----------------------------

  /// Average value for top bin
  double TopAvg[3];

  /// Current weight of the top bin value
  double TopWeight;

  /// Average value for bot bin
  double BotAvg[3];

  /// Current weight of the top bin value
  double BotWeight;

};


//------------------------------------------------------------------------------
//                       Simple Raw Color Histogram
//------------------------------------------------------------------------------


/**
 * Class to construct a color Histogram, a 3D matrix representing an
 * entire color space.
 *
 * Note: This class is optimized for, and only supports RGB 8-bit
 * [0,255] range images.
 */
class Histogram3D
{
public:

  /**
   * Declares a new histogram class, but does not allocate histogram
   */
  Histogram3D() : Hist(NULL), IsValid(false) {}

  /**
   * Allocates a new histogram with total bins: SideLength^3
   *
   * @param SideLength resolution of each channel
   */
  Histogram3D( const int& SideLength );

  /**
   * Allocates a new histogram and builds one from the input img
   *
   * @param img Input image to build histogram off of
   * @param SideLength resolution of each channel
   */
  Histogram3D( IplImage *img, const int& SideLength );

  /**
   * Allocates a new histogram of the given resolution
   *
   * @param SideLength resolution of each channel
   */
  void AllocateHistogram( const int& SideLength );

  /**
   * Destructor, deallocates histogram if present
   */
  ~Histogram3D();

  /**
   * Build a new histogram from an entire input image
   *
   * @param img Input image
   */
  void BuildHistogram( IplImage *img );

  /**
   * Build a new histogram from a square region in an input image
   *
   * @param img Input image
   * @param region Input region (rectangle)
   */
  void BuildHistogram( IplImage *img, const cv::Rect& region );

  /**
   * Adds the histogram of the input image to the currently stored histogram
   *
   * @param img Input image
   * @param weight Weight to give new histogram (default=1)
   */
  void AddImgToHistogram( IplImage *img, double weight = 1.0 );

  /**
   * Adds the histogram of a region to the currently stored histogram
   *
   * @param img Input image
   * @param weight Weight to give new histogram (default=1)
   */
  void AddRegionToHistogram( IplImage *img, const cv::Rect& region, double weight = 1.0 );

  /**
   * Resets all values in the histogram to 0
   */
  void ResetHistogram();

  /**
   * Smoothes a histogram via simple convolution with a 3D kernel
   */
  void SmoothHistogram();

  /**
   * Normalize a histogram such that the sum of its contents is 1
   */
  void NormalizeHistogram();

  /**
   * Negates all values in a histogram
   */
  void NegateHistogram();

  /**
   * Outputs a new IplImage representing the commonality of colors
   * in an image based off of the stored histogram
   *
   * @param img Input image
   * @param scaling Whether or not to scale based on contributors
   * @return A new OpenCV floating point image of the same size as the
   * input, where lower values correspond to colors which are less common
   * (as denoted by the currently stored histogram)
   */
  IplImage * CreateColorCommonalityImage( IplImage * img, bool cscaling = false );

  /**
   * Outputs a new binary IplImage showing all pixels belonging to some
   * channel of the histogram
   *
   * @param img Input image
   * @param channel Channel number
   * @return A new OpenCV floating point image of the same size as the
   * input, where a positive value correspond to colors in the channel
   */
  IplImage * CreateChannelBinaryImage( IplImage * img, int channel );

  /**
   * Dumps a snapshot of the current histogram into the specified vector
   *
   * @param InputVector A vector to dump the histogram contents into
   */
  void CopyHistIntoVector( std::vector<double>& InputVector ) const;

private:

  /**
   * Finds the value of the specified color in the current histogram
   *
   * @param Color A pointer to an 8-bit 3 channel color (3 unsigned chars)
   * @return Value the color has in the current histogram
   */
  double ClassifyColor3D( unsigned char* Color );

  /// Continuous buffer to hold histogram
  double *Hist;

  /// Is the histogram allocated and valid?
  bool IsValid;

  /// The number of bins per each channel of the histogram (Resolution)
  unsigned int BinsPerDim;

  /// Total # of doubles (entries) in histogram
  unsigned int HistSize;

  /// Total # of regions used to form the current histogram
  unsigned int HistContributors;

  /// Width Step for Hist Buffer (Ch1)
  unsigned int WidthStepCh1;

  /// Width Step for Hist Buffer (Ch2)
  unsigned int WidthStepCh2;
};


//------------------------------------------------------------------------------
//                      Intensity Histogram Only
//------------------------------------------------------------------------------

/**
 * Class to construct a color Histogram, a 3D matrix representing an
 * entire color space.
 *
 * Note: This class is optimized for, and only supports RGB 8-bit
 * [0,255] single-channel images.
 */
class IntensityHistogram
{
public:

  /**
   * Declares a new histogram class, but does not allocate histogram
   */
  IntensityHistogram() : Hist(NULL), IsValid(false) {}

  /**
   * Allocates a new histogram with total bins: SideLength^3
   *
   * @param SideLength resolution of each channel
   */
  IntensityHistogram( const int& SideLength );

  /**
   * Allocates a new histogram and builds one from the input img
   *
   * @param img Input image to build histogram off of
   * @param SideLength resolution of each channel
   */
  IntensityHistogram( IplImage *img, const int& SideLength );

  /**
   * Allocates a new histogram of the given resolution
   *
   * @param SideLength resolution of each channel
   */
  void AllocateHistogram( const int& SideLength );

  /**
   * Destructor, deallocates histogram if present
   */
  ~IntensityHistogram();

  /**
   * Build a new histogram from an entire input image
   *
   * @param img Input image
   */
  void BuildHistogram( IplImage *img );

  /**
   * Build a new histogram from a square region in an input image
   *
   * @param img Input image
   * @param region Input region (rectangle)
   */
  void BuildHistogram( IplImage *img, const cv::Rect& region );

  /**
   * Adds the histogram of the input image to the currently stored histogram
   *
   * @param img Input image
   * @param weight Weight to give new histogram (default=1)
   */
  void AddImgToHistogram( IplImage *img, double weight = 1.0 );

  /**
   * Adds the histogram of a region to the currently stored histogram
   *
   * @param img Input image
   * @param weight Weight to give new histogram (default=1)
   */
  void AddRegionToHistogram( IplImage *img, const cv::Rect& region, double weight = 1.0 );

  /**
   * Resets all values in the histogram to 0
   */
  void ResetHistogram();

  /**
   * Smoothes a histogram via simple convolution with a 3D kernel
   */
  void SmoothHistogram();

  /**
   * Normalize a histogram such that the sum of its contents is 1
   */
  void NormalizeHistogram();

  /**
   * Negates all values in a histogram
   */
  void NegateHistogram();

  /**
   * Outputs a new IplImage representing the commonality of colors
   * in an image based off of the stored histogram
   *
   * @param img Input image
   * @param scaling Whether or not to scale based on contributors
   * @return A new OpenCV floating point image of the same size as the
   * input, where lower values correspond to colors which are less common
   * (as denoted by the currently stored histogram)
   */
  IplImage * CreateColorCommonalityImage( IplImage * img, bool cscaling = false );

  /**
   * Dumps a snapshot of the current histogram into the specified vector
   *
   * @param InputVector A vector to dump the histogram contents into
   */
  void CopyHistIntoVector( std::vector<double>& InputVector ) const;

private:

  /**
   * Finds the value of the specified color in the current histogram
   *
   * @param Color A pointer to an 8-bit 1 channel intensity
   * @return Value the color has in the current histogram
   */
  double ClassifyIntensity( unsigned char* Color );

  /// Continuous buffer to hold histogram
  double *Hist;

  /// Is the histogram allocated and valid?
  bool IsValid;

  /// The number of bins per each channel of the histogram (Resolution)
  unsigned int BinsPerDim;

  /// Total # of doubles (entries) in histogram
  unsigned int HistSize;

  /// Total # of regions used to form the current histogram
  unsigned int HistContributors;
};


//------------------------------------------------------------------------------
//                         Simple Spectral Histogram
//------------------------------------------------------------------------------

/**
 * Class to construct a color spectral Histogram, where a histogram
 * of the specified resolution is created for each channel.
 *
 * Note: This class is optimized for, and only supports RGB 8-bit
 * [0,255] range images.
 */
class SpectralHistogram {
public:

  /**
   * Declares a new histogram class, but does not allocate histogram
   */
  SpectralHistogram() : HistCh1(NULL), HistCh2(NULL), HistCh3(NULL), IsValid(false) {}

  /**
   * Allocates a new histogram with total bins: SideLength for each chan
   *
   * @param ResolutionPerChan resolution of each channel hist
   */
  SpectralHistogram( const int& ResolutionPerChan );

  /**
   * Allocates a new histogram and builds one from the input img
   *
   * @param img Input image to build histogram off of
   * @param ResolutionPerChan resolution of each channel hist
   */
  SpectralHistogram( IplImage *img, const int& ResolutionPerChan );

  /**
   * Allocates a new histogram of the given resolution
   *
   * @param ResolutionPerChan resolution of each channel hist
   */
  void AllocateHistograms( const int& ResolutionPerChan );

  /**
   * Destructor, deallocates histogram if present
   */
  ~SpectralHistogram();

  /**
   * Build a new histogram from an entire input image
   *
   * @param img Input image
   */
  void BuildHistograms( IplImage *img );

  /**
   * Build a new histogram from a square region in an input image
   *
   * @param img Input image
   * @param region Input region (rectangle)
   */
  void BuildHistograms( IplImage *img, const cv::Rect& region );

  /**
   * Adds the histogram of the input image to the currently stored histograms
   *
   * @param img Input image
   * @param weight Weight to give new histogram (default=1)
   */
  void AddImgToHistograms( IplImage *img, double weight = 1.0 );

  /**
   * Adds the histogram of a region to the currently stored histograms
   *
   * @param img Input image
   * @param weight Weight to give new histogram (default=1)
   */
  void AddRegionToHistograms( IplImage *img, const cv::Rect& region, double weight = 1.0 );

  /**
   * Resets all values in the histogram to 0
   */
  void ResetHistograms();

  /**
   * Smoothes a histogram via simple convolution with a 3D kernel
   */
  void SmoothHistograms();

  /**
   * Normalize a histogram such that the sum of its contents is 1
   */
  void NormalizeHistograms();

  /**
   * Negates all values in a histogram
   */
  void NegateHistograms();

  /**
   * Outputs a new IplImage representing the commonality of colors
   * in an image based off of the stored histogram
   *
   * @param img Input image
   * @return A new OpenCV floating point image of the same size as the
   * input, where lower values correspond to colors which are less common
   * (as denoted by the currently stored histogram)
   */
  IplImage *CreateColorCommonalityImage( IplImage *img );

  /**
   * Dumps a snapshot of the current histogram into the specified vector
   *
   * @param InputVector A vector to dump the histogram contents into
   */
  void CopyHistsIntoVector( std::vector<double>& InputVector ) const;

private:

  /**
   * Finds the value of the specified color in the current histogram
   *
   * @param Color A pointer to an 8-bit 3 channel color (3 unsigned chars)
   * @return Value the color has in the current histogram
   */
  double ClassifyColor3D( unsigned char* Color );

  /**
   * Normalizes a given histogram
   *
   * @param input A pointer to the start of the histogram buffer
   * @param size The size of the inputted histogram
   */
  void NormalizeHistogram( double* input, unsigned int size );

  /// Continuous buffer to hold Ch1 histogram
  double *HistCh1;

  /// Continuous buffer to hold Ch2 histogram
  double *HistCh2;

  /// Continuous buffer to hold Ch3 histogram
  double *HistCh3;

  /// Are the hists allocated?
  bool IsValid;

  /// Total # of doubles in each histogram
  unsigned int HistSize;
};


} // end namespace vidtk

#endif
