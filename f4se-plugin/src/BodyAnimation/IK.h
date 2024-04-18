#pragma once
#include "Misc/GameUtil.h"
#include "ik/transform.h"

namespace BodyAnimation
{
	//Important facts about the IK system:
	// - The IK library and the Creation Engine handle local rotations in different ways.
	//   Due to this, transforms must be handled in world space. However, when the IK chain
	//   updates, we only have access to the bones' updated local transforms. So, the skeleton
	//   hierarchy has to be either ascended or descended through to translate the local transforms
	//   into world space and vice versa. Ascending is massively more efficient.
	// - The parent of the first bone of a chain has its world transform calculated at the start
	//   of a solve, then this is used as the target root for CalculateWorldAscending. This makes
	//   ascending even more efficient, but comes with a stipulation.
	// - The second and onward bones in an IK chain MUST be somewhere below the first bone's parent
	//   hierarchy-wise, otherwise, CalculateWorldAscending will never find what it's looking for
	//   and likely output the wrong transform.
	// - Thanks to everything being handled in world space, IK chains can 'skip' bones in the hierarchy.
	//   An easy example of this are the arms on the human skeleton. Their hierarchy goes
	//   Shoulder->Elbow->TwistBone1->TwistBone2->Hand. However, we can just have a chain target the
	//   Shoulder, Elbow & Hand and it will solve properly.

	inline static void OnIKMessage(const char*)
	{
		//OutputDebugStringA(std::format("{}\n", msg).c_str());
	}

	inline static bool IK_LIB_INITIALIZED = ([]() {
		ik.init();
		ik.log.init();
		ik.log.set_severity(IK_DEBUG);
		static ik_callback_interface_t callbacks;
		callbacks.on_log_message = &OnIKMessage;
		callbacks.on_node_destroy = nullptr;
		ik.implement_callbacks(&callbacks);
		return true;
	})();

	ik_vec3_t N3ToIK3(const RE::NiPoint3& p) {
		return ik.vec3.vec3(p.x, p.y, p.z);
	}

	ik_quat_t NQToIKQ(RE::NiQuaternion q)
	{
		q = MathUtil::NormalizeQuat(q);
		return ik.quat.quat(q.x, q.y, q.z, q.w);
	}

	RE::NiQuaternion IKQToNQ (const ik_quat_t& q) {
		return RE::NiQuaternion{
			static_cast<float>(q.w),
			static_cast<float>(q.x),
			static_cast<float>(q.y),
			static_cast<float>(q.z)
		};
	}

	RE::NiPoint3 IK3ToN3(const ik_vec3_t& p) {
		return RE::NiPoint3{
			static_cast<float>(p.x),
			static_cast<float>(p.y),
			static_cast<float>(p.z)
		};
	}

	class FABRIKChain
	{
	public:
		FABRIKChain(uint16_t numNodes)
		{
			solver = ik.solver.create(IK_FABRIK);
			nodes.reserve(numNodes);
			nodes.push_back(solver->node->create(0));
			for (uint16_t i = 1; i < numNodes; i++) {
				nodes.push_back(solver->node->create_child(nodes.back(), i));
			}
			effector = solver->effector->create();
			solver->effector->attach(effector, nodes.back());
			solver->flags |= IK_ENABLE_JOINT_ROTATIONS;
			solver->max_iterations = 50;
			solver->tolerance = 1e-7f;
			effector->chain_length = numNodes - 1;
			ik.solver.set_tree(solver, nodes[0]);
			ik.solver.rebuild(solver);
			dirsCache.resize(numNodes);
		}

		~FABRIKChain()
		{
			ik.solver.destroy(solver);
		}

		void CopyTransformToIKNode(const RE::NiTransform& src, ik_node_t* dest)
		{
			NodeTransform tempNT = src;
			dest->rotation = NQToIKQ(tempNT.rotate);
			dest->position = N3ToIK3(tempNT.translate);
		}

		RE::NiTransform CopyNiNodeToIKNode(const RE::NiAVObject* src, ik_node_t* dest, RE::NiNode* potentialParent, const RE::NiTransform& potParentWorld)
		{
			RE::NiTransform srcWorld;
			if (src->parent == potentialParent) {
				srcWorld = MathUtil::ApplyCoordinateSpace(potParentWorld, src->local);
			} else {
				srcWorld = MathUtil::CalculateWorldAscending(chainParent, src, &chainParentWorld);
			}
			CopyTransformToIKNode(srcWorld, dest);
			return srcWorld;
		}

		RE::NiTransform CopyIKNodeToNiNode(const ik_node_t* src, RE::NiAVObject* dest, RE::NiNode* potentialParent, const RE::NiTransform& potParentWorld)
		{
			RE::NiTransform parentWorld;
			if (dest->parent == potentialParent) {
				parentWorld = potParentWorld;
			} else {
				parentWorld = MathUtil::CalculateWorldAscending(chainParent, dest->parent, &chainParentWorld);
			}

			RE::NiTransform destWorld;

			if (usePositions) {
				destWorld.translate = IK3ToN3(src->position);
			} else {
				destWorld = MathUtil::ApplyCoordinateSpace(parentWorld, dest->local);
			}
			
			IKQToNQ(src->rotation).ToRotation(destWorld.rotate);

			RE::NiTransform result = destWorld.WorldToLocal(parentWorld);
			dest->local.rotate = result.rotate;
			if (usePositions) {
				dest->local.translate = result.translate;
			}

			return destWorld;
		}

		void SetTargetLocal(const NodeTransform& target, const RE::NiTransform& parentTransform)
		{
			RE::NiTransform targetWorld;
			target.ToComplex(targetWorld);
			targetWorld = MathUtil::ApplyCoordinateSpace(parentTransform, targetWorld);

			RE::NiQuaternion rot;
			rot.FromRotation(targetWorld.rotate);
			targetRotation = NQToIKQ(rot);
			effector->target_position = N3ToIK3(targetWorld.translate);
		}

		NodeTransform GetTargetLocal(const RE::NiTransform& parentTransform)
		{
			RE::NiTransform targetWorld;
			IKQToNQ(targetRotation).ToRotation(targetWorld.rotate);
			targetWorld.translate = IK3ToN3(effector->target_position);

			return targetWorld.WorldToLocal(parentTransform);
		}

		void SetTarget(const NodeTransform& t)
		{
			targetRotation = NQToIKQ(t.rotate);
			effector->target_position = N3ToIK3(t.translate);
		}

		void SetHasPole(bool hasPole)
		{
			nodes[1]->hasPole = hasPole ? 1 : 0;
		}

		void SetPoleWorld(const RE::NiPoint3& p, const RE::NiTransform& parentTransform, bool parentIsEffector = false)
		{
			RE::NiPoint3 relative = p;
			relative -= parentIsEffector ? IK3ToN3(effector->target_position) : parentTransform.translate;

			ik_quat_t parentRotInverse = targetRotation;
			if (!parentIsEffector) {
				RE::NiQuaternion worldRot;
				worldRot.FromRotation(parentTransform.rotate);
				RE::NiQuaternion inverse;
				inverse.UnitInverse(worldRot);
				parentRotInverse = NQToIKQ(inverse);
			} else {
				ik.quat.conj(parentRotInverse.f);
			}

			ik_quat_t ikWorldRot = parentRotInverse;
			ik_vec3_t ikRelativePos = N3ToIK3(relative);

			ik.vec3.rotate(ikRelativePos.f, ikWorldRot.f);
			relativePole = ikRelativePos;
		}

		ik_vec3_t GetPoleWorldIK(const NodeTransform& parentTransform)
		{
			return GetPoleWorldIK(N3ToIK3(parentTransform.translate), NQToIKQ(parentTransform.rotate));
		}

		ik_vec3_t GetPoleWorldIK(const ik_vec3_t& parentPos, const ik_quat_t& parentRot)
		{
			ik_vec3_t result = relativePole;
			ik.vec3.rotate(result.f, parentRot.f);
			ik.vec3.add_vec3(result.f, parentPos.f);
			return result;
		}

		RE::NiPoint3 GetPoleWorld(const RE::NiTransform& parentTransform, bool parentIsEffector = false)
		{
			return IK3ToN3(parentIsEffector ? GetPoleWorldIK(effector->target_position, targetRotation) : GetPoleWorldIK(parentTransform));
		}

		NodeTransform GetTarget()
		{
			return NodeTransform(IKQToNQ(targetRotation), IK3ToN3(effector->target_position));
		}

		void CalculateParentWorld(RE::NiAVObject* b1, RE::NiNode* rootNode)
		{
			chainParent = b1->parent;
			chainParentWorld = MathUtil::CalculateWorldAscending(rootNode, b1->parent);
		}

		void CopyToIK(const std::span<RE::NiPointer<RE::NiAVObject>>& bones)
		{
			CopyToIKNoTransform(bones);
			ik_transform_chain_list(&solver->chain_list, ik_transform_flags_e::TR_G2L);
		}

		void CopyToIKNoTransform(const std::span<RE::NiPointer<RE::NiAVObject>>& bones)
		{
			RE::NiAVObject* curBone = bones[0].get();
			RE::NiTransform lastWorld = CopyNiNodeToIKNode(curBone, nodes[0], chainParent, chainParentWorld);
			RE::NiAVObject* lastBone = curBone;

			for (size_t i = 1; i < nodes.size(); i++) {
				curBone = bones[i].get();
				lastWorld = CopyNiNodeToIKNode(curBone, nodes[i], lastBone->IsNode(), lastWorld);
				lastBone = curBone;
			}
		}

		void Solve()
		{
			if (!initialized) {
				ik.solver.rebuild(solver);
				initialized = true;
			}
			ik.solver.solve(solver);
		}

		void CopyToNodes(const std::span<RE::NiPointer<RE::NiAVObject>>& bones)
		{
			ik_transform_chain_list(&solver->chain_list, ik_transform_flags_e::TR_L2G);
			CopyToNodesNoTransform(bones);
		}

		void CopyToNodesNoTransform(const std::span<RE::NiPointer<RE::NiAVObject>>& bones)
		{
			RE::NiAVObject* curBone = bones[0].get();
			RE::NiTransform lastWorld = CopyIKNodeToNiNode(nodes[0], curBone, chainParent, chainParentWorld);
			RE::NiAVObject* lastBone = curBone;

			for (size_t i = 1; i < nodes.size(); i++) {
				curBone = bones[i].get();
				lastWorld = CopyIKNodeToNiNode(nodes[i], curBone, lastBone->IsNode(), lastWorld);
				lastBone = curBone;
			}
		}

		void SolveAndApply(const std::span<RE::NiPointer<RE::NiAVObject>>& bones, RE::NiNode* rootNode, RE::NiAVObject* poleParent)
		{
			if (!bones[0]->parent) {
				return;
			}

			CalculateParentWorld(bones[0].get(), rootNode);

			if (nodes[1]->hasPole > 0 && poleParent != nullptr) {
				if (poleParent == bones[2].get()) {
					nodes[1]->pole = GetPoleWorldIK(effector->target_position, targetRotation);
				} else {
					nodes[1]->pole = GetPoleWorldIK(MathUtil::CalculateWorldAscending(rootNode, poleParent));
				}
			}

			if (usePositions) {
				CopyToIKNoTransform(bones);

				ik_vec3_t endPos = nodes.back()->position;
				ik_vec3_t startPos = nodes.front()->position;
				ik.vec3.sub_vec3(endPos.f, startPos.f);
				double endNodeDist = ik.vec3.length(endPos.f);

				endPos = effector->target_position;
				ik.vec3.sub_vec3(endPos.f, startPos.f);
				double effectorDist = ik.vec3.length(endPos.f);

				if (std::fabs(endNodeDist - effectorDist) > 0.001f) {
					double segmentLength = effectorDist / static_cast<double>(nodes.size() - 1);

					for (size_t i = 1; i < nodes.size(); i++) {
						ik_vec3_t dir = nodes[i]->position;
						ik.vec3.sub_vec3(dir.f, nodes[i - 1]->position.f);
						ik.vec3.normalize(dir.f);
						ik.vec3.mul_scalar(dir.f, segmentLength);
						dirsCache[i] = dir;
					}

					for (size_t i = 1; i < nodes.size(); i++) {
						nodes[i]->position = nodes[i - 1]->position;
						ik.vec3.add_vec3(nodes[i]->position.f, dirsCache[i].f);
					}

					CopyToNodesNoTransform(bones);
					ik_transform_chain_list(&solver->chain_list, ik_transform_flags_e::TR_G2L);
					ik.solver.update_distances(solver);
				}
			}

			for (size_t i = 0; i < 10; i++) {
				CopyToIK(bones);
				Solve();
				CopyToNodes(bones);
			}

			//Override the last node's rotation with our target world rotation.
			nodes.back()->rotation = targetRotation;
			CopyToNodesNoTransform(bones);
		}

		void Reset()
		{
			initialized = false;
		}

		bool usePositions = false;
		bool initialized = false;
		ik_vec3_t relativePole;
		ik_solver_t* solver;

	private:

		std::vector<ik_vec3_t> dirsCache;
		RE::NiNode* chainParent;
		RE::NiTransform chainParentWorld;
		std::vector<ik_node_t*> nodes;
		ik_quat_t targetRotation;
		ik_effector_t* effector;
	};

	struct IKHolder
	{
		std::string targetNode;
		SerializableRefHandle targetRef;
		std::string holderId = "";
		bool holderEnabled = false;

		virtual void ClearNodes() = 0;
		virtual void GetTargetNodes(const std::vector<RE::NiPointer<RE::NiAVObject>>& nodes, const std::vector<std::string>& nodeMap, RE::NiAVObject* rootNode) = 0;
		virtual void Update() = 0;
		virtual void SetTarget(const NodeTransform& target) = 0;
		virtual NodeTransform GetTarget() = 0;
		virtual void SetTargetLocal(const NodeTransform& target) = 0;
		virtual NodeTransform GetTargetLocal() = 0;
		virtual void Reset() {}
		virtual void SetPole(const RE::NiPoint3& p) = 0;
		virtual NodeTransform GetPole() = 0;
		virtual void SetPoleLocal(const RE::NiPoint3& p) = 0;
		virtual RE::NiPoint3 GetPoleLocal() = 0;
		virtual void SetTargetParent(RE::NiAVObject* parent, RE::NiNode* parentRoot) = 0;
		virtual RE::NiMatrix3 GetTargetParentRotation() = 0;
		virtual void SetControlsTranslation(bool set) = 0;

		virtual ~IKHolder(){};
	};

	struct IKArbitraryHolder : public IKHolder
	{
		std::vector<std::string> boneNames;
		std::vector<RE::NiPointer<RE::NiAVObject>> bones;
		RE::NiPointer<RE::NiNode> rootNode = nullptr;
		FABRIKChain chain;

		RE::NiPointer<RE::NiAVObject> targetParent = nullptr;
		RE::NiPointer<RE::NiNode> targetParentRoot = nullptr;

		IKArbitraryHolder(const std::vector<std::string>& a_boneNames) :
			chain(static_cast<uint16_t>(a_boneNames.size()))
		{
			boneNames = a_boneNames;
			bones.resize(a_boneNames.size());
		}

		virtual void Reset()
		{
			if (targetParent != nullptr) {
				SetTarget(targetParent->world);
			} else if (bones.back() != nullptr) {
				SetTarget(bones.back()->world);
			}
		}

		virtual void SetTarget(const NodeTransform& target)
		{
			chain.SetTarget(target);
		}

		virtual NodeTransform GetTarget()
		{
			return chain.GetTarget();
		}

		virtual void SetTargetLocal(const NodeTransform& target)
		{
			if (targetParent != nullptr) {
				if (targetParentRoot != nullptr) {
					chain.SetTargetLocal(target, MathUtil::CalculateWorldAscending(targetParentRoot.get(), targetParent.get()));
				} else {
					chain.SetTargetLocal(target, targetParent->world);
				}
			} else if (rootNode != nullptr) {
				chain.SetTargetLocal(target, rootNode->world);
			}
		}

		virtual NodeTransform GetTargetLocal()
		{
			if (targetParent != nullptr) {
				return chain.GetTargetLocal(targetParent->world);
			} else if (rootNode != nullptr) {
				return chain.GetTargetLocal(rootNode->world);
			}
			return NodeTransform::Identity();
		}

		virtual void SetTargetParent(RE::NiAVObject* parent, RE::NiNode* parentRoot)
		{
			targetParent.reset(parent);
			targetParentRoot.reset(parentRoot);
		}

		virtual void SetPole(const RE::NiPoint3&)
		{
		}

		virtual NodeTransform GetPole()
		{
			return NodeTransform::Identity();
		}

		virtual void SetPoleLocal(const RE::NiPoint3&)
		{
		}

		virtual RE::NiPoint3 GetPoleLocal()
		{
			return { .0f, .0f, .0f };
		}

		virtual RE::NiMatrix3 GetTargetParentRotation()
		{
			if (targetParent != nullptr) {
				return targetParent->world.rotate;
			} else if (rootNode != nullptr) {
				return rootNode->world.rotate;
			}
			RE::NiMatrix3 res;
			res.MakeIdentity();
			return res;
		}

		virtual void ClearNodes()
		{
			for (auto& b : bones) {
				b.reset();
			}
			rootNode.reset();
		}

		virtual void GetTargetNodes(const std::vector<RE::NiPointer<RE::NiAVObject>>& nodes, const std::vector<std::string>& nodeMap, RE::NiAVObject* root) override
		{
			auto nodeIdxMap = Utility::VectorToIndexMap(nodeMap);
			rootNode.reset(root->IsNode());

			for (size_t i = 0; i < boneNames.size(); i++) {
				if (auto iter = nodeIdxMap.find(boneNames[i]); iter != nodeIdxMap.end()) {
					bones[i] = nodes[iter->second];
				}
			}

			if (auto curTarget = chain.GetTarget();
				curTarget.translate.x == 0.0f &&
				curTarget.translate.y == 0.0f &&
				curTarget.translate.z == 0.0f) {
				Reset();
			}
		}

		virtual void SetControlsTranslation(bool set)
		{
			chain.usePositions = set;
			if (set) {
				chain.solver->flags &= ~IK_ENABLE_JOINT_ROTATIONS;
			} else {
				chain.solver->flags |= IK_ENABLE_JOINT_ROTATIONS;
			}
		}

		virtual void Update()
		{
			if (rootNode == nullptr)
				return;

			for (const auto& b : bones) {
				if (b == nullptr) {
					return;
				}
			}

			chain.SolveAndApply(bones, rootNode.get(), nullptr);
		}
	};

	struct IKTwoBoneHolder : public IKArbitraryHolder
	{
		std::string poleParentName;
		RE::NiPointer<RE::NiAVObject> poleParent = nullptr;

		IKTwoBoneHolder(const std::vector<std::string>& a_boneNames, const std::string& poleParentName, const RE::NiPoint3& poleStartPos) :
			IKArbitraryHolder(a_boneNames), poleParentName(poleParentName)
		{
			SetPoleLocal(poleStartPos);
		}

		virtual void SetPole(const RE::NiPoint3& p) {
			if (poleParent != nullptr) {
				chain.SetPoleWorld(p, poleParent->world, poleParent.get() == bones.back().get());
			}
		}

		virtual NodeTransform GetPole() {
			if (poleParent != nullptr) {
				NodeTransform result;
				result.translate = chain.GetPoleWorld(poleParent->world, poleParent.get() == bones.back().get());
				result.rotate.FromRotation(poleParent->world.rotate);
				return result;
			}
			return NodeTransform::Identity();
		}

		virtual void SetPoleLocal(const RE::NiPoint3& p) {
			chain.relativePole = N3ToIK3(p);
			chain.SetHasPole(true);
		}

		virtual RE::NiPoint3 GetPoleLocal() {
			return IK3ToN3(chain.relativePole);
		}

		virtual void GetTargetNodes(const std::vector<RE::NiPointer<RE::NiAVObject>>& nodes, const std::vector<std::string>& nodeMap, RE::NiAVObject* root) override
		{
			IKArbitraryHolder::GetTargetNodes(nodes, nodeMap, root);
			poleParent.reset();

			for (size_t i = 0; i < nodeMap.size(); i++) {
				if (nodeMap[i] == poleParentName) {
					poleParent = nodes[i];
					break;
				}
			}
		}

		virtual void Update()
		{
			if (rootNode == nullptr || poleParent == nullptr)
				return;

			for (const auto& b : bones) {
				if (b == nullptr) {
					return;
				}
			}

			chain.SolveAndApply(bones, rootNode.get(), poleParent.get());
		}
	};

	class IKManager
	{
	public:
		struct IKMapping
		{
			enum Type
			{
				kEffector,
				kPole
			};

			std::string nodeName;
			size_t cachedNodeIndex;
			Type target;
			IKHolder* holder;
		};

		IKManager(const std::vector<std::string>& nodeMap, const std::function<void(bool)>& chainActiveCallback) :
			nodeMap(nodeMap), chainActiveCallback(chainActiveCallback) {}

		const std::vector<IKMapping>& GetMappings() {
			return mappings;
		}

		const std::map<size_t, IKMapping>& GetLookupMap() {
			return lookupMap;
		}

		void UpdateMappings() {
			auto nodeIdxMap = Utility::VectorToIndexMap(nodeMap);
			lookupMap.clear();
			for (auto& m : mappings) {
				if (auto iter = nodeIdxMap.find(m.nodeName); iter != nodeIdxMap.end()) {
					m.cachedNodeIndex = iter->second;
				} else {
					m.cachedNodeIndex = UINT64_MAX;
				}
				lookupMap[m.cachedNodeIndex] = m;
			}
		}

		IKHolder* AddChain(std::unique_ptr<IKHolder> h, const std::optional<std::string>& effectorName, const std::optional<std::string>& poleName = std::nullopt) {
			holders.push_back(std::move(h));
			auto hPtr = holders.back().get();

			if (effectorName.has_value()) {
				mappings.emplace_back(effectorName.value(), UINT64_MAX, IKMapping::kEffector, hPtr);
			}

			if (poleName.has_value()) {
				mappings.emplace_back(poleName.value(), UINT64_MAX, IKMapping::kPole, hPtr);
			}
			return hPtr;
		}

		std::vector<std::string> GetChainList() {
			std::vector<std::string> result;
			for (auto& h : holders) {
				result.push_back(h->holderId);
			}
			return result;
		}

		void SetChainEnabled(const std::string& id, bool a_enabled) {
			bool oneEnabled = false;
			for (auto& h : holders) {
				if (h->holderId == id) {
					h->holderEnabled = a_enabled;
				}
				oneEnabled = oneEnabled || h->holderEnabled;
			}
			chainActiveCallback(oneEnabled);
		}

		bool GetChainEnabled(const std::string& id)
		{
			bool result = false;
			for (auto& h : holders) {
				if (h->holderId == id) {
					result = h->holderEnabled;
					break;
				}
			}
			return result;
		}

		void SetChainTarget(const std::string& id, const NodeTransform& a_target) {
			for (auto& h : holders) {
				if (h->holderId == id) {
					h->SetTarget(a_target);
					break;
				}
			}
		}

		void SetChainTargetParent(const std::string& id, SerializableRefHandle a_ref, const std::string& a_nodeName) {
			for (auto& h : holders) {
				if (h->holderId == id) {
					h->targetRef = a_ref;
					h->targetNode = a_nodeName;
					h->SetTargetParent(nullptr, nullptr);
					if (auto refPtr = a_ref.get(); refPtr != nullptr) {
						if (auto _3d = refPtr->Get3D(); _3d != nullptr) {
							h->SetTargetParent(_3d->GetObjectByName(h->targetNode), _3d->IsNode());
							if (reg3dCallback != nullptr) {
								(*reg3dCallback)(a_ref);
							}
						}
					}
					break;
				}
			}
		}

		void ClearChainTargetParent(const std::string& id) {
			for (auto& h : holders) {
				if (h->holderId == id) {
					h->targetRef.reset();
					h->targetNode.clear();
					h->SetTargetParent(nullptr, nullptr);
					break;
				}
			}
		}

		std::pair<SerializableRefHandle, std::string> GetChainTargetParent(const std::string& id) {
			std::pair<SerializableRefHandle, std::string> result;
			for (auto& h : holders) {
				if (h->holderId == id) {
					if (h->targetRef.get() != nullptr) {
						result.first = h->targetRef;
						result.second = h->targetNode;
					}
					break;
				}
			}
			return result;
		}

		void ResetChainTarget(const std::string& id) {
			for (auto& h : holders) {
				if (h->holderId == id) {
					h->Reset();
					return;
				}
			}
		}

		bool RemoveChain(IKHolder* targetHolder) {
			bool hadChain = false;
			for (auto iter = holders.begin(); iter != holders.end(); iter++) {
				if (iter->get() == targetHolder) {
					holders.erase(iter);
					hadChain = true;
					break;
				}
			}
			if (hadChain) {
				for (auto iter = mappings.begin(); iter != mappings.end();) {
					if (iter->holder == targetHolder) {
						iter = mappings.erase(iter);
					} else {
						iter++;
					}
				}
			}
			return hadChain;
		}

		void ClearChains() {
			mappings.clear();
			holders.clear();
			lookupMap.clear();
		}

		void ClearNodes() {
			for (auto& h : holders) {
				h->ClearNodes();
			}
		}

		void GetNodes(const std::vector<RE::NiPointer<RE::NiAVObject>>& nodes, RE::NiAVObject* root){
			for (auto& h : holders) {
				h->GetTargetNodes(nodes, nodeMap, root);
			}
		}

		void OnOther3DChange(RE::TESObjectREFR* a_ref) {
			if (!a_ref)
				return;

			auto hndl = a_ref->GetHandle();
			auto _3d = a_ref->Get3D();
			for (auto& h : holders) {
				if (h->targetRef.hash() == hndl.native_handle_const()) {
					h->SetTargetParent(nullptr, nullptr);
					if (_3d != nullptr) {
						h->SetTargetParent(_3d->GetObjectByName(h->targetNode), _3d->IsNode());
					}
				}
			}
		}

		void Update(const std::vector<NodeTransform>& transforms = {}, bool solve = true)
		{
			for (auto& m : mappings) {
				if (m.cachedNodeIndex < transforms.size() && !transforms[m.cachedNodeIndex].IsIdentity()) {
					SetLocalTransform(m, transforms[m.cachedNodeIndex]);
				}
			}
			if (solve) {
				for (auto& h : holders) {
					if (h->holderEnabled)
						h->Update();
				}
			}
		}

		inline static void SetLocalTransform(const IKMapping& m, const NodeTransform& t) {
			switch (m.target) {
			case IKMapping::kPole:
				m.holder->SetPoleLocal(t.translate);
				break;
			case IKMapping::kEffector:
				m.holder->SetTargetLocal(t);
				break;
			}
		}

		inline static NodeTransform GetLocalTransform(const IKMapping& m) {
			switch (m.target) {
			case IKMapping::kPole:
				return {
					{ 0, 0, 0, 0 },
					{ m.holder->GetPoleLocal() }
				};
			case IKMapping::kEffector:
				return m.holder->GetTargetLocal();
			default:
				return NodeTransform::Identity();
			}
		}

		inline static void SetWorldTransform(const IKMapping& m, const NodeTransform& t) {
			switch (m.target) {
			case IKMapping::kPole:
				m.holder->SetPole(t.translate);
				break;
			case IKMapping::kEffector:
				m.holder->SetTarget(t);
				break;
			}
		}

		inline static NodeTransform GetWorldTransform(const IKMapping& m, bool parentRotation = false) {
			NodeTransform result;
			switch (m.target) {
			case IKMapping::kPole:
				result = m.holder->GetPole();
				break;
			case IKMapping::kEffector:
				result = m.holder->GetTarget();
				if (parentRotation)
					result.rotate.FromRotation(m.holder->GetTargetParentRotation());
				break;
			default:
				result.MakeIdentity();
			}

			return result;
		}

		RegisterFor3DChangeFunctor* reg3dCallback = nullptr;

	private:
		const std::function<void(bool)> chainActiveCallback;
		const std::vector<std::string>& nodeMap;
		std::vector<IKMapping> mappings;
		std::map<size_t, IKMapping> lookupMap;
		std::vector<std::unique_ptr<IKHolder>> holders;
	};
}
