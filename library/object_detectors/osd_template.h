/*ckwg +5
 * Copyright 2012-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_osd_template_
#define vidtk_osd_template_

#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_point_2d.h>
#include <vgl/vgl_intersection.h>

#include <vil/vil_image_view.h>
#include <vil/algo/vil_structuring_element.h>
#include <vil/algo/vil_binary_dilate.h>
#include <vil/algo/vil_blob.h>

#include <vector>
#include <map>

namespace vidtk
{

/// \file This file contains helper objects which jointly define an
/// On-Screen Display (OSD) template, which describes specific features of
/// an individual OSD in order to identify when the OSD is present in video.


/// \brief The storage unit for an OSD "symbol", which includes information
/// detailing, at a high level, an single sub-component which may be a part of
/// some OSD template.
class osd_symbol
{

public:

  osd_symbol() {}
  virtual ~osd_symbol() {}

  enum feature_type
  {
    CROSSHAIR = 0,
    BRACKETS,
    FILLED_RECTANGLE,
    UNFILLED_RECTANGLE,
    TEXT,
    BAR,
    CIRCLE,
    UNKNOWN
  };

  enum feature_location
  {
    TOP_LEFT = 0,
    TOP_CENTER,
    TOP_RIGHT,
    LEFT,
    CENTER,
    RIGHT,
    BOT_LEFT,
    BOT_CENTER,
    BOT_RIGHT,
    ANYWHERE
  };

  /// Symbol ID
  std::string id_;

  /// The type of this feature
  feature_type symbol_;

  /// The expected general location for this feature if specified
  vgl_box_2d< double > region_;

  /// The expected number of blobs this feature will show up as
  unsigned int blob_count_;

  /// The expected size range of this feature relative to the total image area
  double lower_size_;
  double upper_size_;

  /// Weight of this individual feature
  double weight_;

  /// The most common color in the object (if specified)
  vil_rgb<double> color_;

  /// Parse the values in a string defining a symbol
  bool parse_symbol_string( const std::string& str );
};


/// \brief Container detailing very basic relationships between two symbols
class osd_symbol_relationship
{

public:

  osd_symbol_relationship() {}
  virtual ~osd_symbol_relationship() {}

  enum spatial_relationship
  {
    IS_LEFT_OF = 0,
    IS_RIGHT_OF,
    IS_ABOVE,
    IS_BELOW
  };

  std::string sym1_;
  std::string sym2_;
  spatial_relationship rel_;
  double weight_;

  /// Parse the values in a string defining a relationship
  bool parse_relationship_string( const std::string& str );

};


/// \brief Some action to be performed in the event that we detect a specific
/// OSD template.
///
/// Each template may have associated actions. All colors are represented in
/// the [0..1] range, and all sizes/image locations are normalized w.r.t. the
/// height/width of templates in order to make them not be dependant upon image
/// resolution or image format.
class osd_action_item
{

public:

  osd_action_item()
  : blinker_( false ),
    param1_( 0.0 ),
    param2_( 0.0 ),
    param3_( 0 ),
    param4_( 0.0 ),
    node_id_( 1 )
  {}

  virtual ~osd_action_item() {}

  enum action_type
  {
    FILL_RECTANGLE, // Fill in a rectangle in the binary output image with 1s
    UNFILL_RECTANGLE, // Fill in a rectangle in the binary output image with 0s
    REMOVE_UNKNOWNS, // Do not report any objects in the output that we cant recognize
    REMOVE_ANYTHING_OFFCOLOR, // Remove anything that does not have the given color
    RECLASSIFY_IMAGE, // Use some secondary classifier in place of the initial one
    RECLASSIFY_REGION, // Use some secondary template-specific classifier in some region
    CLASSIFY_REGION, // Use some secondary template-specific classifier in some region
    UNION_MASK, // Union all detections with some pre-annotated mask given in an external image
    INTERSECT_MASK, // Add all entries in some static mask to the original detections when enabled
    USE_MASK, // Use some static mask when this template is detected
    XHAIR_SELECT, // A specialized option for specifying multiple classifiers which define xhairs
    ERODE, // Perform a binary erode operation on pixels in the output mask in some region
    DILATE, // Perform a binary dilate operation on pixels in the output mask in some region
    USE_TEMPLATE_MATCHING, // Use default pipeline template matching components for this OSD
    PARSE_TEXT, // Parse text or a number comprised of multiple digits, at some location
    CLASSIFY_BLOBS, // Classify all blobs in some region as either belonging to the OSD or not
    COMPONENT_SEARCH, // Perform a pixel classification followed by a blob classification
    ADD_COLOR_TM_FEAT // Triggers the use of this color for template matching and refinement
  };

  /// Parse the values in a string defining a symbol
  bool parse_action_string( const std::string& str );

  /// Rectangle storage, if used
  vgl_box_2d< double > rect_;

  /// Color and color tolerance, if used
  vil_rgb< double > color_;
  vil_rgb< double > tolerance_;

  /// Action we want to perform
  action_type action_;

  /// Classifier or mask filenames, if used
  std::string key_;

  /// Used if our action requires more than 1 classifier
  std::string key2_;

  /// Does this action only occur on a blinker trigger?
  bool blinker_;

  /// Blinker trigger region, if used
  vgl_box_2d< double > blinker_region_;

  /// Blinker trigger threshold (the region must contain this percentage of OSD pixels...)
  double blinker_per_threshold_;

  /// Action specific parameter, e.g., classifier threshold, dilation amount...
  double param1_;

  /// Action specific parameter, e.g., size threshold
  double param2_;

  /// Action specific parameter, e.g., special sub-type field
  unsigned param3_;

  /// Action specific parameter, e.g., moving average window size, if specified
  double param4_;

  /// ID of the node we want this action to run on, if specified
  unsigned node_id_;
};


/// \brief The storage unit for a mask "template", which includes information
/// describing different known on-screen display.
///
/// This class is used in order to perform any special actions in the event
/// that we detect a specific OSD.
class osd_template
{

public:

  /// Default constructor.
  osd_template()
    : any_resolution_( false ),
      wide_only_( false ),
      any_color_( false ),
      is_default_( false ),
      force_use_( false )
  {}

  /// Constructor given a template identifier.
  osd_template( const std::string& _name )
    : name_( _name ),
      any_resolution_( false ),
      wide_only_( false ),
      any_color_( false ),
      is_default_( false ),
      force_use_( false )
  {}

  /// Destructor.
  virtual ~osd_template() {}

  /// Can this template be used with OSDs of any color?
  void set_any_color_flag( bool val );

  /// Can this template be used at any video resolutions?
  void set_any_resolution_flag( bool val );

  /// Can this template only be used for wide (above 4:3 aspect ratio) data?
  void set_wide_only_flag( bool val );

  /// Add a known resolution requirement to this template.
  bool add_resolution( const unsigned& ni,
                       const unsigned& nj );

  /// Add a known symbol (a required feature) to this template.
  bool add_feature( const osd_symbol& symbol );

  /// Add a relationship to this template.
  bool add_relationship( const osd_symbol_relationship& rel );

  /// Add a known color requirement to this template.
  bool add_color( const vil_rgb<double>& color,
                  const vil_rgb<double>& tolerance = vil_rgb<double>( 0, 0, 0 ) );

  /// Add an associated action item to this template.
  bool add_action( const osd_action_item& action );

  /// Add a classifier to this template.
  bool add_classifier( const std::string& identifier );

  /// Is this template possibly a subset of another template?
  double possibly_contains( const osd_template& other );

  /// Return the identifier of this template.
  std::string const& get_name() const;

  /// Return a list of action items associated with this template.
  std::vector< osd_action_item > const& get_actions() const;

  /// Return a list of symbols associated with this template.
  std::vector< osd_symbol > const& get_symbols() const;

  /// Return the identifiers of all classifiers in this template.
  std::vector< std::string > const& get_classifiers() const;

  /// Check whether or not an image of the given resolution can contain this OSD.
  bool check_resolution( const unsigned ni, const unsigned nj ) const;

  /// Does this template have any classifiers?
  bool has_classifier() const;

  /// Is the force flag set for this template?
  bool force_flag_set() const;

  /// Is this the default template to use in the event that none are detected?
  void set_default_flag();

  /// Set the force use flag, a debugging option which causes the actions in
  /// this template to always be used.
  void set_force_flag();

private:

  /// Name of this template
  std::string name_;

  /// Vector of potential resolutions this mask template is known to occur at
  bool any_resolution_;
  bool wide_only_;
  std::vector< unsigned > ni_;
  std::vector< unsigned > nj_;

  /// Vector of symbols this mask template always contains
  std::vector< osd_symbol > sym_list_;

  /// Vector of actions to perform if we detect this template
  std::vector< osd_action_item > action_list_;

  /// Vector of relationships between symbols
  std::vector< osd_symbol_relationship > rel_list_;

  /// Vector of possible mask colors
  bool any_color_;
  std::vector< vil_rgb< double > > color_list_;
  std::vector< vil_rgb< double > > tolerance_list_;

  /// Vector of hud type classifier identifiers for this template
  std::vector< std::string > classifiers_;

  /// Is this the default template to use if we detect no others
  bool is_default_;

  /// Should we force the use of this template?
  bool force_use_;

};


// Helper functions:

/// Do the two symbols possibly match?
bool can_symbols_match( const osd_symbol& sym1,
                        const osd_symbol& sym2 );


/// Convert a blob given by a vil_blob_pixel_list to an unknown feature
/// which can be compared against some template
bool convert_blob_to_symbol( const unsigned int& ni,
                             const unsigned int& nj,
                             const vil_blob_pixel_list& blob,
                             osd_symbol& sym );


/// Load some templates file to a vector of templates
bool read_templates_file( const std::string& filename,
                          std::vector< osd_template >& templates );

} // namespace vidtk

#endif // vidtk_utilities_osd_template_
