/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "color_descriptor_helpers_opencv.h"

namespace vidtk
{

//------------------------------------------------------------------------------
//                 Multi-Feature PVO Histogram Definition
//------------------------------------------------------------------------------

// Class constructor
MultiFeatureHist::MultiFeatureHist()
{
  this->IsHistValid = false;
  this->Hist = NULL;
  this->Reset();
}

// Class constructor
MultiFeatureHist::MultiFeatureHist( const int& SideLength )
{
  this->AllocateHistogram( SideLength );
  this->Reset();
}

// Allocates a new histogram of the specified resolution
void MultiFeatureHist::AllocateHistogram( const int& SideLength )
{
  // Deallocate histogram if we already allocated one
  if( IsHistValid )
  {
    delete[] Hist;
  }

  // Set required internal variables
  BinsPerDim = SideLength;
  WidthStepCh2 = BinsPerDim;
  WidthStepCh1 = WidthStepCh2 * BinsPerDim;
  HistSize = WidthStepCh1 * BinsPerDim;
  HistContributors = 0;

  // Perform allocation
  Hist = new HistType[ HistSize ];
  IsHistValid = true;
  this->Reset();
}

// Class deconstructor (Deallocates hist)
MultiFeatureHist::~MultiFeatureHist()
{
  if( IsHistValid )
  {
    delete[] Hist;
  }
}

// Reset all stored values
void MultiFeatureHist::Reset()
{
  // If histogram is valid, reset it
  if( IsHistValid )
  {
    std::fill( Hist, Hist + HistSize, 0 );
  }

  // Reset summed values
  for( unsigned int i = 0; i < 3; i++ )
  {
    TopAvg[ i ] = 0;
    BotAvg[ i ] = 0;
  }

  // Reset all internal counters
  HistContributors = 0;
  TopWeight = 0.0;
  BotWeight = 0.0;
}

// Fast filter
IplImage * MultiFeatureHist::CreateColorCommonalityImage( IplImage * img, bool cscaling )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsHistValid == true );

  // Create output img
  IplImage *OutputImg = cvCreateImage( cvGetSize( img ), IPL_DEPTH_32F, 1 );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;

  // Points to image contents
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);
  float *OutputPos = reinterpret_cast<float*>(OutputImg->imageData);

  // Cycle through image rows
  if( cscaling )
  {
    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      unsigned char *RowEnd = InputPos + Width * Channels;

      // Cycle through image column
      for( ; InputPos < RowEnd; InputPos += Channels, OutputPos++ )
      {
        // Lookup color value in histogram
        *OutputPos = static_cast<float>(this->ClassifyColor3D( InputPos ));
      }
    }
  }
  else
  {
    double HistWeight = ( TopWeight + BotWeight > 0 ? TopWeight + BotWeight : 0.0001 );

    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      unsigned char *RowEnd = InputPos + Width * Channels;

      // Cycle through image column
      for( ; InputPos < RowEnd; InputPos += Channels, OutputPos++ )
      {
        // Lookup color value in histogram
        *OutputPos = static_cast<float>(this->ClassifyColor3D( InputPos ) ) / HistWeight;
      }
    }
  }

  // Return Output Image
  return OutputImg;
}


void MultiFeatureHist::AddValueToHist( unsigned char* Color, const double& Weight, const bool& InTopHalf )
{
  // Get BGR values
  int b = static_cast<int>(Color[0]);
  int g = static_cast<int>(Color[1]);
  int r = static_cast<int>(Color[2]);

  // Add to histogram
  int Bin1 = b * BinsPerDim / 256;
  int Bin2 = g * BinsPerDim / 256;
  int Bin3 = r * BinsPerDim / 256;
  HistType *HistColorPos = Hist + Bin1*WidthStepCh1 + Bin2*WidthStepCh2 + Bin3;
  *HistColorPos = *HistColorPos + Weight;

  // Add to single value model
  if( InTopHalf )
  {
    TopAvg[0] += r * Weight;
    TopAvg[1] += g * Weight;
    TopAvg[2] += b * Weight;
    TopWeight += Weight;
  }
  else
  {
    BotAvg[0] += r * Weight;
    BotAvg[1] += g * Weight;
    BotAvg[2] += b * Weight;
    BotWeight += Weight;
  }
}

// Create histogram from image
void MultiFeatureHist::UpdateModelFromImage( IplImage *img, IplImage *mask )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsHistValid == true );
  assert( mask->height == img->height );
  assert( mask->width == img->width );

  // Create necessary variables
  int InputStep = img->widthStep;
  int MaskStep = mask->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Divide region into upper and lower sections
  unsigned int VerticalDivide = img->height / 2;

  if( !mask )
  {
    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;

      // Are we in the top or bottom half of the region
      bool InTopHalf = ( r >= VerticalDivide );

      // Cycle through image columns
      for( unsigned int c = 0; c < Width; c++, InputPos += Channels )
      {
        // Calculate weight for position
        AddValueToHist( InputPos, 1.0, InTopHalf );
      }
    }
  }
  else if( mask->depth == IPL_DEPTH_32F )
  {
    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      float* MaskPos = reinterpret_cast<float*>( r * MaskStep + mask->imageData );

      // Are we in the top or bottom half of the region
      bool InTopHalf = ( r >= VerticalDivide );

      // Cycle through image columns
      for( unsigned int c = 0; c < Width; c++, InputPos += Channels, MaskPos++ )
      {
        // Calculate weight for position
        AddValueToHist( InputPos, *MaskPos, InTopHalf );
      }
    }
  }
  else if( mask->depth == IPL_DEPTH_8U )
  {
    unsigned char* MaskStart = reinterpret_cast<unsigned char*>(mask->imageData);

    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      unsigned char* MaskPos = r * MaskStep + MaskStart;

      // Are we in the top or bottom half of the region
      bool InTopHalf = ( r >= VerticalDivide );

      // Cycle through image columns
      for( unsigned int c = 0; c < Width; c++, InputPos += Channels, MaskPos++ )
      {
        // Calculate weight for position
        if( *MaskPos )
        {
          AddValueToHist( InputPos, 1.0, InTopHalf );
        }
      }
    }
  }
  else if( mask->depth == IPL_DEPTH_1U )
  {
    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      bool* MaskPos = reinterpret_cast<bool*>( r * MaskStep + mask->imageData );

      // Are we in the top or bottom half of the region
      bool InTopHalf = ( r >= VerticalDivide );

      // Cycle through image columns
      for( unsigned int c = 0; c < Width; c++, InputPos += Channels, MaskPos++ )
      {
        // Calculate weight for position
        AddValueToHist( InputPos, 1.0, InTopHalf );
      }
    }
  }
  else
  {
    throw std::runtime_error("Invalid Mask Format!");
  }

  // Increment internal counter
  HistContributors++;
}

// Calculate single value style features
void MultiFeatureHist::CalculateSVFeatures( Descriptor& SVFeatures )
{
  // Validate size
  assert( SVFeatures.size() == 6 );

  // Calculate net RGB from partial sums in lower and upper regions
  double TotalWeight = TopWeight + BotWeight;

  if( !TotalWeight )
  {
    return;
  }

  double TopRatio = static_cast<double>(TopWeight) / TotalWeight;
  double BotRatio = static_cast<double>(BotWeight) / TotalWeight;

  double R = 0.0;
  double G = 0.0;
  double B = 0.0;

  if( TopRatio != 0 )
  {
    R += (TopAvg[0]*TopRatio)/TopWeight;
    G += (TopAvg[1]*TopRatio)/TopWeight;
    B += (TopAvg[2]*TopRatio)/TopWeight;
  }
  if( BotRatio != 0 )
  {
    R += (BotAvg[0]*BotRatio)/BotWeight;
    G += (BotAvg[1]*BotRatio)/BotWeight;
    B += (BotAvg[2]*BotRatio)/BotWeight;
  }

  // Bins 1-3: Raw RGB || Lab
  SVFeatures[0] = R;
  SVFeatures[1] = G;
  SVFeatures[2] = B;

  // Bins 4-5: R/G, B/G
  SVFeatures[3] = ( G != 0 ? R / G : R );
  SVFeatures[4] = ( G != 0 ? B / G : B );

  // Bin 6: Hue
  double TopVal = 1.732051 * (G - B);
  double BotVal = 2* R - G - B;
  SVFeatures[5] = ( TopVal != 0 || BotVal != 0 ? std::atan2( TopVal, BotVal ) : 0.33 );
}

// Calculate single value style features
void MultiFeatureHist::CalculateObjFeatures( Descriptor& ObjFeatures )
{
  // Validate size
  assert( ObjFeatures.size() == 8 );

  // Calculate net pixels logged
  double TotalPixelWeight = TopWeight + BotWeight;

  if( TotalPixelWeight == 0.0 )
  {
    return;
  }

  // Bins 0-3 - Top color
  ObjFeatures[0] = 0.65;
  if( TopWeight != 0 )
  {
    ObjFeatures[1] = TopAvg[0] / TopWeight;
    ObjFeatures[2] = TopAvg[1] / TopWeight;
    ObjFeatures[3] = TopAvg[2] / TopWeight;
  }
  else
  {
    ObjFeatures[5] = BotAvg[0] / BotWeight;
    ObjFeatures[6] = BotAvg[1] / BotWeight;
    ObjFeatures[7] = BotAvg[2] / BotWeight;
  }

  // Bins 4-7 - Bottom color
  ObjFeatures[4] = 0.35;
  if( BotWeight == 0 )
  {
    ObjFeatures[5] = TopAvg[0] / TopWeight;
    ObjFeatures[6] = TopAvg[1] / TopWeight;
    ObjFeatures[7] = TopAvg[2] / TopWeight;
  }
  else
  {
    ObjFeatures[5] = BotAvg[0] / BotWeight;
    ObjFeatures[6] = BotAvg[1] / BotWeight;
    ObjFeatures[7] = BotAvg[2] / BotWeight;
  }
}

// Computes descriptors for current point in time
void MultiFeatureHist::DumpOutputs( std::vector<double>& HistVector,
                                    std::vector<double>& ValueVector,
                                    std::vector<double>& ObjVector )
{
  // Check inputs
  assert( HistVector.size() == HistSize );

  // Dump normalized Histogram to Vector
  HistType HistSum = 0;
  for( unsigned i = 0; i < HistSize; i++ )
    HistSum += Hist[i];
  for( unsigned i = 0; i < HistVector.size(); i++ )
    HistVector[i] = static_cast<double>(Hist[i]) / HistSum;

  // Calculate and dump obj vector
  CalculateSVFeatures( ValueVector );

  // Calculate and dump value vector
  CalculateObjFeatures( ObjVector );
}

// Classifies a 8-bit, 3 chan color according to loaded histogram
MultiFeatureHist::HistType MultiFeatureHist::ClassifyColor3D( unsigned char* Color ) {

  int Bin1 = ( BinsPerDim*static_cast<int>(Color[0]) ) / 256;
  int Bin2 = ( BinsPerDim*static_cast<int>(Color[1]) ) / 256;
  int Bin3 = ( BinsPerDim*static_cast<int>(Color[2]) ) / 256;

  return Hist[ Bin1*WidthStepCh1 + Bin2*WidthStepCh2 + Bin3 ];
}

// Smoothes 3D hist with simple kernel
void MultiFeatureHist::SmoothHistogram()
{

  // Create new filter buffer
  HistType *NewHist = new HistType[HistSize];

  // 0 fill new filter buffer
  for( unsigned int i = 0; i < HistSize; i++ )
  {
    NewHist[i] = 0;
  }

  // Semi-Optimized 3D convolution
  // Convolves with simple 3D func =
  /* [ - - - ]  [ - O - ]  [ - - - ]
   * [ - O - ]  [ O I O ]  [ - O - ]
   * [ - - - ]  [ - O - ]  [ - - - ]  */

  // Helper pointers to access points around kernel center
  HistType *P1, *P2, *P3, *P4, *P5, *P6;
  HistType *CenterPos;

  // The last value of the histogram
  HistType *HistEnd = Hist + HistSize - 1;

  // Kernel Parameters
  double InnerAdjustment = 0.400;
  double OuterAdjustment = 0.100;

  // Handle all values
  for( unsigned int i = 0; i < BinsPerDim; i++ )
  {
    for( unsigned int j = 0; j < BinsPerDim; j++ )
    {
      for( unsigned int k = 0; k < BinsPerDim; k++ )
      {

        // Value at the current position in the old histogram (i, j, k)
        int StepToCenter = WidthStepCh1 * i + WidthStepCh2 * j + k;
        double PosValue = *(Hist + StepToCenter);

        // Pointer to current position in the new histogram
        CenterPos = NewHist + StepToCenter;

        // Puts additional weight on inner value
        *CenterPos = *CenterPos + PosValue * InnerAdjustment;

        // Create pointers to 6 points around current location
        P1 = CenterPos - 1;
        P2 = CenterPos + 1;
        P3 = CenterPos - WidthStepCh1;
        P4 = CenterPos + WidthStepCh1;
        P5 = CenterPos - WidthStepCh2;
        P6 = CenterPos + WidthStepCh2;

        double AdjVal = PosValue * OuterAdjustment;

        // Handle side values if they exist
        int ValuesAdded = 0;
        if( P1 >= Hist && P1 <= HistEnd )
        {
          *P1 = *P1 + AdjVal;
          ValuesAdded++;
        }
        if( P2 >= Hist && P2 <= HistEnd )
        {
          *P2 = *P2 + AdjVal;
          ValuesAdded++;
        }
        if( P3 >= Hist && P3 <= HistEnd )
        {
          *P3 = *P3 + AdjVal;
          ValuesAdded++;
        }
        if( P4 >= Hist && P4 <= HistEnd )
        {
          *P4 = *P4 + AdjVal;
          ValuesAdded++;
        }
        if( P5 >= Hist && P5 <= HistEnd )
        {
          *P5 = *P5 + AdjVal;
          ValuesAdded++;
        }
        if( P6 >= Hist && P6 <= HistEnd )
        {
          *P6 = *P6 + AdjVal;
          ValuesAdded++;
        }

        // Put additional weight on center value if on boundary
        *CenterPos = *CenterPos + AdjVal * (6 - ValuesAdded);
      }
    }
  }

  // Deallocate old filter buffer
  delete[] Hist;

  // Set new histogram
  Hist = NewHist;
}

// Filter the histogram, putting less emphasis on darker colors
void MultiFeatureHist::BlackFilter( double base, int polynomial, double distance )
{
  // Calculate correct polynomial filter functor (cx^n + b)
  double c = ( 1.0 - base ) / std::pow( distance, polynomial );

  // Convert distance from normalized coordinates to relative histogram binning frame
  double max_hist_distance = sqrt( 3.0 * ( BinsPerDim * BinsPerDim ) );

  // Filter the histogram
  for( unsigned int i = 0; i < BinsPerDim; i++ )
  {
    for( unsigned int j = 0; j < BinsPerDim; j++ )
    {
      for( unsigned int k = 0; k < BinsPerDim; k++ )
      {
        // Pointer to the current position in the old histogram (i, j, k)
        HistType *Pos = Hist + WidthStepCh1 * i + WidthStepCh2 * j + k;

        // Adjust position by weight if required
        double dist_to_pure_black = std::sqrt( double( i*i + j*j + k*k ) );
        if( dist_to_pure_black < max_hist_distance )
        {
          *Pos *= ( c * std::pow( dist_to_pure_black, polynomial ) + base );
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
//                     Simple Color Histogram Definition
//------------------------------------------------------------------------------


// Class constructor, allocates
Histogram3D::Histogram3D( const int& SideLength )
{
  this->AllocateHistogram( SideLength );
}

// Class constructor, allocates new histogram
Histogram3D::Histogram3D( IplImage* InputImg, const int& SideLength )
{
  this->AllocateHistogram( SideLength );
  this->BuildHistogram( InputImg );
}

// Allocates a new histogram of the specified resolution
void Histogram3D::AllocateHistogram( const int& SideLength )
{

  // Deallocate histogram if we already allocated one
  if( IsValid )
    delete[] Hist;

  // Set required internal variables
  BinsPerDim = SideLength;
  WidthStepCh2 = BinsPerDim;
  WidthStepCh1 = WidthStepCh2 * BinsPerDim;
  HistSize = WidthStepCh1 * BinsPerDim;
  HistContributors = 0;

  // Perform allocation
  Hist = new double[ HistSize ];
  IsValid = true;
  this->ResetHistogram();
}

// Class deconstructor (Deallocates hist)
Histogram3D::~Histogram3D()
{
  if( IsValid )
  {
    delete[] Hist;
  }
}

// Fast filter
IplImage * Histogram3D::CreateColorCommonalityImage( IplImage * img, bool cscaling )
{

  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create output img
  IplImage *OutputImg = cvCreateImage( cvGetSize( img ), IPL_DEPTH_32F, 1 );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;

  // Points to image contents
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);
  float *OutputPos = reinterpret_cast<float*>(OutputImg->imageData);

  // Cycle through image rows
  if( cscaling )
  {
    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      unsigned char *RowEnd = InputPos + Width * Channels;

      // Cycle through image column
      for( ; InputPos < RowEnd; InputPos += Channels, OutputPos++ )
      {
        // Lookup color value in histogram
        *OutputPos = this->ClassifyColor3D( InputPos );
      }
    }
  }
  else
  {
    for( unsigned int r = 0; r < Height; r++ )
    {
      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      unsigned char *RowEnd = InputPos + Width * Channels;

      // Cycle through image column
      for( ; InputPos < RowEnd; InputPos += Channels, OutputPos++ )
      {
        // Lookup color value in histogram
        *OutputPos = this->ClassifyColor3D( InputPos ) / HistContributors;
      }
    }
  }

  // Return Output Image
  return OutputImg;
}

// Fast filter
IplImage * Histogram3D::CreateChannelBinaryImage( IplImage * img, int channel )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create output img
  IplImage *OutputImg = cvCreateImage( cvGetSize( img ), IPL_DEPTH_8U, 1 );

  // Zero image
  cvSetZero( OutputImg );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;

  // Points to image contents
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Cycle through image rows
  for( unsigned int r = 0; r < Height; r++ )
  {
    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart;

    // Cycle through image column
    for( unsigned int c = 0; c < Width; InputPos += Channels, c++ )
    {
      // Lookup color value in histogram
      int Bin1 = ( BinsPerDim*static_cast<int>(InputPos[0]) ) / 256;
      int Bin2 = ( BinsPerDim*static_cast<int>(InputPos[1]) ) / 256;
      int Bin3 = ( BinsPerDim*static_cast<int>(InputPos[2]) ) / 256;

      // Set pixel if required
      if( Bin1*WidthStepCh1 + Bin2*WidthStepCh2 + Bin3 == static_cast<unsigned>(channel) )
      {
        (reinterpret_cast<unsigned char*>(OutputImg->imageData + r * OutputImg->widthStep))[c] = 255;
      }
    }
  }

  // Return Output Image
  return OutputImg;
}

// Set histogram to 0
void Histogram3D::ResetHistogram()
{
  std::fill( Hist, Hist + HistSize, 0 );

  HistContributors = 0;
}

// Negate values in the histogram
void Histogram3D::NegateHistogram()
{

  double *ptr = Hist;
  double *end = Hist + HistSize;

  for( ; ptr != end; ptr++ )
  {
    *ptr = -(*ptr);
  }
}

// Normalize Histogram such that sum(sum(hist)) == 1
void Histogram3D::NormalizeHistogram()
{

  // Variables
  double sum = 0;
  double *ptr = Hist;
  double *end = Hist + HistSize;

  // First pass = calc sum of array
  sum = std::accumulate( ptr, end, 0.0 );

  // Second pass = normalize array
  if( sum != 0 )
  {
    for( ptr = Hist; ptr != end; ptr++ )
    {
      *ptr = *ptr / sum;
    }
  }
}

// Create histogram from image
void Histogram3D::BuildHistogram( IplImage *img )
{
  this->ResetHistogram();
  this->AddImgToHistogram( img, 1.0 );
}

// Create histogram from square region in image
void Histogram3D::BuildHistogram( IplImage *img, const cv::Rect& region )
{
  this->ResetHistogram();
  this->AddRegionToHistogram( img, region, 1.0 );
}

// Create histogram from image
void Histogram3D::AddImgToHistogram( IplImage *img, double weight )
{

  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Assign weight such that after img pass histogram is rel. normalized
  double PtWeight = weight / static_cast<double>( Height * Width );

  // Cycle through image rows
  for( unsigned int r = 0; r < Height; r++ )
  {

    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart;

    // Position to end of row data
    unsigned char *RowEnd = r * InputStep + InputStart + Channels * Width;

    // Cycle through image columns
    for( ; InputPos != RowEnd; InputPos += Channels )
    {
      // Enter weight for point's color into histogram
      int Bin1 = static_cast<int>(InputPos[0]) * BinsPerDim / 256;
      int Bin2 = static_cast<int>(InputPos[1]) * BinsPerDim / 256;
      int Bin3 = static_cast<int>(InputPos[2]) * BinsPerDim / 256;
      double *HistColorPos = Hist + Bin1*WidthStepCh1 + Bin2*WidthStepCh2 + Bin3;
      *HistColorPos = *HistColorPos + PtWeight;
    }
  }

  // Increment internal counter
  HistContributors++;
}

// Create histogram from square region in image
void Histogram3D::AddRegionToHistogram( IplImage *img, const cv::Rect& region, double weight )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Check to make sure dims dont exceed respective boundaries
  unsigned int LowerR = region.y;
  unsigned int UpperR = LowerR + region.height - 1;
  unsigned int LowerC = region.x;
  unsigned int UpperC = LowerC + region.width - 1;

  if( UpperC >= Width )
    UpperC = Width - 1;
  if( UpperR >= Height )
    UpperR = Height - 1;

  // Assign weight such that after img pass histogram is normalized
  int NumOfPixelsInBB = (UpperR - LowerR + 1) * (UpperC - LowerC + 1);
  double PtWeight = weight / static_cast<double>( NumOfPixelsInBB );

  // Cycle through image rows
  for( unsigned int r = LowerR; r <= UpperR; r++ ) {

    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart + LowerC * Channels;
    unsigned char *ColEnd = r * InputStep + InputStart + UpperC * Channels;

    // Cycle through image columns
    for( ; InputPos <= ColEnd; InputPos += Channels )
    {
      // Enter weight for point's color into histogram
      int Bin1 = static_cast<int>(InputPos[0]) * BinsPerDim / 256;
      int Bin2 = static_cast<int>(InputPos[1]) * BinsPerDim / 256;
      int Bin3 = static_cast<int>(InputPos[2]) * BinsPerDim / 256;
      double *HistColorPos = Hist + Bin1*WidthStepCh1 + Bin2*WidthStepCh2 + Bin3;
      *HistColorPos = *HistColorPos + PtWeight;
    }
  }

  // Increment internal counter
  HistContributors++;
}

// Classifies a 8-bit, 3 chan color according to loaded histogram
double Histogram3D::ClassifyColor3D( unsigned char* Color )
{
  int Bin1 = ( BinsPerDim*static_cast<int>(Color[0]) ) / 256;
  int Bin2 = ( BinsPerDim*static_cast<int>(Color[1]) ) / 256;
  int Bin3 = ( BinsPerDim*static_cast<int>(Color[2]) ) / 256;

  return Hist[ Bin1*WidthStepCh1 + Bin2*WidthStepCh2 + Bin3 ];
}

// Copies the histogram stored in the class into a vector (redundant)
void Histogram3D::CopyHistIntoVector( std::vector<double>& InputVector ) const
{
  InputVector.clear();
  InputVector.resize( HistSize );
  for( unsigned int i = 0; i < HistSize; i++ )
  {
    InputVector[i] = Hist[i];
  }
}

// Smoothes 3D hist with simple kernel
void Histogram3D::SmoothHistogram()
{
  // Create new filter buffer
  double *NewHist = new double[HistSize];

  // 0 fill new filter buffer
  for( unsigned int i = 0; i < HistSize; i++ )
  {
    NewHist[i] = 0.0f;
  }

  // Semi-Optimized 3D convolution
  // Convolves with simple 3D func =
  /* [ - - - ]  [ - O - ]  [ - - - ]
   * [ - O - ]  [ O I O ]  [ - O - ]
   * [ - - - ]  [ - O - ]  [ - - - ]  */

  // Helper pointers to access points around kernel center
  double *P1, *P2, *P3, *P4, *P5, *P6;
  double *CenterPos;

  // The last value of the histogram
  double *HistEnd = Hist + HistSize - 1;

  // Kernel Parameters
  double InnerAdjustment = 0.400;
  double OuterAdjustment = 0.100;

  // Handle all values
  for( unsigned int i = 0; i < BinsPerDim; i++ )
  {
    for( unsigned int j = 0; j < BinsPerDim; j++ )
    {
      for( unsigned int k = 0; k < BinsPerDim; k++ )
      {
        // Value at the current position in the old histogram (i, j, k)
        int StepToCenter = WidthStepCh1 * i + WidthStepCh2 * j + k;
        double PosValue = *(Hist + StepToCenter);

        // Pointer to current position in the new histogram
        CenterPos = NewHist + StepToCenter;

        // Puts additional weight on inner value
        *CenterPos = *CenterPos + PosValue * InnerAdjustment;

        // Create pointers to 6 points around current location
        P1 = CenterPos - 1;
        P2 = CenterPos + 1;
        P3 = CenterPos - WidthStepCh1;
        P4 = CenterPos + WidthStepCh1;
        P5 = CenterPos - WidthStepCh2;
        P6 = CenterPos + WidthStepCh2;

        double AdjVal = PosValue * OuterAdjustment;

        // Handle side values if they exist
        int ValuesAdded = 0;
        if( P1 >= Hist && P1 <= HistEnd )
        {
          *P1 = *P1 + AdjVal;
          ValuesAdded++;
        }
        if( P2 >= Hist && P2 <= HistEnd )
        {
          *P2 = *P2 + AdjVal;
          ValuesAdded++;
        }
        if( P3 >= Hist && P3 <= HistEnd )
        {
          *P3 = *P3 + AdjVal;
          ValuesAdded++;
        }
        if( P4 >= Hist && P4 <= HistEnd )
        {
          *P4 = *P4 + AdjVal;
          ValuesAdded++;
        }
        if( P5 >= Hist && P5 <= HistEnd )
        {
          *P5 = *P5 + AdjVal;
          ValuesAdded++;
        }
        if( P6 >= Hist && P6 <= HistEnd )
        {
          *P6 = *P6 + AdjVal;
          ValuesAdded++;
        }

        // Put additional weight on center value if on boundary
        *CenterPos = *CenterPos + AdjVal * (6 - ValuesAdded);
      }
    }
  }

  // Deallocate old filter buffer
  delete[] Hist;

  // Set new histogram
  Hist = NewHist;
}

//------------------------------------------------------------------------------
//                       Intensity Histogram Information
//------------------------------------------------------------------------------

// Class constructor, allocates
IntensityHistogram::IntensityHistogram( const int& SideLength )
{
  this->AllocateHistogram( SideLength );
}

// Class constructor, allocates new histogram
IntensityHistogram::IntensityHistogram( IplImage* InputImg, const int& SideLength )
{
  this->AllocateHistogram( SideLength );
  this->BuildHistogram( InputImg );
}

// Allocates a new histogram of the specified resolution
void IntensityHistogram::AllocateHistogram( const int& SideLength )
{
  // Deallocate histogram if we already allocated one
  if( IsValid )
    delete[] Hist;

  // Set required internal variables
  BinsPerDim = SideLength;
  HistSize = SideLength;
  HistContributors = 0;

  // Perform allocation
  Hist = new double[ HistSize ];
  IsValid = true;
  this->ResetHistogram();
}

// Class deconstructor (Deallocates hist)
IntensityHistogram::~IntensityHistogram()
{
  if( IsValid )
  {
    delete[] Hist;
  }
}

// Fast filter
IplImage * IntensityHistogram::CreateColorCommonalityImage( IplImage * img, bool cscaling )
{

  // Check input type
  assert( img->nChannels == 1 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create output img
  IplImage *OutputImg = cvCreateImage( cvGetSize( img ), IPL_DEPTH_32F, 1 );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;

  // Points to image contents
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);
  float *OutputPos = reinterpret_cast<float*>(OutputImg->imageData);

  // Cycle through image rows
  if( cscaling )
  {
    for( unsigned int r = 0; r < Height; r++ )
    {

      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      unsigned char *RowEnd = InputPos + Width * Channels;

      // Cycle through image column
      for( ; InputPos < RowEnd; InputPos += Channels, OutputPos++ )
      {
        // Lookup color value in histogram
        *OutputPos = this->ClassifyIntensity( InputPos );
      }
    }
  }
  else
  {
    for( unsigned int r = 0; r < Height; r++ )
    {

      // Position to start of row data
      unsigned char *InputPos = r * InputStep + InputStart;
      unsigned char *RowEnd = InputPos + Width * Channels;

      // Cycle through image column
      for( ; InputPos < RowEnd; InputPos += Channels, OutputPos++ )
      {
        // Lookup color value in histogram
        *OutputPos = this->ClassifyIntensity( InputPos ) / HistContributors;
      }
    }
  }

  // Return Output Image
  return OutputImg;
}

// Set histogram to 0
void IntensityHistogram::ResetHistogram()
{
  std::fill( Hist, Hist + HistSize, 0 );
  HistContributors = 0;
}

// Negate values in the histogram
void IntensityHistogram::NegateHistogram()
{
  double *ptr = Hist;
  double *end = Hist + HistSize;

  for( ; ptr != end; ptr++ )
  {
    *ptr = -(*ptr);
  }
}

// Normalize Histogram such that sum(sum(hist)) == 1
void IntensityHistogram::NormalizeHistogram()
{
  // Variables
  double sum = 0;
  double *ptr = Hist;
  double *end = Hist + HistSize;

  // First pass = calc sum of array
  sum = std::accumulate( ptr, end, 0.0 );

  // Second pass = normalize array
  if( sum != 0 )
  {
    for( ptr = Hist; ptr != end; ptr++ )
    {
      *ptr = *ptr / sum;
    }
  }
}

// Create histogram from image
void IntensityHistogram::BuildHistogram( IplImage *img )
{
  this->ResetHistogram();
  this->AddImgToHistogram( img, 1.0 );
}

// Create histogram from square region in image
void IntensityHistogram::BuildHistogram( IplImage *img, const cv::Rect& region )
{
  this->ResetHistogram();
  this->AddRegionToHistogram( img, region, 1.0 );
}

// Create histogram from image
void IntensityHistogram::AddImgToHistogram( IplImage *img, double weight )
{
  // Check input type
  assert( img->nChannels == 1 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Assign weight such that after img pass histogram is rel. normalized
  double PtWeight = weight / static_cast<double>( Height * Width );

  // Cycle through image rows
  for( unsigned int r = 0; r < Height; r++ )
  {
    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart;

    // Position to end of row data
    unsigned char *RowEnd = r * InputStep + InputStart + Channels * Width;

    // Cycle through image columns
    for( ; InputPos != RowEnd; InputPos += Channels )
    {
      // Enter weight for point's color into histogram
      int Bin = static_cast<int>(InputPos[0]) * BinsPerDim / 256;
      double *HistColorPos = Hist + Bin;
      *HistColorPos = *HistColorPos + PtWeight;
    }
  }

  // Increment internal counter
  HistContributors++;
}

// Create histogram from square region in image
void IntensityHistogram::AddRegionToHistogram( IplImage *img, const cv::Rect& region, double weight )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Check to make sure dims dont exceed respective boundaries
  unsigned int LowerR = region.y;
  unsigned int UpperR = LowerR + region.height - 1;
  unsigned int LowerC = region.x;
  unsigned int UpperC = LowerC + region.width - 1;

  if( UpperC >= Width )
    UpperC = Width - 1;
  if( UpperR >= Height )
    UpperR = Height - 1;

  // Assign weight such that after img pass histogram is normalized
  int NumOfPixelsInBB = (UpperR - LowerR + 1) * (UpperC - LowerC + 1);
  double PtWeight = weight / static_cast<double>( NumOfPixelsInBB );

  // Cycle through image rows
  for( unsigned int r = LowerR; r <= UpperR; r++ )
  {
    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart + LowerC * Channels;
    unsigned char *ColEnd = r * InputStep + InputStart + UpperC * Channels;

    // Cycle through image columns
    for( ; InputPos <= ColEnd; InputPos += Channels )
    {
      // Enter weight for point's color into histogram
      int Bin = static_cast<int>(InputPos[0]) * BinsPerDim / 256;
      double *HistColorPos = Hist + Bin;
      *HistColorPos = *HistColorPos + PtWeight;
    }
  }

  // Increment internal counter
  HistContributors++;
}

// Classifies a 8-bit, 3 chan color according to loaded histogram
double IntensityHistogram::ClassifyIntensity( unsigned char* Color )
{
  int Bin = ( BinsPerDim*static_cast<int>(Color[0]) ) / 256;
  return Hist[ Bin ];
}

// Copies the histogram stored in the class into a vector (redundant)
void IntensityHistogram::CopyHistIntoVector( std::vector<double>& InputVector ) const
{
  InputVector.clear();
  InputVector.resize( HistSize );
  for( unsigned int i = 0; i < HistSize; i++ )
  {
    InputVector[i] = Hist[i];
  }
}

// Smoothes 3D hist with simple kernel
void IntensityHistogram::SmoothHistogram()
{
  // Create new filter buffer
  double *NewHist = new double[HistSize];

  // 0 fill new filter buffer
  std::fill( NewHist, NewHist + HistSize, 0.0f );

  // The last value of the histogram
  double *HistEnd = Hist + HistSize - 1;

  // Points used in operation
  double *P1, *P2;
  double *CenterPos;

  // Kernel Parameters
  double InnerAdjustment = 0.600;
  double OuterAdjustment = 0.200;

  // Handle all values
  for( unsigned int i = 0; i < BinsPerDim; i++ )
  {
    // Value at the current position in the old histogram (i, j, k)
    double PosValue = *(Hist + i);

    // Pointer to current position in the new histogram
    CenterPos = NewHist + i;

    // Puts additional weight on inner value
    *CenterPos = *CenterPos + PosValue * InnerAdjustment;

    // Create pointers to 2 points around current location
    P1 = CenterPos - 1;
    P2 = CenterPos + 1;

    double AdjVal = PosValue * OuterAdjustment;

    // Handle side values if they exist
    int ValuesAdded = 0;
    if( P1 >= Hist && P1 <= HistEnd )
    {
      *P1 = *P1 + AdjVal;
      ValuesAdded++;
    }
    if( P2 >= Hist && P2 <= HistEnd )
    {
      *P2 = *P2 + AdjVal;
      ValuesAdded++;
    }

    // Put additional weight on center value if on boundary
    *CenterPos = *CenterPos + AdjVal * (2 - ValuesAdded);

  }

  // Deallocate old filter buffer
  delete[] Hist;

  // Set new histogram
  Hist = NewHist;
}


//------------------------------------------------------------------------------
//                         Spectral Hist Class Definition
//------------------------------------------------------------------------------

//Class constructor
SpectralHistogram::SpectralHistogram( const int& ResolutionPerChan )
{
  this->AllocateHistograms( ResolutionPerChan );
}

//Class constructor
SpectralHistogram::SpectralHistogram( IplImage* img, const int& ResolutionPerChan )
{
  this->AllocateHistograms( ResolutionPerChan );
  this->BuildHistograms( img );
}

// Allocates a new histogram of the specified resolution
void SpectralHistogram::AllocateHistograms( const int& ResolutionPerChan )
{

  // Deallocate histogram if we already allocated one
  if( IsValid )
  {
    delete[] HistCh1;
    delete[] HistCh2;
    delete[] HistCh3;
  }

  // Set required internal variables
  HistSize = ResolutionPerChan;

  // Perform allocation
  HistCh1 = new double[ HistSize ];
  HistCh2 = new double[ HistSize ];
  HistCh3 = new double[ HistSize ];

  // Set as valid and reset
  IsValid = true;
  this->ResetHistograms();
}

// Class deconstructor (Deallocates hist)
SpectralHistogram::~SpectralHistogram()
{
  if( HistCh1 != NULL && IsValid )
  {
    delete[] HistCh1;
    delete[] HistCh2;
    delete[] HistCh3;
  }
}

// Fast filter
IplImage *SpectralHistogram::CreateColorCommonalityImage( IplImage *img )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Create output img
  IplImage *OutputImg = cvCreateImage( cvGetSize( img ), IPL_DEPTH_32F, 1 );

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = OutputImg->height;
  unsigned int Width = OutputImg->width;
  unsigned int Channels = OutputImg->nChannels;

  // Points to image contents
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);
  double *OutputPos = reinterpret_cast<double*>(OutputImg->imageData);

  // Cycle through image rows
  for( unsigned int r = 0; r < Height; r++ )
  {
    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart;
    unsigned char *RowEnd = InputPos + Width * Channels;

    // Cycle through image column
    for( ; InputPos < RowEnd; InputPos += Channels, OutputPos++ )
    {
      // Lookup color value in histogram
      *OutputPos = this->ClassifyColor3D( InputPos );
    }
  }

  // Return Output Image
  return OutputImg;
}

// Set histogram to 0
void SpectralHistogram::ResetHistograms()
{
  for( unsigned int i = 0; i < HistSize; i++ )
  {
    HistCh1[i] = 0.0;
    HistCh2[i] = 0.0;
    HistCh3[i] = 0.0;
  }
}

// Negate values in the histogram
void SpectralHistogram::NegateHistograms()
{
  for( unsigned int i = 0; i < HistSize; i++ )
  {
    HistCh1[i] = -HistCh1[i];
    HistCh2[i] = -HistCh2[i];
    HistCh3[i] = -HistCh3[i];
  }
}

// Normalize Histogram such that sum(sum(hist)) == 1
void SpectralHistogram::NormalizeHistograms()
{
  // Ch1
  this->NormalizeHistogram( HistCh1, HistSize );

  // Ch2
  this->NormalizeHistogram( HistCh2, HistSize );

  // Ch3
  this->NormalizeHistogram( HistCh3, HistSize );

}

// Normalize a histogram
void SpectralHistogram::NormalizeHistogram( double* input, unsigned int size )
{
  // Variables
  double sum = 0;
  double *ptr = input;
  double *end = input + size;

  // First pass = calc sum of array
  sum = std::accumulate( ptr, end, 0.0 );

  // Second pass = normalize array
  if( sum != 0 )
  {
    for( ptr = input; ptr != end; ptr++ )
    {
      *ptr = *ptr / sum;
    }
  }
}

// Create histogram from image
void SpectralHistogram::BuildHistograms( IplImage *img )
{
  this->ResetHistograms();
  this->AddImgToHistograms( img, 1.0 );
}

// Create histogram from square region in image
void SpectralHistogram::BuildHistograms( IplImage *img, const cv::Rect& region )
{
  this->ResetHistograms();
  this->AddRegionToHistograms( img, region, 1.0 );
}

// Create histogram from image
void SpectralHistogram::AddImgToHistograms( IplImage *img, double weight )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Zero current histogram
  this->ResetHistograms();

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Assign weight such that after img pass histogram is rel. normalized
  double PtWeight = weight / static_cast<double>( Height * Width );

  // Cycle through image rows
  for( unsigned int r = 0; r < Height; r++ ) {

    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart;

    // Position to end of row data
    unsigned char *RowEnd = r * InputStep + InputStart + Channels * Width;

    // Cycle through image columns
    for( ; InputPos != RowEnd; InputPos += Channels ) {

      // Enter weight for point's color into histogram
      int Bin1 = static_cast<int>(InputPos[0]) * HistSize / 256;
      int Bin2 = static_cast<int>(InputPos[1]) * HistSize / 256;
      int Bin3 = static_cast<int>(InputPos[2]) * HistSize / 256;

      HistCh1[Bin1] += PtWeight;
      HistCh2[Bin2] += PtWeight;
      HistCh3[Bin3] += PtWeight;
    }
  }
}

// Create histogram from square region in image
void SpectralHistogram::AddRegionToHistograms( IplImage *img, const cv::Rect& region, double weight )
{
  // Check input type
  assert( img->nChannels == 3 );
  assert( img->depth == IPL_DEPTH_8U );
  assert( IsValid == true );

  // Zero current histogram
  this->ResetHistograms();

  // Create necessary variables
  int InputStep = img->widthStep;
  unsigned int Height = img->height;
  unsigned int Width = img->width;
  unsigned int Channels = img->nChannels;
  unsigned char *InputStart = reinterpret_cast<unsigned char*>(img->imageData);

  // Check to make sure dims dont exceed respective boundaries
  unsigned int LowerR = region.y;
  unsigned int UpperR = LowerR + region.height - 1;
  unsigned int LowerC = region.x;
  unsigned int UpperC = LowerC + region.width - 1;

  if( UpperC >= Width )
    UpperC = Width - 1;
  if( UpperR >= Height )
    UpperR = Height - 1;

  // Assign weight such that after img pass histogram is normalized
  int NumOfPixelsInBB = (UpperR - LowerR + 1) * (UpperC - LowerC + 1);
  double PtWeight = weight / static_cast<double>( NumOfPixelsInBB );

  // Cycle through image rows
  for( unsigned int r = LowerR; r <= UpperR; r++ )
  {
    // Position to start of row data
    unsigned char *InputPos = r * InputStep + InputStart + LowerC * Channels;
    unsigned char *ColEnd = r * InputStep + InputStart + UpperC * Channels;

    // Cycle through image columns
    for( ; InputPos <= ColEnd; InputPos += Channels )
    {
      // Enter weight for point's color into histogram
      int Bin1 = static_cast<int>(InputPos[0]) * HistSize / 256;
      int Bin2 = static_cast<int>(InputPos[1]) * HistSize / 256;
      int Bin3 = static_cast<int>(InputPos[2]) * HistSize / 256;

      HistCh1[Bin1] += PtWeight;
      HistCh2[Bin2] += PtWeight;
      HistCh3[Bin3] += PtWeight;
    }
  }
}

// Classifies a 8-bit, 3 chan color according to loaded histogram
double SpectralHistogram::ClassifyColor3D( unsigned char* Color )
{
  int Bin1 = ( HistSize*static_cast<int>(Color[0]) ) / 256;
  int Bin2 = ( HistSize*static_cast<int>(Color[1]) ) / 256;
  int Bin3 = ( HistSize*static_cast<int>(Color[2]) ) / 256;

  return HistCh1[ Bin1 ] * HistCh2[ Bin2 ] * HistCh3[ Bin3 ];
}

// Copies the histogram stored in the class into a vector (redundant)
void SpectralHistogram::CopyHistsIntoVector( std::vector<double>& InputVector ) const
{
  InputVector.clear();
  InputVector.resize( HistSize * 3 );
  for( unsigned int i = 0; i < HistSize; i++ )
  {
    InputVector[i] = HistCh1[i];
    InputVector[HistSize+i] = HistCh2[i];
    InputVector[2*HistSize+i] = HistCh3[i];
  }
}

// Smoothes hist with simple kernel
void SpectralHistogram::SmoothHistograms()
{
}


} // end namespace vidtk
