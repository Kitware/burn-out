/*ckwg +5
 * Copyright 2013-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "osd_template.h"

#include <boost/config.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <cmath>
#include <limits>
#include <fstream>

#include <logger/logger.h>

namespace vidtk
{


VIDTK_LOGGER( "osd_template_cxx" );


// ------------------------ SYMBOL DEFINITION --------------------------


// Parse an exact image location given an array of 4 strings
inline vgl_box_2d<double>
parse_exact_location( const std::string* input )
{
  return vgl_box_2d<double>( boost::lexical_cast<double>( input[0] ),
                             boost::lexical_cast<double>( input[2] ),
                             boost::lexical_cast<double>( input[1] ),
                             boost::lexical_cast<double>( input[3] ) );
}


// Parse an old relative image location
inline vgl_box_2d<double>
parse_relative_location( const std::string& input )
{
  osd_symbol::feature_location rel_loc = osd_symbol::ANYWHERE;

  if( input == "CENTER" )              rel_loc = osd_symbol::CENTER;
  else if( input == "TOP_CENTER" )     rel_loc = osd_symbol::TOP_CENTER;
  else if( input == "TOP_LEFT" )       rel_loc = osd_symbol::TOP_LEFT;
  else if( input == "TOP_RIGHT" )      rel_loc = osd_symbol::TOP_RIGHT;
  else if( input == "RIGHT" )          rel_loc = osd_symbol::RIGHT;
  else if( input == "LEFT" )           rel_loc = osd_symbol::LEFT;
  else if( input == "BOT_CENTER" )     rel_loc = osd_symbol::BOT_CENTER;
  else if( input == "BOT_LEFT" )       rel_loc = osd_symbol::BOT_LEFT;
  else if( input == "BOT_RIGHT" )      rel_loc = osd_symbol::BOT_RIGHT;

  if( rel_loc >= osd_symbol::ANYWHERE )
  {
    return vgl_box_2d<double>( 0.0, 1.0, 0.0, 1.0 );
  }

  const double i = (static_cast<int>(rel_loc) % 3) / 3.0;
  const double j = (static_cast<int>(rel_loc) / 3) / 3.0;

  return vgl_box_2d<double>( i, i + 1.0/3.0, j, j + 1.0/3.0 );
}


bool
osd_symbol
::parse_symbol_string( const std::string& str )
{
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> delim(" ()\n");
  tokenizer tokens( str, delim );
  std::vector< std::string > parsed;
  parsed.assign(tokens.begin(),tokens.end());

  if( parsed.size() < 10 )
  {
    return false;
  }

  if( parsed[0] != "SYMBOL" )
  {
    return false;
  }

  id_ = parsed[1];

  if     ( parsed[2] == "CROSSHAIR" )           symbol_ = CROSSHAIR;
  else if( parsed[2] == "BRACKETS" )            symbol_ = BRACKETS;
  else if( parsed[2] == "FILLED_RECTANGLE" )    symbol_ = FILLED_RECTANGLE;
  else if( parsed[2] == "UNFILLED_RECTANGLE" )  symbol_ = UNFILLED_RECTANGLE;
  else if( parsed[2] == "TEXT" )                symbol_ = TEXT;
  else if( parsed[2] == "BAR" )                 symbol_ = BAR;
  else if( parsed[2] == "CIRCLE" )              symbol_ = CIRCLE;
  else                                          symbol_ = UNKNOWN;

  if( parsed[3] != "SIZE" )
  {
    return false;
  }

  lower_size_ = boost::lexical_cast<double>( parsed[4] );
  upper_size_ = boost::lexical_cast<double>( parsed[5] );

  if( parsed[6] != "WEIGHT" )
  {
    return false;
  }

  weight_ = boost::lexical_cast<double>( parsed[7] );

  if( parsed[8] == "RELATIVE_LOCATION" )
  {
    region_ = parse_relative_location( parsed[7] );
  }
  else if( parsed.size() < 13 || parsed[8] != "LOCATION" )
  {
    return false;
  }
  else
  {
    region_ = parse_exact_location( &parsed[9] );
  }

  return true;
}


// Might two symbols possibly match given their observed properties?
bool
can_symbols_match( const osd_symbol& sym1, const osd_symbol& sym2 )
{
  // Position matching
  if( sym1.region_.area() == 0 || sym2.region_.area() == 0 )
  {
    return false;
  }

  // Calculate intersection between locations
  vgl_box_2d<double> intersect = vgl_intersection( sym1.region_, sym2.region_ );
  double percent_overlap1 = static_cast<double>(intersect.area()) / sym1.region_.area();
  double percent_overlap2 = static_cast<double>(intersect.area()) / sym2.region_.area();

  // Two symbols must overlap by at least 50% to be considered the same
  if( percent_overlap1 < 0.5 && percent_overlap2 < 0.5 )
  {
    return false;
  }

  // Type matching
  if( sym1.symbol_ != osd_symbol::UNKNOWN &&
      sym2.symbol_ != osd_symbol::UNKNOWN &&
      sym1.symbol_ != sym2.symbol_ )
  {
    return false;
  }

  // Size matching
  if( sym1.lower_size_ > sym2.upper_size_ )
  {
    return false;
  }

  if( sym1.upper_size_ < sym2.lower_size_ )
  {
    return false;
  }

  return true;
}


bool
osd_symbol_relationship
::parse_relationship_string( const std::string& str )
{
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> delim(" ()\n");
  tokenizer tokens( str, delim );
  std::vector< std::string > parsed;
  parsed.assign(tokens.begin(),tokens.end());

  if( parsed.size() != 6 )
  {
    return false;
  }

  if( parsed[0] != "RELATIONSHIP" )
  {
    return false;
  }

  sym1_ = parsed[1];

  if( parsed[2] == "LEFT_OF" )        rel_ = IS_LEFT_OF;
  else if( parsed[2] == "RIGHT_OF" )  rel_ = IS_RIGHT_OF;
  else if( parsed[2] == "ABOVE" )     rel_ = IS_ABOVE;
  else if( parsed[2] == "BELOW" )     rel_ = IS_BELOW;
  else return false;

  sym2_ = parsed[3];

  if( parsed[4] != "WEIGHT" )
  {
    return false;
  }

  weight_ = boost::lexical_cast<double>( parsed[5] );

  return true;
}


// ------------------------ ACTION DEFINITION --------------------------


bool
osd_action_item
::parse_action_string( const std::string& str )
{
  // Parse line
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> delim(" ()\n");
  tokenizer tokens( str, delim );
  std::vector< std::string > parsed;
  parsed.assign(tokens.begin(),tokens.end());

  // Threshold initial size
  if( parsed.size() < 2 )
  {
    return false;
  }

  // Special pre-opt for actions which only trigger on blinking components
  if( parsed[0] == "BLINKER_TRIGGER" )
  {
    if( parsed.size() < 8 )
    {
      return false;
    }

    std::string blinker_cmd;

    for( unsigned i = 7; i < parsed.size(); i++ )
    {
      blinker_cmd = blinker_cmd + parsed[i] + " ";
    }
    if( !this->parse_action_string( blinker_cmd ) )
    {
      return false;
    }

    blinker_per_threshold_ = boost::lexical_cast<double>( parsed[1] );

    if( blinker_per_threshold_ > 1.0 )
    {
      blinker_per_threshold_ /= 100;
    }

    blinker_region_ =  parse_exact_location( &parsed[3] );
    blinker_ = true;
    return true;
  }

  // Perform action-specific parsing
  if( parsed[0].substr( 0, 11 ) == "ACTION_NODE" )
  {
    node_id_ = boost::lexical_cast<unsigned>( parsed[0].substr( 11, std::string::npos ) );
  }
  else if( parsed[0] != "ACTION" )
  {
    return false;
  }

  if( parsed[1] == "UNFILL" )
  {
    if( parsed.size() != 7 )
    {
      return false;
    }

    action_ = UNFILL_RECTANGLE;
    rect_ = parse_exact_location( &parsed[3] );
  }
  else if( parsed[1] == "FILL" )
  {
    if( parsed.size() != 7 )
    {
      return false;
    }

    action_ = FILL_RECTANGLE;
    rect_ = parse_exact_location( &parsed[3] );
  }
  else if( parsed[1] == "REMOVE_OFF_COLOR" )
  {
    action_ = REMOVE_ANYTHING_OFFCOLOR;

    if( parsed.size() != 6 )
    {
      return false;
    }

    tolerance_.r = boost::lexical_cast<double>( parsed[3] );
    tolerance_.g = boost::lexical_cast<double>( parsed[4] );
    tolerance_.b = boost::lexical_cast<double>( parsed[5] );
  }
  else if( parsed[1] == "REMOVE_UNKNOWNS" )
  {
    action_ = REMOVE_UNKNOWNS;
  }
  else if( parsed[1] == "RECLASSIFY_IMAGE"  )
  {
    if( parsed.size() != 4 )
    {
      return false;
    }

    action_ = RECLASSIFY_IMAGE;
    key_ = parsed[3];
  }
  else if( parsed[1] == "RECLASSIFY_REGION"  )
  {
    if( parsed.size() != 9 && !( parsed.size() >= 11 && parsed.size() < 15 ) )
    {
      return false;
    }

    action_ = RECLASSIFY_REGION;
    key_ = parsed[3];
    rect_ = parse_exact_location( &parsed[5] );

    if( parsed.size() >= 11 )
    {
      param3_ = boost::lexical_cast<unsigned>( parsed[10] );
    }
    if( parsed.size() >= 12 )
    {
      param1_ = boost::lexical_cast<double>( parsed[11] );
    }
    if( parsed.size() >= 13 )
    {
      param2_ = boost::lexical_cast<double>( parsed[12] );
    }
    if( parsed.size() >= 14 )
    {
      param4_ = boost::lexical_cast<double>( parsed[13] );
    }
  }
  else if( parsed[1] == "CLASSIFY_REGION"  )
  {
    if( parsed.size() != 9 && !( parsed.size() >= 11 && parsed.size() < 15 ) )
    {
      return false;
    }

    action_ = CLASSIFY_REGION;
    key_ = parsed[3];
    rect_ = parse_exact_location( &parsed[5] );

    if( parsed.size() >= 11 )
    {
      param3_ = boost::lexical_cast<unsigned>( parsed[10] );
    }
    if( parsed.size() >= 12 )
    {
      param1_ = boost::lexical_cast<double>( parsed[11] );
    }
    if( parsed.size() >= 13 )
    {
      param2_ = boost::lexical_cast<double>( parsed[12] );
    }
    if( parsed.size() >= 14 )
    {
      param4_ = boost::lexical_cast<double>( parsed[13] );
    }
  }
  else if( parsed[1] == "XHAIR_SELECT"  )
  {
    if( parsed.size() != 11 )
    {
      return false;
    }

    action_ = XHAIR_SELECT;
    key_ = parsed[3];
    key2_ = parsed[5];
    rect_ = parse_exact_location( &parsed[7] );
  }
  else if( parsed[1] == "UNION_MASK"  )
  {
    if( parsed.size() != 4 )
    {
      return false;
    }

    action_ = UNION_MASK;
    key_ = parsed[3];
  }
  else if( parsed[1] == "INTERSECT_MASK"  )
  {
    if( parsed.size() != 4 )
    {
      return false;
    }

    action_ = INTERSECT_MASK;
    key_ = parsed[3];
  }
  else if( parsed[1] == "USE_MASK"  )
  {
    if( parsed.size() != 4 )
    {
      return false;
    }

    action_ = USE_MASK;
    key_ = parsed[3];
  }
  else if( parsed[1] == "ERODE"  )
  {
    if( parsed.size() != 8 )
    {
      return false;
    }

    action_ = ERODE;
    param1_ = boost::lexical_cast<double>(parsed[2]);
    rect_ = parse_exact_location( &parsed[4] );
  }
  else if( parsed[1] == "DILATE"  )
  {
    if( parsed.size() != 8 )
    {
      return false;
    }

    action_ = DILATE;
    param1_ = boost::lexical_cast<double>(parsed[2]);
    rect_ = parse_exact_location( &parsed[4] );
  }
  else if( parsed[1] == "USE_TEMPLATE_MATCHING" )
  {
    if( parsed.size() < 4 || parsed.size() > 6 )
    {
      return false;
    }

    action_ = USE_TEMPLATE_MATCHING;

    if( parsed.size() == 6 )
    {
      key_ = parsed[3];
      key2_ = parsed[5];
    }
    else if( parsed.size() == 5 )
    {
      key_ = parsed[3];
      key2_ = parsed[4];
    }
    else if( parsed.size() == 4 )
    {
      key_ = parsed[2];
      key2_ = parsed[3];
    }
  }
  else if( parsed[1] == "PARSE_TEXT" )
  {
    if( parsed.size() != 9 && parsed.size() != 10 )
    {
      return false;
    }

    action_ = PARSE_TEXT;
    key_ = parsed[2];
    key2_ = parsed[3];
    rect_ = parse_exact_location( &parsed[5] );

    if( parsed.size() == 10 )
    {
      if( parsed[9] == "TRUE" )
      {
        param1_ = 1.0;
      }
      else if( parsed[9] == "FALSE" )
      {
        param1_ = 0.0;
      }
      else
      {
        param1_ = boost::lexical_cast<double>( parsed[9] );
      }
    }
  }
  else
  {
    return false;
  }

  return true;
}


// ----------------------- TEMPLATE DEFINITION -------------------------


void
osd_template
::set_any_color_flag( bool val )
{
  any_color_ = val;
}


void
osd_template
::set_any_resolution_flag( bool val )
{
  any_resolution_ = val;
}


void
osd_template
::set_wide_only_flag( bool val )
{
  wide_only_ = val;
}


bool
osd_template
::add_resolution( const unsigned& ni, const unsigned& nj )
{
  ni_.push_back( ni );
  nj_.push_back( nj );
  return true;
}


bool
osd_template
::add_feature( const osd_symbol& symbol )
{
  sym_list_.push_back( symbol );
  return true;
}


bool
osd_template
::add_classifier( const std::string& identifier )
{
  classifiers_.push_back( identifier );
  return true;
}


bool
osd_template
::add_relationship( const osd_symbol_relationship& rel )
{
  bool sym1_present = false;
  bool sym2_present = false;

  if( rel.sym1_ == rel.sym2_ )
  {
    return false;
  }

  for( unsigned i = 0; i < sym_list_.size(); i++ )
  {
    if( sym_list_[i].id_ == rel.sym1_ )
    {
      sym1_present = true;
    }
    if( sym_list_[i].id_ == rel.sym2_ )
    {
      sym2_present = true;
    }
  }

  if( !sym1_present || !sym2_present )
  {
    return false;
  }

  rel_list_.push_back( rel );
  return true;
}


bool
osd_template
::add_color( const vil_rgb<double>& color,
             const vil_rgb<double>& tolerance )
{
  color_list_.push_back( color );
  tolerance_list_.push_back( tolerance );
  return true;
}


bool
osd_template
::add_action( const osd_action_item& action )
{
  action_list_.push_back( action );
  return true;
}


bool
osd_template
::check_resolution( const unsigned ni, const unsigned nj ) const
{
  if( wide_only_ && ( static_cast<double>( ni ) / nj ) <= 1.5 )
  {
    return false;
  }

  if( any_resolution_ )
  {
    return true;
  }

  for( unsigned i = 0; i < ni_.size(); ++i )
  {
    if( ni == ni_[i] && nj == nj_[i] )
    {
      return true;
    }
  }

  return false;
}


// Does a given template possibly contain another? Note: not bidirectional
double
osd_template
::possibly_contains( const osd_template& other )
{
  // All of our tests
  bool resolution_match = false;
  bool rough_color_match = false;

  // Resolution comparison tests
  if( this->any_resolution_ )
  {
    if( !this->wide_only_ || other.any_resolution_ )
    {
      resolution_match = true;
    }
    else
    {
      for( unsigned r = 0; r < other.ni_.size(); r++ )
      {
        if( this->check_resolution( other.ni_[r], other.nj_[r] ) )
        {
          resolution_match = true;
          break;
        }
      }
    }
  }
  else if( other.any_resolution_ )
  {
    for( unsigned r = 0; r < this->ni_.size(); r++ )
    {
      if( other.check_resolution( this->ni_[r], this->nj_[r] ) )
      {
        resolution_match = true;
        break;
      }
    }
  }
  else
  {
    for( unsigned r = 0; r < other.ni_.size(); r++ )
    {
      if( this->check_resolution( other.ni_[r], other.nj_[r] ) )
      {
        resolution_match = true;
        break;
      }
    }
  }

  if( !resolution_match )
  {
    return 0.0;
  }

  // Basic color match
  if( this->any_color_ || other.any_color_ )
  {
    rough_color_match = true;
  }

  for( unsigned i = 0; i < this->color_list_.size(); i++ )
  {
    const double r = this->color_list_[i].r;
    const double g = this->color_list_[i].g;
    const double b = this->color_list_[i].b;

    for( unsigned j = 0; j < other.color_list_.size(); j++ )
    {
      if( std::fabs(other.color_list_[j].r-r) < this->tolerance_list_[i].r &&
          std::fabs(other.color_list_[j].g-g) < this->tolerance_list_[i].g &&
          std::fabs(other.color_list_[j].b-b) < this->tolerance_list_[i].b )
      {
        rough_color_match = true;
        break;
      }
    }
  }

  if( !rough_color_match )
  {
    return 0.0;
  }

  // Calculate possible symbol correspondances
  const unsigned sym_count = sym_list_.size();
  std::vector< std::vector< unsigned > > possible_matches( sym_count );
  for( unsigned i = 0; i < sym_count; i++ )
  {
    for( unsigned j = 0; j < other.sym_list_.size(); j++ )
    {
      if( can_symbols_match( sym_list_[i], other.sym_list_[j] ) )
      {
        possible_matches[i].push_back( j );
      }
    }
  }

  // Calculate net weight
  double weight = 0.0;
  for( unsigned i = 0; i < possible_matches.size(); i++ )
  {
    if( possible_matches[i].size() > 0 )
    {
      weight += sym_list_[i].weight_;
    }
  }

  if( this->is_default_ && weight < 1.0 )
  {
    weight = 1.0;
  }

  if( this->force_use_ )
  {
    weight = std::numeric_limits<double>::max();
  }

  // Formulate output score
  return weight;
}


// Return the identifier of this template.
std::string const&
osd_template
::get_name() const
{
  return name_;
}


// Return a list of action items associated with this template.
std::vector< osd_action_item > const&
osd_template
::get_actions() const
{
  return action_list_;
}


// Return a list of symbols associated with this template.
std::vector< osd_symbol > const&
osd_template
::get_symbols() const
{
  return sym_list_;
}


// Return the identifiers of all classifiers in this template.
std::vector< std::string > const&
osd_template
::get_classifiers() const
{
  return classifiers_;
}


// Does this template have any classifiers?
bool
osd_template
::has_classifier() const
{
  return !classifiers_.empty();
}


// Is the force flag set for this template?
bool
osd_template
::force_flag_set() const
{
  return force_use_;
}


// Is this the default template to use in the event that none are detected?
void
osd_template
::set_default_flag()
{
  is_default_ = true;
}


// Set the force use flag.
void
osd_template
::set_force_flag()
{
  force_use_ = true;
}


// Convert a blob to a simple symbol, given image resolution
bool
convert_blob_to_symbol( const unsigned int& ni,
                        const unsigned int& nj,
                        const vil_blob_pixel_list& blob,
                        osd_symbol& sym )
{
  vgl_box_2d<unsigned int> border;

  for( unsigned i = 0; i < blob.size(); i++ )
  {
    const unsigned int& pt_i = blob[i].first;
    const unsigned int& pt_j = blob[i].second;

    border.add( vgl_point_2d< unsigned >(pt_i,pt_j) );
  }

  double img_area = static_cast<double>( ni * nj );
  double img_width = static_cast<double>( ni );
  double img_height = static_cast<double>( nj );

  sym.lower_size_ = static_cast<double>( blob.size() ) / img_area;
  sym.upper_size_ = static_cast<double>( border.area() ) / img_area;

  sym.symbol_ = osd_symbol::UNKNOWN;

  sym.region_ = vgl_box_2d<double>( border.min_x() / img_width,
                                    border.max_x() / img_width,
                                    border.min_y() / img_height,
                                    border.max_y() / img_height );

  return true;
}


bool
read_templates_file( const std::string& filename,
                     std::vector< osd_template >& templates )
{
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

  std::ifstream input( filename.c_str() );

  if( !input )
  {
    LOG_ERROR( "Unable to open template file: " << filename );
    return false;
  }

  osd_template new_template;
  bool add_template = false;
  boost::char_separator<char> delim(" ()\n");
  unsigned line_num = 0;

  std::string line;
  while( std::getline(input, line) )
  {
    // Parse line
    tokenizer tokens( line, delim );
    std::vector< std::string > parsed_line;
    parsed_line.assign( tokens.begin(), tokens.end() );
    line_num++;

    // Ignore empty lines
    if( parsed_line.size() == 0 ) continue;

    // Ignore comment lines
    if( parsed_line[0].size() > 0 && parsed_line[0][0] == '#' ) continue;

    // Check for a new template
    if( parsed_line[0] == "template" )
    {
      if( add_template )
      {
        templates.push_back( new_template );
      }
      if( parsed_line.size() == 2 )
      {
        new_template = osd_template( parsed_line[1] );
      }
      else
      {
        LOG_ERROR( "Template string ill-formatted. Line " << line_num );
        return false;
      }
      add_template = true;
    }
    else if( parsed_line[0] == "DEFAULT" && add_template )
    {
      new_template.set_default_flag();
    }
    else if( parsed_line[0] == "FORCE" && add_template )
    {
      new_template.set_force_flag();
    }
    else if( parsed_line[0] == "RESOLUTION" && add_template )
    {
      if( parsed_line.size() == 2 && parsed_line[1] == "any" )
      {
        new_template.set_any_resolution_flag( true );
      }
      else if( parsed_line.size() == 2 && parsed_line[1] == "any_wide" )
      {
        new_template.set_any_resolution_flag( true );
        new_template.set_wide_only_flag( true );
      }
      else if( parsed_line.size() == 3 )
      {
        unsigned width = boost::lexical_cast<unsigned>( parsed_line[1] );
        unsigned height = boost::lexical_cast<unsigned>( parsed_line[2] );
        new_template.add_resolution( width, height );
      }
      else
      {
        LOG_ERROR( "Error parsing resolution string. Line " << line_num );
        return false;
      }
    }
    else if( parsed_line[0] == "SYMBOL" && add_template )
    {
      osd_symbol to_add;
      if( !to_add.parse_symbol_string( line ) )
      {
        LOG_ERROR( "Error parsing symbol string. Line " << line_num );
        return false;
      }
      new_template.add_feature( to_add );
    }
    else if( parsed_line[0] == "RELATIONSHIP" && add_template )
    {
      osd_symbol_relationship to_add;
      if( !to_add.parse_relationship_string( line ) )
      {
        LOG_ERROR( "Error parsing relationship string. Line " << line_num );
        return false;
      }
      new_template.add_relationship( to_add );
    }
    else if( parsed_line[0] == "COLOR" && add_template )
    {
      if( parsed_line.size() == 2 && parsed_line[1] == "any" )
      {
        new_template.set_any_color_flag( true );
      }
      else if( parsed_line.size() == 8 )
      {
        vil_rgb<double> color, tolerance;

        color.r = boost::lexical_cast<double>( parsed_line[1] );
        color.g = boost::lexical_cast<double>( parsed_line[2] );
        color.b = boost::lexical_cast<double>( parsed_line[3] );

        tolerance.r = boost::lexical_cast<double>( parsed_line[5] );
        tolerance.g = boost::lexical_cast<double>( parsed_line[6] );
        tolerance.b = boost::lexical_cast<double>( parsed_line[7] );

        new_template.add_color( color, tolerance );
      }
      else
      {
        LOG_ERROR( "Error parsing color string. Line " << line_num );
        return false;
      }
    }
    else if( ( parsed_line[0].substr( 0, 6 ) == "ACTION" ||
               parsed_line[0] == "BLINKER_TRIGGER" ) && add_template )
    {
      osd_action_item to_add;
      if( !to_add.parse_action_string( line ) )
      {
        LOG_ERROR( "Error parsing action string. Line " << line_num );
        return false;
      }
      new_template.add_action( to_add );
    }
    else if( parsed_line[0] == "CLASSIFIER" || parsed_line[0] == "CLASSIFIERS" )
    {
      for( unsigned i = 1; i < parsed_line.size(); ++i )
      {
        if( parsed_line[i] == "FILE" )
        {
          continue;
        }
        new_template.add_classifier( parsed_line[i] );
      }
    }
    else
    {
      LOG_ERROR( "Undefined string: " << parsed_line[0] << " on line " << line_num );
      return false;
    }
  }

  // Add last template in file to list
  if( add_template )
  {
    templates.push_back( new_template );
  }

  input.close();
  return true;
}


} // namespace vidtk
