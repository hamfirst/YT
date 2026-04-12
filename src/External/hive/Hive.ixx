
module;

#include <plf_hive.h>

module YT:Hive;

namespace plf
{
  using ::plf::hive;
}

template <typename T>
using Hive = plf::hive<T>;
