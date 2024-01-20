#pragma once
#include <ppl.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

namespace Data
{
	class Animation;
	class Position;
	class FaceAnim;
	class MorphSet;
	class EquipmentSet;
	class Action;
	class AnimationGroup;
	class PositionTree;
	class Race;
	class GraphInfo;

	std::shared_ptr<const Animation> GetAnimation(const std::string&);
	std::shared_ptr<const Position> GetPosition(const std::string&);
	std::shared_ptr<const FaceAnim> GetFaceAnim(const std::string&);
	std::shared_ptr<const MorphSet> GetMorphSet(const std::string&);
	std::shared_ptr<const EquipmentSet> GetEquipmentSet(const std::string&);
	std::shared_ptr<const Action> GetAction(const std::string&);
	std::shared_ptr<const AnimationGroup> GetAnimationGroup(const std::string&);
	std::shared_ptr<const PositionTree> GetPositionTree(const std::string&);
	std::shared_ptr<const Race> GetRace(const std::string&);
	std::shared_ptr<const Race> GetRaceFromGraph(const std::string&);
	std::shared_ptr<const Race> GetRace(RE::Actor*);
	std::shared_ptr<const GraphInfo> GetGraphInfo(const std::string&, RE::Actor* actorBase = nullptr);
}

#include <Windows.h>
#include <fileapi.h>
#include <fstream>
#include "Misc/Utility.h"
#include "Settings.h"
#include "Serialization/General.h"
#include "XMLUtil.h"
#include "Cache/XMLCache.h"
#include "Cache/AnimCache.h"
#include "Data/Forms.h"
#include "Data/User/IdentifiableObject.h"
#include "Data/User/GraphInfo.h"
#include "Data/User/Tag.h"
#include "Data/User/Condition.h"
#include "Data/User/Action.h"
#include "Data/User/Furniture.h"
#include "Data/User/MorphSet.h"
#include "Scene/OrderedActionQueue.h"
#include "Data/User/EquipmentSet.h"
#include "Scene/Types.h"
#include "Data/User/Animation.h"
#include "Data/User/AnimationGroup.h"
#include "Data/User/PositionTree.h"
#include "Data/User/Position.h"
#include "Data/User/Race.h"
#include "Data/User/FaceAnim.h"
#include <shared_mutex>
#include "BodyAnimation/NodeAnimationData.h"
#include "BodyAnimation/NANIM.h"

namespace Data
{
	using namespace Serialization::General;

	class Global
	{
	public:
		template <typename T>
		class IDMap : public concurrency::concurrent_unordered_map<std::string, std::pair<RE::BSSpinLock, std::shared_ptr<T>>>
		{
		public:
			using super = concurrency::concurrent_unordered_map<std::string, std::pair<RE::BSSpinLock, std::shared_ptr<T>>>;

			void priority_insert(std::shared_ptr<T> ele) {
				auto& target = super::operator[](ele->id);
				RE::BSAutoLock l{ target.first };
				if (target.second == nullptr || target.second->loadPriority < ele->loadPriority) {
					target.second = ele;
				}
			}

			std::shared_ptr<T> get_ptr(const std::string& id) {
				auto iter = super::find(id);
				if (iter != super::end()) {
					return iter->second.second;
				} else {
					return nullptr;
				}
			}
		};

		inline static ThreadSafeAccessor<std::unordered_map<std::string, std::string>> RaceLinkMap;

		inline static IDMap<Race> Races;
		inline static IDMap<Animation> Animations;
		inline static IDMap<Position> Positions;
		inline static IDMap<FaceAnim> FaceAnims;
		inline static IDMap<MorphSet> MorphSets;
		inline static IDMap<EquipmentSet> EquipmentSets;
		inline static IDMap<Action> Actions;
		inline static IDMap<AnimationGroup> AnimationGroups;
		inline static IDMap<Furniture> Furnitures;
		inline static IDMap<PositionTree> PositionTrees;
		inline static IDMap<GraphInfo> GraphInfos;

		struct ApplicableFurniture
		{
			std::unordered_set<RE::TESBoundObject*> forms;
			std::unordered_set<const RE::BGSKeyword*> keywords;
		};

		static ApplicableFurniture GetApplicableFurniture(AnimationFilter filter, bool excludeHiddenPositions = false) {
			std::unique_lock _l{ reloadLock };
			ApplicableFurniture result;

			const auto locations = GetFilteredPositions(filter, excludeHiddenPositions, false, nullptr, true);
			const std::unordered_set<std::string> locSet(locations.begin(), locations.end());

			for (auto& loc : locSet) {
				auto furn = Furnitures.get_ptr(loc);
				if (furn != nullptr) {
					for (auto& form : furn->forms) {
						result.forms.insert(form.get());
					}
					for (auto& kw : furn->keywords) {
						if (auto kwForm = RE::TESForm::GetFormByEditorID<RE::BGSKeyword>(kw); kwForm != nullptr)
							result.keywords.insert(kwForm);
					}
				}
			}
			return result;
		}

		class TagFilter
		{
		public:
			TagFilter(const std::vector<std::string_view>& include, const std::vector<std::string_view>& exclude,
				const std::vector<std::string_view>& require) {
				includeTags = { include.begin(), include.end() };
				excludeTags = { exclude.begin(), exclude.end() };
				requireTags = { require.begin(), require.end() };
			}

			bool operator()(const std::set<std::string>& compare) const {
				for (const auto& s : excludeTags) {
					if (compare.contains(s))
						return false;
				}

				for (const auto& s : requireTags) {
					if (!compare.contains(s))
						return false;
				}

				if (includeTags.size() > 0) {
					for (const auto& s : includeTags) {
						if (compare.contains(s)) {
							return true;
						}
					}
					return false;
				}
				
				return true;
			}

			std::vector<std::string> includeTags;
			std::vector<std::string> excludeTags;
			std::vector<std::string> requireTags;
		};

		static std::vector<std::string> GetFilteredPositions(AnimationFilter filter, bool excludeHidden = false,
			bool sort = false, RE::TESObjectREFR* furnRefr = nullptr, bool outputLocations = false,
			const std::optional<TagFilter>& filterTags = std::nullopt)
		{
			std::unique_lock _l{ reloadLock };

			std::vector<std::string> filteredPositions;
			std::unordered_set<std::string> locations;

			if (furnRefr != nullptr) {
				RE::TESBoundObject* targetObj = furnRefr->data.objectReference;

				for (auto& f : Furnitures) {
					bool found = false;
					for (auto& obj : f.second.second->forms) {
						if (obj.get() == targetObj) {
							locations.insert(f.first);
							found = true;
							break;
						}
					}
					if (!found) {
						for (auto& kw : f.second.second->keywords) {
							auto kwForm = RE::TESForm::GetFormByEditorID<RE::BGSKeyword>(kw);
							if (kwForm != nullptr && furnRefr->HasKeyword(kwForm)) {
								locations.insert(f.first);
								break;
							}
						}
					}
				}
			}

			bool doLocFilter = locations.size() > 0;

			AnimationFilter::InfoMap filterResult;
			std::vector<size_t> anyIndices;
			for (auto& pBase : Positions) {
				auto& p = pBase.second;
				if (excludeHidden && p.second->hidden) {
					continue;
				}
				if ((doLocFilter && (p.second->locations.empty() || !Utility::SetContainsAnyInVector(p.second->locations, locations)))
					|| (!doLocFilter && !outputLocations && !p.second->locations.empty()) ) {
					continue;
				}
				auto a = p.second->GetBaseAnimation();
				if (a->slots.size() == filter.numTotalActors) {
					filter.CopyToMap(filterResult);
					anyIndices.clear();
					bool match = true;

					for (size_t i = 0; i < a->slots.size(); i++) {
						const auto& s = a->slots[i];
						if (s.gender != Any) {
							const auto it = filterResult.find(s.rootBehavior);
							if (it == filterResult.end() || !it->second.SubtractGender(s.gender)) {
								match = false;
								break;
							}
						} else {
							anyIndices.push_back(i);
						}
					}

					if (match) {
						for (const auto& i : anyIndices) {
							const auto& s = a->slots[i];
							const auto it = filterResult.find(s.rootBehavior);
							if (it == filterResult.end() || !it->second.SubtractGender(s.gender)) {
								match = false;
								break;
							}
						}
					}

					if (match) {
						if (filterTags.has_value() && !filterTags.value()(p.second->tags)) {
							continue;
						}
						if (!outputLocations) {
							filteredPositions.push_back(p.second->id);
						} else if (!p.second->locations.empty()) {
							for (auto& i : p.second->locations) {
								filteredPositions.push_back(i);
							}
						}
					}
				}
			}

			if (sort) {
				std::sort(filteredPositions.begin(), filteredPositions.end());
			}

			return filteredPositions;
		}

		static void Init(bool verbose = true)
		{
			Settings::Load();

			try {
				std::filesystem::create_directories(USERDATA_DIR);
			} catch (std::exception ex) {
				logger::warn("Failed to create '{}' directory. Full message: {}", USERDATA_DIR, ex.what());
			}

			std::vector<std::pair<const std::string, const std::filesystem::file_time_type>> xmlFiles;
			std::vector<std::string> nanimFiles;

			try {
				for (auto& f : std::filesystem::recursive_directory_iterator(USERDATA_DIR)) {
					auto p = f.path();
					if (f.exists() && !f.is_directory() && p.has_filename() && p.has_extension()) {
						auto ex = p.extension().generic_string();
						if (ex == ".xml") {
							xmlFiles.push_back({ p.generic_string(), std::filesystem::last_write_time(p) });
						} else if (ex == ".nanim") {
							nanimFiles.push_back(p.generic_string());
						}
					}
				}
			} catch (std::exception ex) {
				logger::warn("Failed to get contents of '{}' directory. Full message: {}", USERDATA_DIR, ex.what());
			}

			Utility::StartPerformanceCounter();
			
			if (!XMLCache::IsCacheValid(xmlFiles) || !XMLCache::LoadCache() || !AnimCache::Load()) {
				AnimCache::Delete();
				XMLCache::Delete();
				if (verbose)
					logger::info("Cache invalid, rebuilding (startup may take longer than usual)...");
				for (auto& p : xmlFiles) {
					XMLCache::AddFileToCache(p.first);
				}
			} else {
				FaceAnim::nextFileId = XMLCache::primaryCache.nextFaceAnimId;
			}

			concurrency::parallel_for_each(XMLCache::primaryCache.files.begin(), XMLCache::primaryCache.files.end(), [&](auto& iter) {
				if (ParseXML(iter.data, iter.filename, verbose) && verbose) {
					logger::info("Loaded {}", iter.filename);
				}
			});

			XMLCache::primaryCache.nextFaceAnimId = FaceAnim::nextFileId;
			XMLCache::Flush();

			concurrency::parallel_for_each(nanimFiles.begin(), nanimFiles.end(), [&](const std::string& f) {
				if (ParseNANIM(f, verbose) && verbose) {
					logger::info("Loaded {}", f);
				}
			});

			if (verbose) {
				auto performanceSeconds = Utility::GetPerformanceCounter();
				logger::info(FMT_STRING("Finished loading files in {:.3f}s"), performanceSeconds);
			}
		}

		static void InitGameData()
		{
			LinkDataReferences();
			Forms::GetFormPointers();
		}

		inline static safe_mutex reloadLock;

		static void HotReload(bool rebuildFiles = true)
		{
			std::unique_lock l{ reloadLock };

			auto timer = Utility::CreatePerfCounter();
			logger::info("Rebuilding cache...");
			if (rebuildFiles) {
				XMLCache::Delete();
				AnimCache::Delete();
			} else {
				AnimCache::Clear();
			}
			RaceLinkMap.GetWriteAccess()->clear();
			Races.clear();
			Animations.clear();
			Positions.clear();
			FaceAnims.clear();
			MorphSets.clear();
			EquipmentSets.clear();
			Actions.clear();
			AnimationGroups.clear();
			Furnitures.clear();
			PositionTrees.clear();
			GraphInfos.clear();
			FaceAnim::nextFileId = 0;
			Init(false);
			LinkDataReferences(false);
			logger::info("Finished rebuilding in {:.0f}ms", Utility::QueryPerfCounterTime(timer));
		}

		template <typename T, typename J>
		static void ParseXMLType(IDMap<J>& map, XMLUtil::Mapper& mapper)
		{
			auto obj = std::make_shared<J>();
			if (T::Parse(mapper, *obj)) {
				map.priority_insert(obj);
			}
		}

		template <typename T>
		static void ParseXMLType(IDMap<T>& map)
		{
			ParseXMLType<T, T>(map);
		}

		static inline constexpr std::array<std::string_view, 10> topNodeNames{
			"animationData",
			"raceData",
			"positionData",
			"morphSetData",
			"equipmentSetData",
			"actionData",
			"animationGroupData",
			"furnitureData",
			"positionTreeData",
			"tagData"
		};

		static inline const std::unordered_map<std::string_view, std::function<void(XMLUtil::Mapper&)>> nodeMappings{
			{ "race", [](auto& m) { ParseXMLType<Race>(Races, m); } },
			{ "animation", [](auto& m) { ParseXMLType<Animation>(Animations, m); } },
			{ "faceAnim", [](auto& m) { ParseXMLType<FaceAnim>(FaceAnims, m); } },
			{ "position", [](auto& m) { ParseXMLType<Position>(Positions, m); } },
			{ "morphSet", [](auto& m) { ParseXMLType<MorphSet>(MorphSets, m); } },
			{ "equipmentSet", [](auto& m) { ParseXMLType<EquipmentSet>(EquipmentSets, m); } },
			{ "action", [](auto& m) { ParseXMLType<Action>(Actions, m); } },
			{ "animationGroup", [](auto& m) { ParseXMLType<AnimationGroup>(AnimationGroups, m); } },
			{ "group", [](auto& m) { ParseXMLType<Furniture>(Furnitures, m); } },
			{ "tree", [](auto& m) { ParseXMLType<PositionTree>(PositionTrees, m); } },
			{ "tag", [](auto& m) { TagData::Parse(m); } },
			{ "graph", [](auto& m) { ParseXMLType<GraphInfo>(GraphInfos, m); } }
		};

		static bool ParseNANIM(const std::string& fName, bool = true) {
			BodyAnimation::NANIM container;
			if (!container.LoadFromFile(fName, true))
				return false;
			auto& info = container.characters;
			if (info.data.empty())
				return false;

			auto anim = std::make_shared<Data::Animation>();
			anim->id = info.animId;
			anim->loadPriority = 1;
			for (const auto& c : info.data) {
				auto& slot = anim->slots.emplace_back();
				slot.dynamicIdle = true;
				slot.gender = c.gender;
				slot.idle[0] = std::filesystem::path(fName).lexically_relative(USERDATA_DIR).generic_string();
				slot.idle[1] = c.animId;
				slot.rootBehavior = c.behaviorGraphProject;
				if (c.scale.has_value()) {
					slot.customScale.set_has_value(true);
					slot.customScale.value() = c.scale.value();
				}
			}
			Animations.priority_insert(anim);

			auto pos = std::make_shared<Data::Position>();
			pos->id = info.animId;
			pos->idForType = info.animId;
			pos->loadPriority = 1;
			pos->posType = Data::Position::kAnimation;
			Positions.priority_insert(pos);
			return true;
		}

		static bool ParseXML(const std::string& f, std::string_view fName, bool verbose = true)
		{
			pugi::xml_document doc;
			const pugi::xml_parse_result result = doc.load_string(f.c_str());

			if (!result) {
				if (verbose)
					logger::warn("Failed to parse {}, message: {} at character {}", fName, result.description(), std::to_string(result.offset));
				return false;
			}

			pugi::xml_node topNode = doc;
			
			for (auto& n : topNodeNames) {
				if (auto data = doc.child(n.data()); !data.empty()) {
					topNode = data;
					break;
				}
			}

			auto defaults = topNode.child("defaults");
			auto childNodes = topNode.children();

			auto m = XMLUtil::Mapper(defaults, topNode, fName);
			m.verbose = verbose;
	
			for (auto& node : childNodes) {
				m.ResetSuccessFlag();
				m.SetCurrentNode(&node);

				if (auto mapping = nodeMappings.find(m.GetCurrentName()); mapping != nodeMappings.end()) {
					mapping->second(m);
				}
			}

			return true;
		}

		static void PatchHeadParts() {
			static bool patchApplied{ false };
			if (patchApplied)
				return;

			std::string strPartType = Settings::Values.sHeadPartPatchType.get();
			std::string triPath = Settings::Values.sHeadPartPatchTriPath.get();

			if (strPartType.size() < 1 || triPath.size() < 1)
				return;

			RE::BGSHeadPart::HeadPartType partType;
			if (strPartType == "Eyes") {
				partType = RE::BGSHeadPart::HeadPartType::kEyes;
			} else if (strPartType == "Misc") {
				partType = RE::BGSHeadPart::HeadPartType::kMisc;
			} else {
				return;
			}

			const auto& [map, lock] = RE::TESForm::GetAllForms();
			RE::BSAutoWriteLock l{ lock };

			std::atomic<uint64_t> count = 0;
			concurrency::parallel_for_each(map->begin(), map->end(), [&](RE::BSTTuple<const uint32_t, RE::TESForm*> ele) {
				RE::BGSHeadPart* part = ele.second->As<RE::BGSHeadPart>();
				
				if (part && part->type == partType) {
					part->morphs[1].model = triPath;
					count++;
				}
			});

			patchApplied = true;
			logger::info("Patched {} HeadParts.", count);
		}

		static void LinkDataReferences(bool verbose = true)
		{
			if (Settings::Values.bHeadPartMorphPatch)
				PatchHeadParts();

			std::unordered_map<std::string_view, RE::BSFixedString> skeletonProjectMap;
			skeletonProjectMap["Human"] = "RaiderProject";

			//Link Race skeletons to root behaviors.

			auto linkMap = RaceLinkMap.GetWriteAccess();
			for (auto iter = Races.begin(); iter != Races.end();) {
				const auto rForm = iter->second.second->baseForm.get(verbose);

				if (rForm != nullptr) {
					linkMap->insert({ rForm->behaviorGraphProjectName[0].c_str(), iter->first });
					skeletonProjectMap.insert({ iter->first, rForm->behaviorGraphProjectName[0] });
					iter++;
				} else {
					iter = Races.unsafe_erase(iter);
				}
			}

			//Link Condition skeletons to root behaviors.

			for (auto& m : MorphSets) {
				m.second.second->morphs.SkeletonToRootBehavior(skeletonProjectMap);
			}

			for (auto& e : EquipmentSets) {
				e.second.second->datas.SkeletonToRootBehavior(skeletonProjectMap);
			}

			//Link any LinkableForms

			for (auto& f : Furnitures) {
				for (auto& form : f.second.second->forms) {
					form.get(verbose);
				}
			}

			//Link Animation information.

			concurrency::parallel_for_each(Animations.begin(), Animations.end(), [&](const auto& pair) {
				auto& a = *pair.second.second;

				for (auto it = a.slots.begin(); it != a.slots.end(); it++) {
					auto& s = *it;

					if (s.idleRequiresConvert) {
						const auto iForm = IdentifiableObject::StringsToForm<RE::TESIdleForm>(s.idle[0], s.idle[1], verbose);
						if (iForm == nullptr) {
							s.idle[0] = "LooseIdleStop";
						} else {
							s.idle[0] = iForm->GetFormEditorID();
						}

						s.idleRequiresConvert = false;
					}

					if (s.behaviorRequiresConvert) {
						if (!skeletonProjectMap.contains(s.rootBehavior)) {
							if (verbose)
								logger::warn("Animation '{}' contains actor with invalid skeleton '{}', defaulting to Human.", a.id, s.rootBehavior);
							s.rootBehavior = skeletonProjectMap["Human"];
						} else {
							s.rootBehavior = skeletonProjectMap[s.rootBehavior];
						}
						s.behaviorRequiresConvert = false;
					}
				}
			});

			//Check that all Positions refer to a valid base Animation.

			std::vector<std::string> pendingDeletes;
			for (auto& pBase : Positions) {
				auto& p = pBase.second;
				auto baseAnim = p.second->GetBaseAnimation();
				if (baseAnim == nullptr) {
					if (verbose)
						logger::warn("Position '{}' does not point to a valid animation, discarding.", p.second->id);
					pendingDeletes.push_back(p.second->id);
				} else {
					p.second->Merge(*baseAnim);
				}
			}

			for (auto& s : pendingDeletes) {
				Positions.unsafe_erase(s);
			}

			//Link TagData to Positions.

			for (auto& d : TagData::Datas) {
				auto p = Positions.get_ptr(d.first);
				if (p == nullptr)
					continue;

				if (d.second.replace)
					p->tags.clear();

				p->Merge(d.second);
			}

			TagData::Datas.clear();
		}
	};

	template <typename T>
	std::shared_ptr<T> GetObjectFromIDMap(Global::IDMap<T>& m, const std::string& id)
	{
		std::unique_lock _l{ Global::reloadLock };
		return m.get_ptr(id);
	}

	std::shared_ptr<const Animation> GetAnimation(const std::string& id) { return GetObjectFromIDMap(Global::Animations, id); }
	std::shared_ptr<const Position> GetPosition(const std::string& id) { return GetObjectFromIDMap(Global::Positions, id); }
	std::shared_ptr<const FaceAnim> GetFaceAnim(const std::string& id) { return GetObjectFromIDMap(Global::FaceAnims, id); }
	std::shared_ptr<const MorphSet> GetMorphSet(const std::string& id) { return GetObjectFromIDMap(Global::MorphSets, id); }
	std::shared_ptr<const EquipmentSet> GetEquipmentSet(const std::string& id) { return GetObjectFromIDMap(Global::EquipmentSets, id); }
	std::shared_ptr<const Action> GetAction(const std::string& id) { return GetObjectFromIDMap(Global::Actions, id); }
	std::shared_ptr<const AnimationGroup> GetAnimationGroup(const std::string& id) { return GetObjectFromIDMap(Global::AnimationGroups, id); }
	std::shared_ptr<const PositionTree> GetPositionTree(const std::string& id) { return GetObjectFromIDMap(Global::PositionTrees, id); }
	std::shared_ptr<const Race> GetRace(const std::string& id) { return GetObjectFromIDMap(Global::Races, id); }

	std::shared_ptr<const GraphInfo> GetGraphInfo(const std::string& id, RE::Actor* actorBase) {
		auto result = GetObjectFromIDMap(Global::GraphInfos, id);
		if (result || !actorBase) {
			return result;
		}

		RE::NiPointer<RE::NiNode> skeleton1;
		RE::NiPointer<RE::NiNode> skeleton2;
		RE::BSModelDB::Demand(actorBase->race->skeletonModel[0].model.data(), skeleton1);
		RE::BSModelDB::Demand(actorBase->race->skeletonModel[1].model.data(), skeleton2);

		result = std::make_shared<GraphInfo>();
		result->id = id;
		std::unordered_set<std::string> nodeSet;

		const auto GetApplicableNodes = [&nodeSet](RE::NiNode* skeleton, GraphInfo* infoOut) {
			if (!skeleton)
				return;

			bool foundRoot = false;
			std::queue<RE::NiNode*> nodeQueue;
			nodeQueue.push(skeleton);

			while (!nodeQueue.empty()) {
				RE::NiNode* currentNode = nodeQueue.front();
				nodeQueue.pop();

				if (!foundRoot) {
					if (Utility::StringStartsWith(currentNode->name.c_str(), "Root")) {
						foundRoot = true;
						nodeQueue = {};
					}
				} else {
					std::string nodeName = currentNode->name.c_str();
					if (!nodeSet.contains(nodeName)) {
						infoOut->nodeList.push_back(nodeName);
						infoOut->skeletonPose.push_back(currentNode->local);
						nodeSet.insert(nodeName);
					}
				}

				for (uint16_t i = 0; i < currentNode->children.size(); i++) {
					auto c = currentNode->children[i];
					if (!c) {
						continue;
					}
					if (auto n = c->IsNode(); n) {
						nodeQueue.push(n);
					}
				}
			}
		};

		GetApplicableNodes(skeleton1.get(), result.get());
		GetApplicableNodes(skeleton2.get(), result.get());
		GetApplicableNodes(actorBase->Get3D()->IsNode(), result.get());

		std::unique_lock _l{ Global::reloadLock };
		Global::GraphInfos.priority_insert(result);

		return result;
	}

	std::shared_ptr<const Race> GetRaceFromGraph(const std::string& s) {
		std::string skeleton = "";
		{
			auto l = Global::RaceLinkMap.GetReadAccess();
			if (auto iter = l->find(s); iter != l->end()) {
				skeleton = iter->second;
			} else {
				return nullptr;
			}
		}

		return GetRace(skeleton);
	}

	std::shared_ptr<const Race> GetRace(RE::Actor* a) {
		if (!a || !a->race)
			return nullptr;

		return GetRaceFromGraph(a->race->behaviorGraphProjectName[0].c_str());
	}

	bool ApplyEquipmentSet(RE::Actor* a, const std::string& id) {
		if (auto set = GetEquipmentSet(id); set != nullptr) {
			set->Apply(a);
			return true;
		} else {
			return false;
		}
	}

	bool ApplyMorphSet(RE::Actor* a, const std::string& id)
	{
		if (auto set = GetMorphSet(id); set != nullptr) {
			set->Apply(a);
			return true;
		} else {
			return false;
		}
	}

	void ActionSet::RunStart(RE::Actor* a) const {
		for (auto iter = std::vector<std::string>::begin(); iter != std::vector<std::string>::end(); iter++) {
			if (auto action = GetAction(*iter); action != nullptr) {
				action->RunStart(a);
			}
		}
	}

	void ActionSet::RunStop(RE::Actor* a) const {
		for (auto iter = std::vector<std::string>::begin(); iter != std::vector<std::string>::end(); iter++) {
			if (auto action = GetAction(*iter); action != nullptr) {
				action->RunStop(a);
			}
		}
	}

	void Action::RunStart(RE::Actor* a) const {
		if (startEquipSet.has_value()) {
			if (auto set = GetEquipmentSet(startEquipSet.value()); set != nullptr) {
				set->Apply(a);
			}
		}
	}

	void Action::RunStop(RE::Actor* a) const {
		if (stopEquipSet.has_value()) {
			if (auto set = GetEquipmentSet(stopEquipSet.value()); set != nullptr) {
				set->Apply(a);
			}
		}
	}

	void Morphs::Apply(RE::Actor* a) const {
		Scene::OrderedActionQueue::ApplyMorphs(a, *this);
	}
}


