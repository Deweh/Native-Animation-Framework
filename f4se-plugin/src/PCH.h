#pragma once

#pragma warning(push)
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <spdlog/sinks/basic_file_sink.h>

#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;

using namespace std::literals;

#define M_PI 3.1415926535897

#include "Version.h"
#include "cereal/archives/binary.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/tuple.hpp"
#include "cereal/types/unordered_set.hpp"
#include "cereal/types/set.hpp"
#include "cereal/types/optional.hpp"
#include "cereal/types/atomic.hpp"
#include "cereal/types/utility.hpp"
#define PUGIXML_HEADER_ONLY
#include "../extern/pugixml/src/pugixml.hpp"
#include "zstr.hpp"
#include "nlohmann/json.hpp"
#include "ik/ik.h"
#include "BodyAnimation/Spline.h"
#include "libzippp/libzippp.h"
#include "fp16.h"
