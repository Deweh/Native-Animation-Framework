#pragma once
#ifndef M_PI
#	define M_PI 3.1415926535897
#endif

#ifndef SHORT_PI
#	define SHORT_PI 3.1415926545f
#endif

#include <optional>
#include <string.h>
#include <string>
#include <algorithm>
#include <cctype>
using namespace std::literals;

#include <tuple>
#include <string_view>
#include <sstream>
#include <set>
#include <map>
#include <mutex>
#include <ppl.h>
#include <unordered_set>
#include <unordered_map>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

#include <pugixml/src/pugixml.hpp>
#define PUGIXML_HEADER_ONLY

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <RE/Bethesda/BSLock.h>

namespace logger = F4SE::log;

#include <Data/Constants.h>
#include <Data/XMLUtil.h> 
//#include <Misc/Strings.h>
#include <Misc/Utility.h>
//#include <Misc/MathUtil.h>

#include <utils/utility.h>
#include <NAFicator/Objects/objects_map.h>
#include <NAFicator/Form/Form.h>
//#include <NAFicator/XML/XMLFile.h>

#include "ParsedObject.h"

//#include "Tag.h"
//#include "Action.h"
//#include "Animation.h"
//#include "AnimationGroup.h"
//#include "EquipmentSet.h"
//#include "Furniture.h"
//#include "MfgSet.h"
//#include "MorphSet.h"
//#include "objects_map.h"
//#include "Overlay.h"
//#include "Position.h"
//#include "PositionTree.h"
//#include "ProtectedEquipment.h"
//#include "Race.h"


