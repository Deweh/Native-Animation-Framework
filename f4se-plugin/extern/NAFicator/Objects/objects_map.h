#pragma once
#include <string_view>
#include <unordered_map>
#include <set>

using namespace std::string_view_literals;
typedef std::unordered_map<std::string_view, std::vector<std::string_view>> Attributes;
typedef std::unordered_map<std::string_view, Attributes> Nodes;

namespace NAFicator
{
	// Инициализация структуры данных
	static const Nodes nodes_map = {
		{ "animationData"sv,
			{ { "defaults"sv, { "loadPriority"sv, "source"sv, "idleSource"sv } },
				{ "animation"sv, { "tags"sv, "offset"sv } },
				{ "idle"sv, { "form"sv, "source"sv } },
				{ "actor"sv, { "idleSource"sv, "idleForm"sv, "file"sv, "skeleton"sv, "gender"sv, "faceAnim"sv, "mfgSet"sv, "loopFaceAnim"sv, "startEquipmentSet"sv, "stopEquipmentSet"sv, "scale"sv } },
				{ "action"sv, { "id"sv } },
				{ "morph"sv, { "id"sv, "to"sv } } } },

		{ "raceData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "race"sv, { "skeleton"sv, "loadPriority"sv, "form"sv, "source"sv, "requiresReset"sv, "requiresAnimReset"sv, "requiresForceLoop"sv, "startEvent"sv, "stopEvent"sv, "graph"sv } } } },

		{ "positionData"sv,
			{ { "defaults"sv, { "loadPriority"sv, "offset"sv } },
				{ "animationOffset"sv, { "offset"sv, ""sv, ""sv } },
				{ "position"sv, { "id"sv, "tags"sv, "isHidden"sv, "startMorphSet"sv, "stopMorphSet"sv, "startEquipmentSet"sv, "stopEquipmentSet"sv, "location"sv, "animation"sv, "animationGroup"sv, "positionTree"sv } } } },

		{ "morphSetData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "morphSet"sv, { "id"sv } },
				{ "condition"sv, { "isFemale"sv, "isPlayer"sv, "name"sv, "skeleton"sv, "hasKeyword"sv } },
				{ "morph"sv, { "value"sv, "to"sv } } } },

		{ "equipmentSetData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "equipmentSet"sv, { "id"sv } },
				{ "condition"sv, { "isFemale"sv, "isPlayer"sv, "name"sv, "skeleton"sv, "hasKeyword"sv } },
				{ "unEquip"sv, { "bipedSlot"sv } },
				{ "reEquip"sv, { "addEquipment"sv } },
				{ "addEquipment"sv, { "form"sv, "source"sv } },
				{ "removeEquipment"sv, { "form"sv, "source"sv } } } },

		{ "actionData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "action"sv, { "id"sv } },
				{ "self"sv, { "startEquipmentSet"sv } } } },

		{ "animationGroupData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "animationGroup"sv, { "id"sv, "sequential"sv } },
				{ "stage"sv, { "animation"sv, "loops"sv, "weight"sv } } } },

		{ "furnitureData"sv,
			{ { "defaults"sv, { "loadPriority"sv, "source"sv } },
				{ "group"sv, { "id"sv } },
				{ "furniture"sv, { "source"sv, "form"sv, "keyword"sv, "startAnimation"sv, "stopAnimation"sv } } } },

		{ "positionTreeData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "tree"sv, { "id"sv } },
				{ "branch"sv, { "id"sv, "positionID"sv, "time"sv, "forceComplete"sv } } } },

		{ "mfgSetData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "mfgSet"sv, { "id"sv } },
				{ "setting"sv, { "morphID"sv, "intensity"sv } } } },

		{ "overlayData"sv,
			{ { "defaults"sv, { "loadPriority"sv } },
				{ "overlaySet", { "id"sv } },
				{ "condition"sv, { "isFemale"sv, "isPlayer"sv, "name"sv, "skeleton"sv, "hasKeyword"sv } },
				{ "overlayGroup"sv, { "duration"sv, "quantity"sv } },
				{ "overlay"sv, { "template"sv, "alpha"sv, "isFemale"sv } } } },

		{ "protectedEquipmentData"sv,
			{ { "defaults"sv, { "loadPriority"sv, "source"sv } },
				{ "condition"sv, { "isFemale"sv, "isPlayer"sv, "name"sv, "skeleton"sv, "hasKeyword"sv } },
				{ "protectKeyword"sv, { "form"sv, "source"sv } } } }
	};
}
