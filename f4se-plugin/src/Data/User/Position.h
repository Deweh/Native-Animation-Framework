#pragma once
#include "Data/ControlSystemTypes.h"
#include "Data/XMLUtil.h"

namespace Data
{
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

		std::unique_ptr<ControlSystemInfo> GetControlSystemInfo() const {
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

		std::shared_ptr<const Animation> GetBaseAnimation() const {
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

		template <class Archive>
		void save(Archive& ar, const uint32_t) const
		{
			ar(id, hidden, posType, idForType, startEquipSet, stopEquipSet, startMorphSet, stopMorphSet, locations);
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
			} else {
				ar(locations);
			}
			
		}
	};
}

CEREAL_CLASS_VERSION(Data::Position, 2);
