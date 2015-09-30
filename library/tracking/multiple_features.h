/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_multiple_features_h_
#define vidtk_multiple_features_h_

#include <vcl_iostream.h>
#include <vnl/vnl_matrix_fixed.h>

namespace vidtk
{

enum Multiple_Feature_Types{ VIDTK_COLOR, 
                             VIDTK_KINEMATICS, 
                             VIDTK_AREA };

//Multi-feature (mf) parameters: Using color, area and kinemaatics 
//in the cost function for correspondence between objects and tracks.
class multiple_features
{
public:
  multiple_features();

  ~multiple_features();

  double get_similarity_score( double in_prob,  
                               Multiple_Feature_Types idx ) 
                               const;

  bool load_cdfs_params( vcl_string cdfs_filename );

  //Only for track_init_process
  double tangential_sigma;
  double normal_sigma;

  bool test_enabled;
  bool train_enabled;

  double w_color;
  double w_kinematics;
  double w_area;

  double pdf_delta;
  double min_prob_for_log;

  //TODO:Move this param into the cdfs file.
  unsigned bin_count; 
  vnl_matrix<double> cdfs; 

  vcl_string train_filename;
}; //class multiple_features


} //namespace vidtk

#endif //vidtk_multiple_features_h_
