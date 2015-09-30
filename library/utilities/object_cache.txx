#ifndef vidtk_object_cache_txx_
#define vidtk_object_cache_txx_

#include "object_cache.h"

namespace vidtk
{

template<typename T>
object_cache<T>*
object_cache<T>
::instance_ = NULL;

#define VIDTK_OBJECT_CACHE_INSTANTIATE(T) \
  template class vidtk::object_cache<T >;

} // end namespace vidtk
#endif // #ifndef vidtk_object_cache_txx_
