#pragma once
#include "NodeAnimation.h"
#include "NodeAnimationGraph.h"

namespace BodyAnimation
{
	class GraphHook
	{
	public:
		typedef bool(GraphUpdate)(RE::IAnimationGraphManagerHolder* a_graphHolder, float* a_deltaTime);
		typedef void(Set3d)(RE::TESObjectREFR* a_ref, RE::NiAVObject* a_object, bool a_queue);
		inline static DetourXS UpdateDetour;
		inline static DetourXS Set3dDetour;
		inline static REL::Relocation<GraphUpdate> OriginalUpdate;
		inline static REL::Relocation<Set3d> OriginalSet3d;

		struct AnimationInfo
		{
			std::string filePath = "";
			std::string id = "";

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(filePath, id);
			}
		};

		struct GraphStateInfo
		{
			Serialization::General::SerializableRefHandle ref;
			AnimationInfo animInfo;
			float animTime = 0.0f;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(ref, animInfo, animTime);
			}
		};

		struct PersistentState
		{
			std::unordered_map<RE::IAnimationGraphManagerHolder*, NodeAnimationGraph> graphs;

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				std::vector<GraphStateInfo> infos;
				for (const auto& pair : graphs) {
					const auto& g = pair.second;
					if (g.IsAnimating() && g.targetHandle.get() != nullptr) {
						infos.push_back({ g.targetHandle, { g.animationFilePath, g.animationId }, g.generator.localTime });
					}
				}
				ar(infos);
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				std::vector<GraphStateInfo> infos;
				ar(infos);
				for (const auto& i : infos) {
					auto ref = i.ref.get();
					if (ref == nullptr)
						continue;

					if (auto animData = LoadAnimation(ref.get(), i.animInfo.filePath, i.animInfo.id); animData != nullptr) {
						StartAnimation(ref.get(), std::move(animData), 0.0f, i.animInfo.filePath, i.animInfo.id);
						if (auto g = GetGraph(ref.get()); g != nullptr) {
							g->generator.localTime = i.animTime;
						}
					}
				}
			}
		};

		inline static std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();
		inline static std::shared_mutex stateLock;
		inline static std::unordered_map<SerializableRefHandle, AnimationInfo> loadingAnims;
		inline static std::mutex loadingAnimsLock;
		inline static std::unordered_map<SerializableRefHandle, std::set<RE::IAnimationGraphManagerHolder*>> regsFor3d;
		inline static std::shared_mutex regsFor3dLock;

		static std::unique_ptr<NodeAnimation> LoadAnimation(RE::TESObjectREFR* ref, const std::string& filePath, const std::string& id)
		{
			if (!ref)
				return nullptr;

			return LoadAnimation(GetGraphInfo(ref), filePath, id);
		}

		static std::unique_ptr<NodeAnimation> LoadAnimation(std::shared_ptr<const Data::GraphInfo> info, const std::string& filePath, const std::string& id) {
			if (!info)
				return nullptr;

			NANIM file;
			if (!file.LoadFromFile(filePath))
				return nullptr;

			std::unique_ptr<NodeAnimation> animData;
			if (!file.GetAnimation(id, info->nodeList, animData))
				return nullptr;

			return animData;
		}

		static bool IsAnimationLoading(RE::TESObjectREFR* ref, const std::string& filePath, const std::string& animId) {
			if (!ref)
				return false;

			std::unique_lock l{ loadingAnimsLock };
			if (auto iter = loadingAnims.find(ref->GetHandle());
				iter != loadingAnims.end() &&
				Utility::StringToLower(iter->second.filePath) == Utility::StringToLower(filePath) &&
				iter->second.id == animId)
			{
				return true;
			} else {
				return false;
			}
		}

		static bool LoadAndPlayAnimation(RE::TESObjectREFR* ref, const std::string& filePath, float transitionDur = 1.3f, const std::string& animName = "default")
		{
			if (!ref)
				return false;

			auto info = GetGraphInfo(ref);
			if (!info)
				return false;

			Serialization::General::SerializableRefHandle hndl = ref->GetHandle();

			std::unique_lock l{ loadingAnimsLock };
			loadingAnims[hndl] = { filePath, animName };

			std::thread([filePath = filePath, animName = animName, info = info, hndl = hndl, transitionDur = transitionDur]() {
				auto animData = LoadAnimation(info, filePath, animName);
				auto ref = hndl.get();

				std::unique_lock l{ loadingAnimsLock };
				if (auto iter = loadingAnims.find(hndl); iter != loadingAnims.end() && iter->second.filePath == filePath && iter->second.id == animName) {
					if (animData != nullptr && ref != nullptr) {
						StartAnimation(ref.get(), std::move(animData), transitionDur, filePath, animName);
					}
					loadingAnims.erase(hndl);
				}

			}).detach();

			return true;
		}

		static bool StartAnimation(RE::TESObjectREFR* ref, std::unique_ptr<NodeAnimation> anim, float transitionDur = 1.3f, const std::string_view& filePath = "", const std::string_view& id = "")
		{
			if (!ref || !anim)
				return false;

			std::unique_lock l{ stateLock };
			auto g = GetOrCreateGraph(ref);
			g->TransitionToAnimation(std::move(anim), transitionDur);
			g->animationFilePath = filePath;
			g->animationId = id;

			return true;
		}

		static bool StopAnimation(RE::TESObjectREFR* ref, float transitionDur = 1.3f) {
			if (!ref)
				return false;

			std::unique_lock l1{ loadingAnimsLock };
			loadingAnims.erase(ref->GetHandle());

			std::unique_lock l2{ stateLock };
			auto g = GetGraph(ref);
			if (!g)
				return false;

			g->TransitionToAnimation(nullptr, transitionDur);
			return true;
		}

		static bool VisitGraph(RE::TESObjectREFR* ref, std::function<void(NodeAnimationGraph*)> visitFunc, bool createIfNeeded = true, bool onlyVisitIfAnimating = false) {
			if (!ref)
				return false;

			std::unique_lock le{ stateLock, std::defer_lock };
			std::shared_lock ls{ stateLock };
			NodeAnimationGraph* g = nullptr;
			g = GetGraph(ref);
			if (g == nullptr && createIfNeeded) {
				//Only acquire exclusive access to the graphs map if a graph needs to be created.
				ls.unlock();
				le.lock();
				g = CreateGraph(ref);
			}

			if (g != nullptr) {
				std::unique_lock l{ g->updateLock };
				if (!onlyVisitIfAnimating || g->IsAnimating()) {
					if (visitFunc != nullptr)
						visitFunc(g);
					return true;
				}
			}

			return false;
		}

		static bool DeleteGraph(RE::TESObjectREFR* ref) {
			if (!ref)
				return false;

			std::unique_lock l{ stateLock };
			state->graphs.erase(ref);
			return true;
		}

		static void RegisterGraphFor3DChange(RE::IAnimationGraphManagerHolder* a_graphHolder, SerializableRefHandle a_ref) {
			std::unique_lock l{ regsFor3dLock };
			regsFor3d[a_ref].insert(a_graphHolder);
		}

		static void UnregisterGraphFor3DChange(RE::IAnimationGraphManagerHolder* a_graphHolder, SerializableRefHandle a_ref) {
			std::unique_lock l{ regsFor3dLock };
			auto& target = regsFor3d[a_ref];
			target.erase(a_graphHolder);
			if (target.empty()) {
				regsFor3d.erase(a_ref);
			}
		}

		static void UnregisterGraphForAll3DChanges(RE::IAnimationGraphManagerHolder* a_graphHolder) {
			std::unique_lock l{ regsFor3dLock };
			for (auto iter = regsFor3d.begin(); iter != regsFor3d.end();) {
				iter->second.erase(a_graphHolder);
				if (iter->second.empty()) {
					iter = regsFor3d.erase(iter);
				} else {
					iter++;
				}
			}
		}

		static void HookedGraphUpdate(RE::IAnimationGraphManagerHolder* a_graphHolder, float* a_deltaTime)
		{
			OriginalUpdate(a_graphHolder, a_deltaTime);
			std::shared_lock l1{ stateLock };

			auto iter = state->graphs.find(a_graphHolder);
			if (iter == state->graphs.end() || !a_graphHolder->ShouldUpdateAnimation())
				return;

			auto& g = iter->second;
			std::unique_lock l2{ g.updateLock };

			g.Update(*a_deltaTime);

			if (g.flags.all(NodeAnimationGraph::kTemporary, NodeAnimationGraph::kNoActiveIKChains) && g.state == NodeAnimationGraph::kIdle) {
				l1.unlock();
				l2.unlock();
				DeleteGraphInternal(a_graphHolder);
			}
		}

		static void HookedSet3D(RE::TESObjectREFR* a_ref, RE::NiAVObject* a_object, bool a_queue) {
			OriginalSet3d(a_ref, a_object, a_queue);
			std::shared_lock l1{ stateLock };
			std::shared_lock l2{ regsFor3dLock };

			auto iter = regsFor3d.find(a_ref->GetHandle());
			if (iter == regsFor3d.end())
				return;

			for (auto& g : iter->second) {
				if (auto iter2 = state->graphs.find(g); iter2 != state->graphs.end()) {
					std::unique_lock l3{ iter2->second.updateLock };
					iter2->second.UpdateNodes(a_ref);
				}
			}
		}

		static void Reset() {
			std::scoped_lock l{ loadingAnimsLock, stateLock, regsFor3dLock };
			state->graphs.clear();
			loadingAnims.clear();
			regsFor3d.clear();
		}

		static void RegisterHook()
		{
			//IAnimationGraphManagerHolder::UpdateAnimationGraphManager(BSAnimationUpdateData)
			REL::Relocation<GraphUpdate> hookLoc1{ REL::ID(1492656) };

			if (UpdateDetour.Create(reinterpret_cast<LPVOID>(hookLoc1.address()), &HookedGraphUpdate)) {
				OriginalUpdate = reinterpret_cast<uintptr_t>(UpdateDetour.GetTrampoline());
			} else {
				logger::warn("Failed to create IAnimationGraphManagerHolder::UpdateAnimationGraphManager hook!");
			}

			//TESObjectREFR::Set3D
			REL::Relocation<Set3d> hookLoc2{ REL::ID(1216262) };

			if (Set3dDetour.Create(reinterpret_cast<LPVOID>(hookLoc2.address()), &HookedSet3D)) {
				OriginalSet3d = reinterpret_cast<uintptr_t>(Set3dDetour.GetTrampoline());
			} else {
				logger::warn("Failed to create TESObjectREFR::Set3D hook!");
			}
		}

	private:
		static NodeAnimationGraph* GetOrCreateGraph(RE::TESObjectREFR* ref) {
			if (auto g = GetGraph(ref); g != nullptr) {
				return g;
			}

			return CreateGraph(ref);
		}

		static NodeAnimationGraph* GetGraph(RE::TESObjectREFR* ref) {
			if (auto iter = state->graphs.find(ref); iter != state->graphs.end()) {
				return &iter->second;
			}

			return nullptr;
		}

		static NodeAnimationGraph* CreateGraph(RE::TESObjectREFR* ref)
		{
			NodeAnimationGraph* result = &state->graphs[ref];
			result->targetHandle = ref->GetHandle();
			result->reg3dCallback = std::bind(RegisterGraphFor3DChange, static_cast<RE::IAnimationGraphManagerHolder*>(ref), std::placeholders::_1);
			result->ikManager.reg3dCallback = &result->reg3dCallback;
			result->unreg3dCallback = std::bind(UnregisterGraphForAll3DChanges, static_cast<RE::IAnimationGraphManagerHolder*>(ref));
			RegisterGraphFor3DChange(ref, ref->GetHandle());
			if (auto info = GetGraphInfo(ref); info != nullptr) {
				result->SetGraphData(*info);
			}
			result->UpdateNodes(ref);
			return result;
		}

		static void DeleteGraphInternal(RE::IAnimationGraphManagerHolder* a_graphHolder) {
			std::unique_lock l{ stateLock };
			state->graphs.erase(a_graphHolder);
		}

		static std::shared_ptr<const Data::GraphInfo> GetGraphInfo(RE::TESObjectREFR* ref) {
			RE::BSFixedString graphName;
			ref->GetAnimationGraphProjectName(graphName);
			if (auto info = Data::GetGraphInfo(graphName.c_str(), ref->As<RE::Actor>()); info != nullptr) {
				return info;
			}
			return nullptr;
		}
	};
}
