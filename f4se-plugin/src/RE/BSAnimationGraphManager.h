#pragma once

namespace RE
{
	class AnimVariableCacheInfo;
	class BSAnimationGraphChannel;
	class BShkbAnimationGraph;
	class hkbVariableValue;
	struct hkbGeneratorSyncInfo;

	struct BSAnimationGraphVariableCache
	{
	public:
		// members
		BSTArray<AnimVariableCacheInfo> variableCache;         // 00
		BSTArray<hkbVariableValue*> variableQuickLookup;       // 18
		BSSpinLock* lock;                                      // 30
		BSTSmartPointer<BShkbAnimationGraph> graphToCacheFor;  // 38
	};
	static_assert(sizeof(BSAnimationGraphVariableCache) == 0x40);

	class BSAnimationGraphManager :
		public BSTEventSink<BSAnimationGraphEvent>,  //00
		public BSIntrusiveRefCounted                 //08
	{
	public:
		struct DependentManagerSmartPtr
		{
		public:
			// members
			std::uint64_t ptrAndFlagsStorage;  // 00
		};
		static_assert(sizeof(DependentManagerSmartPtr) == 0x08);

		// members
		BSTArray<BSTSmartPointer<BSAnimationGraphChannel>> boundChannel;   // 10
		BSTArray<BSTSmartPointer<BSAnimationGraphChannel>> bumpedChannel;  // 28
		BSTSmallArray<BSTSmartPointer<BShkbAnimationGraph>, 1> graph;      // 40
		BSTArray<DependentManagerSmartPtr> subManagers;                    // 58
		BSTArray<BSTTuple<BSFixedString, BSFixedString>> eventQueuea;      // 70
		BSAnimationGraphVariableCache variableCache;                       // 88
		BSSpinLock updateLock;                                             // C8
		BSSpinLock dependentManagerLock;                                   // D0
		std::uint32_t activeGraph;                                         // D8
		std::uint32_t generateDepth;                                       // DC
	};
	static_assert(sizeof(BSAnimationGraphManager) == 0xE0);

	enum BSVisitControl : uint32_t
	{
		kContinue = 0,
		kStop = 1
	};

	namespace BShkbUtils
	{
		class GraphTraverser
		{
		public:
			GraphTraverser(int a_unkMaybeGraphType, hkbNode* a_rootNode) {
				Ctor(a_unkMaybeGraphType, a_rootNode);
			}

			~GraphTraverser() {
				DtorTree(&BSTBTreePolicy_tree[0]);
				DtorArena(&BSTObjectArena_arena[0]);
			}

			hkbNode* Next()
			{
				using func_t = decltype(&GraphTraverser::Next);
				REL::Relocation<func_t> func{ REL::ID(849404) };
				return func(this);
			}

			uint8_t BSTObjectArena_arena[64];
			uint8_t BSTBTreePolicy_tree[56];

		private:
			void Ctor(int a_unk, hkbNode* a_rootNode) {
				using func_t = decltype(&GraphTraverser::Ctor);
				REL::Relocation<func_t> func{ REL::ID(424303) };
				return func(this, a_unk, a_rootNode);
			}

			static void DtorTree(uint8_t* a_tree) {
				using func_t = decltype(&GraphTraverser::DtorTree);
				REL::Relocation<func_t> func{ REL::ID(16523) };
				return func(a_tree);
			}

			static void DtorArena(uint8_t* a_arena)
			{
				using func_t = decltype(&GraphTraverser::DtorArena);
				REL::Relocation<func_t> func{ REL::ID(1530664) };
				return func(a_arena);
			}
		};
	}

	class BShkbAnimationGraph;

	class BShkbVisitor
	{
	public:
		static constexpr auto VTABLE{ VTABLE::BSAnimGraphVisit__BShkbVisitor };

		BShkbVisitor()
		{
			stl::emplace_vtable(this);
			character = nullptr;
			unk01 = 1;
			unk02 = 0;
		}

		virtual BSVisitControl TraverseAnimationGraphNodes(hkbNode* a_node) {
			using func_t = decltype(&BShkbVisitor::TraverseAnimationGraphNodes);
			REL::Relocation<func_t> func{ REL::ID(1065709) };
			return func(this, a_node);
		}
		virtual BSVisitControl Visit(hkbNode*) = 0;
		virtual BSVisitControl InitVisit() { return RE::BSVisitControl::kContinue; }

		hkbCharacter* character;
		int32_t unk01;
		BSFixedString unk02;
	};

	struct hkbGraphInformation : public BShkbVisitor
	{
		virtual BSVisitControl Visit(hkbNode*) override { return RE::BSVisitControl::kStop; }
	};

	struct hkbContext
	{
	public:

		hkbContext(hkbCharacter* a_char, void* a_physIntfc = nullptr, void* a_attchMngr = nullptr) {
			Ctor(a_char, a_physIntfc, a_attchMngr);
		}

		~hkbContext() {
			Dtor();
		}

		uint8_t unk01[104];

	private:
		void Ctor(hkbCharacter* a_char, void* a_physIntfc = nullptr, void* a_attchMngr = nullptr)
		{
			using func_t = decltype(&hkbContext::Ctor);
			REL::Relocation<func_t> func{ REL::ID(1381136) };
			return func(this, a_char, a_physIntfc, a_attchMngr);
		}

		void Dtor() {
			using func_t = decltype(&hkbContext::Dtor);
			REL::Relocation<func_t> func{ REL::ID(144578) };
			return func(this);
		}
	};

	class BShkbAnimationGraph
	{
	public:
		uint8_t pad00[0x1C8];
		hkbCharacter character;

		void VisitGraph(BShkbVisitor& a_visitor)
		{
			using func_t = decltype(&BShkbAnimationGraph::VisitGraph);
			REL::Relocation<func_t> func{ REL::ID(194777) };
			return func(this, a_visitor);
		}

		hkbNode* getNodeClone(hkbNode* a_node) {
			using func_t = decltype(&BShkbAnimationGraph::getNodeClone);
			REL::Relocation<func_t> func{ REL::ID(326555) };
			return func(this, a_node);
		}
	};
	static_assert(offsetof(BShkbAnimationGraph, BShkbAnimationGraph::character) == 0x1C8);

	struct hkbAnimationBindingWithTriggers
	{
		float unk00[10];
	};

	struct LoadedIdleAnimData
	{
		static BSTArray<LoadedIdleAnimData>* GetArray() {
			static REL::Relocation<BSTArray<LoadedIdleAnimData>*> arr{ REL::ID(762973) };
			return arr.get();
		}

		BSFixedString animFile;
		uint64_t unk2;
		hkbAnimationBindingWithTriggers* binding;
		hkbClipGenerator* clipGenerator;
		BShkbAnimationGraph* animationGraph;
	};
}
