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

	inline static void OnIKMessage(const char* msg)
	{
		OutputDebugStringA(std::format("{}\n", msg).c_str());
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

	class IKTwoBoneChain
	{
	public:
		IKTwoBoneChain()
		{
			solver = ik.solver.create(IK_FABRIK);
			nodes[0] = solver->node->create(0);
			nodes[1] = solver->node->create_child(nodes[0], 1);
			nodes[2] = solver->node->create_child(nodes[1], 2);
			effector = solver->effector->create();
			solver->effector->attach(effector, nodes[2]);
			solver->flags |= IK_ENABLE_JOINT_ROTATIONS;
			solver->max_iterations = 50;
			solver->tolerance = 1e-7f;
			effector->chain_length = 2;
			ik.solver.set_tree(solver, nodes[0]);
			ik.solver.rebuild(solver);
		}

		~IKTwoBoneChain()
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
			RE::NiTransform destWorld = MathUtil::ApplyCoordinateSpace(parentWorld, dest->local);
			IKQToNQ(src->rotation).ToRotation(destWorld.rotate);

			RE::NiTransform result = destWorld.WorldToLocal(parentWorld);
			dest->local.rotate = result.rotate;

			return destWorld;
		}

		void SetTargetLocal(const NodeTransform& target, const RE::NiTransform& parentTransform) {
			RE::NiTransform targetWorld;
			target.ToComplex(targetWorld);
			targetWorld = MathUtil::ApplyCoordinateSpace(parentTransform, targetWorld);

			RE::NiQuaternion rot;
			rot.FromRotation(targetWorld.rotate);
			targetRotation = NQToIKQ(rot);
			effector->target_position = N3ToIK3(targetWorld.translate);
		}

		NodeTransform GetTargetLocal(const RE::NiTransform& parentTransform) {
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

		void SetHasPole(bool hasPole) {
			nodes[1]->hasPole = hasPole ? 1 : 0;
		}

		void SetPoleWorld(const RE::NiPoint3& p, const RE::NiTransform& parentTransform) {
			RE::NiPoint3 relative = p;
			relative -= parentTransform.translate;

			RE::NiQuaternion worldRot;
			worldRot.FromRotation(parentTransform.rotate);
			RE::NiQuaternion inverse;
			inverse.UnitInverse(worldRot);

			ik_quat_t ikWorldRot = NQToIKQ(inverse);
			ik_vec3_t ikRelativePos = N3ToIK3(relative);

			ik.vec3.rotate(ikRelativePos.f, ikWorldRot.f);
			relativePole = ikRelativePos;

			SetHasPole(true);
		}

		ik_vec3_t GetPoleWorldIK(const RE::NiTransform& parentTransform) {
			RE::NiQuaternion rot;
			rot.FromRotation(parentTransform.rotate);

			ik_vec3_t result = relativePole;
			ik.vec3.rotate(result.f, NQToIKQ(rot).f);
			ik.vec3.add_vec3(result.f, N3ToIK3(parentTransform.translate).f);
			return result;
		}

		RE::NiPoint3 GetPoleWorld(const RE::NiTransform& parentTransform) {
			return IK3ToN3(GetPoleWorldIK(parentTransform));
		}

		NodeTransform GetTarget() {
			return NodeTransform(IKQToNQ(targetRotation), IK3ToN3(effector->target_position));
		}

		void CalculateParentWorld(RE::NiAVObject* b1, RE::NiNode* rootNode) {
			chainParent = b1->parent;
			chainParentWorld = MathUtil::CalculateWorldAscending(rootNode, b1->parent);
		}

		void CopyToIK(RE::NiAVObject* b1, RE::NiAVObject* b2, RE::NiAVObject* b3)
		{
			RE::NiTransform b1W = CopyNiNodeToIKNode(b1, nodes[0], chainParent, chainParentWorld);
			RE::NiTransform b2W = CopyNiNodeToIKNode(b2, nodes[1], b1->IsNode(), b1W);
			CopyNiNodeToIKNode(b3, nodes[2], b2->IsNode(), b2W);

			ik_transform_chain_list(&solver->chain_list, ik_transform_flags_e::TR_G2L);
		}

		void Solve()
		{
			if (!initialized) {
				ik.solver.rebuild(solver);
				initialized = true;
			}
			ik.solver.solve(solver);
		}

		RE::NiTransform CopyToNodes(RE::NiAVObject* b1, RE::NiAVObject* b2, RE::NiAVObject* b3)
		{
			ik_transform_chain_list(&solver->chain_list, ik_transform_flags_e::TR_L2G);
			RE::NiTransform b1W = CopyIKNodeToNiNode(nodes[0], b1, chainParent, chainParentWorld);
			RE::NiTransform b2W = CopyIKNodeToNiNode(nodes[1], b2, b1->IsNode(), b1W);
			CopyIKNodeToNiNode(nodes[2], b3, b2->IsNode(), b2W);
			return b2W;
		}

		void SolveAndApply(RE::NiAVObject* b1, RE::NiAVObject* b2, RE::NiAVObject* b3, RE::NiNode* rootNode, RE::NiAVObject* poleParent)
		{
			CalculateParentWorld(b1, rootNode);

			if (nodes[1]->hasPole > 0) {
				nodes[1]->pole = GetPoleWorldIK(MathUtil::CalculateWorldAscending(rootNode, poleParent));
			}

			RE::NiTransform b2W;
			for (size_t i = 0; i < 10; i++) {
				CopyToIK(b1, b2, b3);
				Solve();
				b2W = CopyToNodes(b1, b2, b3);
			}

			//Override the last node's rotation with our target world rotation.
			nodes[2]->rotation = targetRotation;
			CopyIKNodeToNiNode(nodes[2], b3, b2->IsNode(), b2W);
		}

		void Reset() {
			initialized = false;
		}

		bool initialized = false;
		ik_vec3_t relativePole;

	private:

		RE::NiNode* chainParent;
		RE::NiTransform chainParentWorld;
		std::array<ik_node_t*, 3> nodes;
		ik_quat_t targetRotation;
		ik_solver_t* solver;
		ik_effector_t* effector;
	};

	struct IKHolder
	{
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
		virtual RE::NiPoint3 GetPole() = 0;
		virtual void SetPoleLocal(const RE::NiPoint3& p) = 0;
		virtual RE::NiPoint3 GetPoleLocal() = 0;

		virtual ~IKHolder(){};
	};

	struct IKTwoBoneHolder : public IKHolder
	{
		std::array<std::string, 3> boneNames;
		std::string poleParentName;
		RE::NiPointer<RE::NiNode> rootNode = nullptr;
		RE::NiPointer<RE::NiAVObject> poleParent = nullptr;
		RE::NiPointer<RE::NiAVObject> targetNodes[3];
		IKTwoBoneChain chain;

		IKTwoBoneHolder(const std::string_view& b1, const std::string_view& b2, const std::string_view& b3, const std::string& poleParentName, const RE::NiPoint3& poleStartPos) :
			poleParentName(poleParentName)
		{
			boneNames[0] = b1;
			boneNames[1] = b2;
			boneNames[2] = b3;
			SetPoleLocal(poleStartPos);
		}

		virtual void Reset()
		{
			if (targetNodes[2] != nullptr) {
				SetTarget(targetNodes[2]->world);
			}
		}

		virtual void SetTarget(const NodeTransform& target)
		{
			chain.SetTarget(target);
		}

		virtual NodeTransform GetTarget() {
			return chain.GetTarget();
		}

		virtual void SetTargetLocal(const NodeTransform& target) {
			if (rootNode != nullptr) {
				chain.SetTargetLocal(target, rootNode->world);
			}
		}

		virtual NodeTransform GetTargetLocal() {
			if (rootNode != nullptr) {
				return chain.GetTargetLocal(rootNode->world);
			}
			return NodeTransform::Identity();
		}

		virtual void SetPole(const RE::NiPoint3& p) {
			if (poleParent != nullptr) {
				chain.SetPoleWorld(p, poleParent->world);
			}
		}

		virtual RE::NiPoint3 GetPole() {
			if (poleParent != nullptr) {
				return chain.GetPoleWorld(poleParent->world);
			}
			return { 0.0f, 0.0f, 0.0f };
		}

		virtual void SetPoleLocal(const RE::NiPoint3& p) {
			chain.relativePole = N3ToIK3(p);
			chain.SetHasPole(true);
		}

		virtual RE::NiPoint3 GetPoleLocal() {
			return IK3ToN3(chain.relativePole);
		}

		virtual void ClearNodes()
		{
			for (size_t i = 0; i < 3; i++) {
				targetNodes[i].reset();
			}
			rootNode.reset();
			poleParent.reset();
		}

		virtual void GetTargetNodes(const std::vector<RE::NiPointer<RE::NiAVObject>>& nodes, const std::vector<std::string>& nodeMap, RE::NiAVObject* root) override
		{
			auto nodeIdxMap = Utility::VectorToIndexMap(nodeMap);
			rootNode.reset(root->IsNode());
			poleParent.reset();

			if (auto iter = nodeIdxMap.find(poleParentName); iter != nodeIdxMap.end()) {
				poleParent = nodes[iter->second];
			}

			for (size_t i = 0; i < 3; i++) {
				if (auto iter = nodeIdxMap.find(boneNames[i]); iter != nodeIdxMap.end()) {
					targetNodes[i] = nodes[iter->second];
				}
			}
		}

		void DebugLogMidTransform()
		{
			RE::NiQuaternion temp;
			temp.FromRotation(targetNodes[1]->world.rotate);
			OutputDebugStringA(std::format("W: {:.3f}, X: {:.3f}, Y: {:.3f}, Z: {:.3f}\n", temp.w, temp.x, temp.y, temp.z).c_str());
		}

		virtual void Update()
		{
			if (poleParent == nullptr)
				return;

			if (rootNode == nullptr)
				return;

			for (size_t i = 0; i < 3; i++) {
				if (targetNodes[i] == nullptr)
					return;
			}

			chain.SolveAndApply(targetNodes[0].get(), targetNodes[1].get(), targetNodes[2].get(), rootNode.get(), poleParent.get());
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

		bool GetChainEnabled(const std::string& id) {
			bool result = false;
			for (auto& h : holders) {
				if (h->holderId == id) {
					result = h->holderEnabled;
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

		inline static NodeTransform GetWorldTransform(const IKMapping& m) {
			switch (m.target) {
			case IKMapping::kPole:
				return {
					{ 0, 0, 0, 0 },
					{ m.holder->GetPole() }
				};
			case IKMapping::kEffector:
				return m.holder->GetTarget();
			default:
				return NodeTransform::Identity();
			}
		}

	private:
		const std::function<void(bool)> chainActiveCallback;
		const std::vector<std::string>& nodeMap;
		std::vector<IKMapping> mappings;
		std::map<size_t, IKMapping> lookupMap;
		std::vector<std::unique_ptr<IKHolder>> holders;
	};
}
