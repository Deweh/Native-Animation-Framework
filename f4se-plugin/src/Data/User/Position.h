#pragma once
#include "Data/ControlSystemTypes.h"
#include "Data/XMLUtil.h"
#include "Bridge/NewData/OffsetClass.h"

namespace Data
{
	//NAF Bridge offset
	//class Position : public IdentifiableObject, public TagHolder
	class Position : public IdentifiableObject, public TagHolder
	{
	public:
		enum Type : uint8_t
		{
			kAnimation = 0,
			kAnimationGroup = 1,
			kPositionTree = 2,
		};

		struct ControlSystemInfo
		{
			ControlSystemType type = kAnimationSystem;
			std::string id;
			std::shared_ptr<const Animation> anim;
			std::string startMorphSet;
			std::string stopMorphSet;
		};

		struct AnimationGroupInfo : public ControlSystemInfo
		{
			AnimationGroupInfo(std::shared_ptr<const AnimationGroup> g)
			{
				type = kAnimationGroupSystem;
				group = g;
			}

			std::shared_ptr<const AnimationGroup> group;
		};

		struct PositionTreeInfo : public ControlSystemInfo
		{
			PositionTreeInfo(std::shared_ptr<const PositionTree> t)
			{
				type = kPositionTreeSystem;
				tree = t;
			}

			std::shared_ptr<const PositionTree> tree;
		};

		std::unique_ptr<ControlSystemInfo> GetControlSystemInfo() const
		{
			std::unique_ptr<ControlSystemInfo> info;
			switch (posType) {
			case kAnimation:
				info = std::make_unique<ControlSystemInfo>();
				break;
			case kAnimationGroup:
				info = std::make_unique<AnimationGroupInfo>(GetAnimationGroup(idForType));
				break;
			case kPositionTree:
				info = std::make_unique<PositionTreeInfo>(GetPositionTree(idForType));
				break;
			}
			info->id = id;
			info->anim = GetBaseAnimation();
			info->startMorphSet = startMorphSet;
			info->stopMorphSet = stopMorphSet;
			return info;
		}

		std::shared_ptr<const Animation> GetBaseAnimation() const
		{
			switch (posType) {
			case kAnimation:
				{
					return GetAnimation(idForType);
				}
			case kAnimationGroup:
				{
					if (auto group = GetAnimationGroup(idForType); group != nullptr) {
						return group->GetBaseAnimation();
					} else {
						return nullptr;
					}
				}
			case kPositionTree:
				{
					if (auto tree = GetPositionTree(idForType); tree != nullptr) {
						if (auto rootPos = GetPosition(tree->tree->position); rootPos != nullptr && rootPos->posType != kPositionTree) {
							return rootPos->GetBaseAnimation();
						}
					}
				}
			default:
				return nullptr;
			}
		}

		static bool Parse(XMLUtil::Mapper& m, Position& out)
		{
			//NAF Bridge offset
			std::string offset_str("");

			if (m(&offset_str, XMLUtil::Mapper::emptyStr, true, false, "", "offset"))
			{
				if (!offset_str.empty()) {
					size_t f = 0;
					do {
						f = offset_str.find(':', f);
						if (f != std::string::npos) {
							offset_str.erase(f, 1);
							offset_str.insert(f, ";");
						} else break;
					} while (true);
					out.offset = Scene::offset_from_string(offset_str);
				}
			} else {
				auto r = m.GetRoot();
				offset_str.clear();
				r.GetArray([&](XMLUtil::Mapper& r) {
					std::string o;
					r(&o, XMLUtil::Mapper::emptyStr, true, false, "", "offset");
					if (!o.empty()) {
						if (!offset_str.empty())
						{
							offset_str += ';';
						}
						offset_str += o;
					}
					return r;
				},
					"animationOffset", "", false);
				if (!offset_str.empty())
					out.offset = Scene::offset_from_string(offset_str);
			}
			//NAF Bridge offset end

			out.ParseID(m);
			out.ParseTags(m);
			m(&out.hidden, false, true, false, "", "isHidden");
			m(&out.startMorphSet, XMLUtil::Mapper::emptyStr, true, false, "", "startMorphSet");
			m(&out.stopMorphSet, XMLUtil::Mapper::emptyStr, true, false, "", "stopMorphSet");
			m(&out.startEquipSet, XMLUtil::Mapper::emptyStr, true, false, "", "startEquipmentSet");
			m(&out.stopEquipSet, XMLUtil::Mapper::emptyStr, true, false, "", "stopEquipmentSet");

			std::string locs;
			m(&locs, XMLUtil::Mapper::emptyStr, true, false, "", "location");
			if (!locs.empty()) {
				Utility::ForEachSubstring(locs, ",", [&](const std::string_view& s) {
					out.locations.emplace_back(s);
				});
			}

			out.posType = kAnimation;
			std::string foundId = "";
			if (m(&foundId, XMLUtil::Mapper::emptyStr, true, false, "", "animation"); foundId.size() > 0) {
				out.idForType = foundId;
			} else if (m(&foundId, XMLUtil::Mapper::emptyStr, true, false, "", "animationGroup"); foundId.size() > 0) {
				out.posType = kAnimationGroup;
				out.idForType = foundId;
			} else if (m(&foundId, XMLUtil::Mapper::emptyStr, true, false, "", "positionTree"); foundId.size() > 0) {
				out.posType = kPositionTree;
				out.idForType = foundId;
			} else {
				out.idForType = out.id;
			}

			return m;
		}

		bool hidden = false;
		Type posType;
		std::string idForType;
		std::string startEquipSet;
		std::string stopEquipSet;
		std::string startMorphSet;
		std::string stopMorphSet;
		std::vector<std::string> locations;
		//std::string offset;  //NAF Bridge offset
		std::vector<Scene::offset_optional> offset; //NAF Bridge offset

		template <class Archive>
		void save(Archive& ar, const uint32_t) const
		{
			//NAF Bridge offset
			//ar(id, hidden, posType, idForType, startEquipSet, stopEquipSet, startMorphSet, stopMorphSet, locations);
			ar(id, hidden, posType, idForType, startEquipSet, stopEquipSet, startMorphSet, stopMorphSet, locations, offset);
		}

		template <class Archive>
		void load(Archive& ar, const uint32_t ver)
		{
			ar(id, hidden, posType, idForType, startEquipSet, stopEquipSet, startMorphSet, stopMorphSet);
			if (ver < 2) {
				std::optional<std::string> loc;
				ar(loc);
				if (loc.has_value()) {
					locations.push_back(loc.value());
				}
			} else if (ver == 2) { //NAF Bridge just if;
				ar(locations);
			} else {
				ar(locations);
				ar(offset); //NAF Bridge;
			}
		}
	};
}
//NAF Bridge offset
//CEREAL_CLASS_VERSION(Data::Position, 2);
CEREAL_CLASS_VERSION(Data::Position, 3);
