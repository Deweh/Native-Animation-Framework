#pragma once

namespace RE
{
	class hkbContext;

	enum hkbNodeType : uint8_t
	{
		kClipGenerator = 4
	};

	class hkbSyncInfo
	{
	public:
		uint8_t unk00[128];
		float f3;
		float f1;
		uint8_t unk01[28];
		float f2;
	};

	class hkbNode
	{
	public:
		uint8_t unk00[15];
		hkbSyncInfo* syncInfo;
	};

	enum BSVisitControl : uint32_t
	{
		kContinue = 0,
		kStop = 1
	};

	class hkbClipGenerator : public hkbNode
	{
	public:
		void setLocalTime(const hkbContext* a_context, float a_time)
		{
			using func_t = decltype(&hkbClipGenerator::setLocalTime);
			REL::Relocation<func_t> func{ REL::ID(1447515) };
			return func(this, a_context, a_time);
		}

		float getLocalLocalTime()
		{
			using func_t = decltype(&hkbClipGenerator::getLocalLocalTime);
			REL::Relocation<func_t> func{ REL::ID(564058) };
			return func(this);
		}
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
	
	class hkbCharacter
	{
	public:
		std::byte unk01[128];
		BShkbAnimationGraph* graph;
	};

	class __declspec(novtable) BShkbVisitor
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

		virtual BSVisitControl TraverseAnimationGraphNodes(hkbNode*) {}
		virtual BSVisitControl Visit(hkbNode*) = 0;

		hkbCharacter* character;
		int32_t unk01;
		BSFixedString unk02;
	};

	class hkbContext
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
		uint8_t unk01[192];
		hkbNode* targetNode;
		uint8_t unk00[256];
		hkbCharacter* character;

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

		void setActiveGeneratorLocalTime(const hkbContext* a_context, hkbClipGenerator* a_gen, float a_time) {
			using func_t = decltype(&BShkbAnimationGraph::setActiveGeneratorLocalTime);
			REL::Relocation<func_t> func{ REL::ID(992878) };
			return func(this, a_context, a_gen, a_time);
		}
	};

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

	struct BSAnimationGraphVariableCache
	{
		uint64_t unk1;
		uint64_t unk2;
		uint64_t unk3;
		uint64_t unk4;
		uint64_t unk5;
		uint64_t unk6;
		uint64_t unk7;
		BShkbAnimationGraph* animGraph;
	};
}
