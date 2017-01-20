/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/property_map.h>

#include <sstream>

namespace vidtk
{

namespace
{
// We use a function to return the shared, global key map instead of a
// static variable because we can't be sure about the initialization
// order of static variables.  There is likely a call to key()
// somewhere to generate a key that is stored in a static key_type
// variable.  We can't be sure that the key map will be initialized
// before that variable.  By relying on a function, we ensure that the
// key map is initialized before it is used.
inline key_map_type& get_key_map()
{
  static key_map_type key_map_;
  return key_map_;
}

} // end anonymous namespace

property_map::key_type
property_map
::key( std::string const& name )
{
  static key_type last_key = 0;

  key_map_type::iterator it = get_key_map().find( name );
  if( it == get_key_map().end() )
  {
    ++last_key;
    get_key_map()[name] = last_key;
    return last_key;
  }
  else
  {
    return it->second;
  }
}

key_map_type property_map::key_map()
{
  return get_key_map();
}


std::vector< std::string >
property_map::
get_content_names() const
{
  std::vector< std::string > name_list;
  key_map_type const&  my_key_map( get_key_map() );

  map_type::const_iterator it = map_.begin();
  for ( /* empty */; it != map_.end(); ++it )
  {
    std::string name;
    // Reverse lookup integer to find string
    key_map_type::const_iterator kit = my_key_map.begin();
    for ( /* empty */; kit != my_key_map.end() ; ++kit )
    {
      if (kit->second == it->first)
      {
        name = kit->first;
        break;
      }
    }

    if ( ! name.empty() )
    {
      name_list.push_back( kit->first );
    }
    else
    {
      // unknown entry code
      std::stringstream str;
      str << "unknown (" << it->first << ")";
      name_list.push_back( str.str() );
    }
  }

  return name_list;
}


} // end namespace vidtk
