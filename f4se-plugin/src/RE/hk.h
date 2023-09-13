#pragma once

namespace RE
{
	struct hkbGenerateNodesJob;
	struct hkbNodePartitionInfo;
	struct hkbBehaviorGraph;
	struct hkbSpatialQueryInterface;
	struct hkbCharacter;
	struct hkbCharacterSetup;
	struct hkbEventQueue;
	struct hkbGeneratorOutput;
	struct hkbEventPayload;
	struct hkbRagdollInterface;
	struct hkbCharacterData;
	struct hkbSymbolIdMap;
	struct hkbStateMachine;
	struct hkbRagdollDriver;
	struct hkbAssetLoader;
	struct hkbNodeInfo;
	struct hkbStateListener;
	struct hkaFootPlacementIkSolver;
	struct hkbContext;
	struct hkbWorld;
	struct hkbPhysicsInterface;
	struct hkbFootIkDriver;
	struct hkbTransitionEffect;
	struct hkbNodeInternalStateInfo;
	struct hkRefVariant;
	struct hkbVariableValueSet;
	struct hkClassMember;
	struct hkbAnimationBindingWithTriggers;
	struct hkbAnimationBindingSet;
	struct hkbHandIkDriver;
	struct hkbDockingDriver;
	struct hkPseudoRandomGenerator;
	struct hkbBindable;
	struct hkaAnimationBinding;
	struct hkbClipTrigger;
	struct hkbEventInfo;
	struct hkbClipTriggerArray;
	struct hkaDefaultAnimationControl;
	struct hkbBehaviorGraphStringData;
	struct hkbVariableBindingSet;
	struct hkbGeneratorSyncInfo;
	struct hkbReferencedGeneratorSyncInfo;
	struct hkbSharedEventQueue;
	struct hkbSceneModifier;
	struct hkbAttachmentManager;
	struct hkbAttachmentInstance;
	struct hkaDefaultAnimationControlMapperData;
	struct hkbProjectData;
	struct hkaMirroredSkeleton;
	struct hkaSkeleton;
	struct hkLocalFrame;
	struct hkbCharacterControllerDriver;
	struct hkbProjectAssetManager;
	struct hkaAnimation;
	struct hkbBehaviorGraphData;
	struct hkbCondition;
	struct hkbFootIkDriverInfo;
	struct hkbMirroredSkeletonInfo;
	struct hkbRagdollController;
	struct hkaBone;
	struct hkbCharacterController;
	struct hkbDockingTarget;
	struct hkbAssetBundleStringData;
	struct hkaAnnotationTrack;
	struct hkbCustomIdSelector;
	struct hkbAttachmentSetup;
	struct hkbGeneratorOutputListener;
	struct hkaAnimatedReferenceFrame;
	struct hkaSkeletonMapper;
	struct hkClassEnum;
	struct hkaBoneAttachment;
	struct hkbProjectStringData;
	struct hkbHandIkDriverInfo;
	struct hkbAssetBundle;
	struct hkbCharacterStringData;
	struct hkbVariableBindingSet;

	struct hkClassMember
	{
		enum Type : uint8_t
		{
			TYPE_VOID = 0x0,
			TYPE_BOOL = 0x1,
			TYPE_CHAR = 0x2,
			TYPE_INT8 = 0x3,
			TYPE_UINT8 = 0x4,
			TYPE_INT16 = 0x5,
			TYPE_UINT16 = 0x6,
			TYPE_INT32 = 0x7,
			TYPE_UINT32 = 0x8,
			TYPE_INT64 = 0x9,
			TYPE_UINT64 = 0xA,
			TYPE_REAL = 0xB,
			TYPE_VECTOR4 = 0xC,
			TYPE_QUATERNION = 0xD,
			TYPE_MATRIX3 = 0xE,
			TYPE_ROTATION = 0xF,
			TYPE_QSTRANSFORM = 0x10,
			TYPE_MATRIX4 = 0x11,
			TYPE_TRANSFORM = 0x12,
			TYPE_ZERO = 0x13,
			TYPE_POINTER = 0x14,
			TYPE_FUNCTIONPOINTER = 0x15,
			TYPE_ARRAY = 0x16,
			TYPE_INPLACEARRAY = 0x17,
			TYPE_ENUM = 0x18,
			TYPE_STRUCT = 0x19,
			TYPE_SIMPLEARRAY = 0x1A,
			TYPE_HOMOGENEOUSARRAY = 0x1B,
			TYPE_VARIANT = 0x1C,
			TYPE_CSTRING = 0x1D,
			TYPE_ULONG = 0x1E,
			TYPE_FLAGS = 0x1F,
			TYPE_HALF = 0x20,
			TYPE_STRINGPTR = 0x21,
			TYPE_RELARRAY = 0x22,
			TYPE_MAX = 0x23
		};
	};

	struct hkStringPtr
	{
		const char* stringAndFlag;
	};

	struct hkbVariableBindingSet : public hkReferencedObject
	{
		struct Binding
		{
			enum BindingType : int8_t
			{
				BINDING_TYPE_VARIABLE = 0x0,
				BINDING_TYPE_CHARACTER_PROPERTY = 0x1
			};

			enum InternalBindingFlags : int8_t
			{
			};

			hkStringPtr memberPath;
			const hkClass* memberClass;
			int32_t offsetInObjectPlusOne;
			int32_t offsetInArrayPlusOne;
			int32_t rootVariableIndex;
			int32_t variableIndex;
			int8_t bitIndex;
			hkEnum<BindingType, int8_t> bindingType;
			hkEnum<hkClassMember::Type, uint8_t> memberType;
			int8_t variableType;
			hkFlags<InternalBindingFlags, int8_t> flags;
		};

		hkArray<Binding, hkContainerHeapAllocator> bindings;
		int32_t indexOfBindingToEnable;
		hkBool hasOutputBinding;
		hkBool initializedOffsets;
	};

	struct hkbBindable : public hkReferencedObject
	{
		hkRefPtr<hkbVariableBindingSet> variableBindingSet;
		hkArray<hkbBindable*, hkContainerHeapAllocator> cachedBindables;
		hkBool areBindablesCached;
		hkBool hasEnableChanged;
	};

	enum hkbNodeType : uint8_t
	{
		HKB_NODE_TYPE_INVALID = 0x0,
		HKB_NODE_TYPE_FIRST_GENERATOR = 0x1,
		HKB_NODE_TYPE_BEHAVIOR_GRAPH = 0x1,
		HKB_NODE_TYPE_BEHAVIOR_REFERENCE_GENERATOR = 0x2,
		HKB_NODE_TYPE_BLENDER_GENERATOR = 0x3,
		HKB_NODE_TYPE_CLIP_GENERATOR = 0x4,
		HKB_NODE_TYPE_MANUAL_SELECTOR_GENERATOR = 0x5,
		HKB_NODE_TYPE_MODIFIER_GENERATOR = 0x6,
		HKB_NODE_TYPE_REFERENCE_POSE_GENERATOR = 0x7,
		HKB_NODE_TYPE_STATE_MACHINE = 0x8,
		HKB_NODE_TYPE_SCRIPT_GENERATOR = 0x9,
		HKB_NODE_TYPE_LAYER_GENERATOR = 0xA,
		HKB_NODE_TYPE_END_OF_SPU_GENERATORS = 0xB,
		HKB_NODE_TYPE_DOCKING_GENERATOR = 0xB,
		HKB_NODE_TYPE_PARAMETRIC_MOTION_GENERATOR = 0xC,
		HKB_NODE_TYPE_PIN_BONE_GENERATOR = 0xD,
		HKB_NODE_TYPE_OTHER_GENERATOR = 0xE,
		HKB_NODE_TYPE_FIRST_TRANSITION_EFFECT = 0x30,
		HKB_NODE_TYPE_BLENDING_TRANSITION_EFFECT = 0x30,
		HKB_NODE_TYPE_GENERATOR_TRANSITION_EFFECT = 0x31,
		HKB_NODE_TYPE_END_OF_SPU_TRANSITION_EFFECTS = 0x32,
		HKB_NODE_TYPE_MANUAL_SELECTOR_TRANSITION_EFFECT = 0x32,
		HKB_NODE_TYPE_FIRST_MODIFIER = 0x40,
		HKB_NODE_TYPE_ATTACHMENT_MODIFIER = 0x40,
		HKB_NODE_TYPE_ATTRIBUTE_MODIFIER = 0x41,
		HKB_NODE_TYPE_CHARACTER_CONTROLLER_MODIFIER = 0x42,
		HKB_NODE_TYPE_COMBINE_TRANSFORMS_MODIFIER = 0x43,
		HKB_NODE_TYPE_COMPUTE_DIRECTION_MODIFIER = 0x44,
		HKB_NODE_TYPE_COMPUTE_ROTATION_FROM_AXIS_ANGLE_MODIFIER = 0x45,
		HKB_NODE_TYPE_COMPUTE_ROTATION_TO_TARGET_MODIFIER = 0x46,
		HKB_NODE_TYPE_DAMPING_MODIFIER = 0x47,
		HKB_NODE_TYPE_DELAYED_MODIFIER = 0x48,
		HKB_NODE_TYPE_EVALUATE_EXPRESSION_MODIFIER = 0x49,
		HKB_NODE_TYPE_EVENTS_FROM_RANGE_MODIFIER = 0x4A,
		HKB_NODE_TYPE_EVENT_DRIVEN_MODIFIER = 0x4B,
		HKB_NODE_TYPE_FOOT_IK_CONTROLS_MODIFIER = 0x4C,
		HKB_NODE_TYPE_GET_WORLD_FROM_MODEL_MODIFIER = 0x4D,
		HKB_NODE_TYPE_HAND_IK_CONTROLS_MODIFIER = 0x4E,
		HKB_NODE_TYPE_KEYFRAME_BONES_MODIFIER = 0x4F,
		HKB_NODE_TYPE_LOOK_AT_MODIFIER = 0x50,
		HKB_NODE_TYPE_MIRROR_MODIFIER = 0x51,
		HKB_NODE_TYPE_MODIFIER_LIST = 0x52,
		HKB_NODE_TYPE_MOVE_BONE_ATTACHMENT_MODIFIER = 0x53,
		HKB_NODE_TYPE_MOVE_CHARACTER_MODIFIER = 0x54,
		HKB_NODE_TYPE_POWERED_RAGDOLL_CONTROLS_MODIFIER = 0x55,
		HKB_NODE_TYPE_RIGID_BODY_RAGDOLL_CONTROLS_MODIFIER = 0x56,
		HKB_NODE_TYPE_ROTATE_CHARACTER_MODIFIER = 0x57,
		HKB_NODE_TYPE_SET_WORLD_FROM_MODEL_MODIFIER = 0x58,
		HKB_NODE_TYPE_TIMER_MODIFIER = 0x59,
		HKB_NODE_TYPE_TRANSFORM_VECTOR_MODIFIER = 0x5A,
		HKB_NODE_TYPE_TWIST_MODIFIER = 0x5B,
		HKB_NODE_TYPE_END_OF_SPU_MODIFIERS = 0x5C,
		HKB_NODE_TYPE_DETECT_CLOSE_TO_GROUND_MODIFIER = 0x5C,
		HKB_NODE_TYPE_EVALUATE_HANDLE_MODIFIER = 0x5D,
		HKB_NODE_TYPE_GET_HANDLE_ON_BONE_MODIFIER = 0x5E,
		HKB_NODE_TYPE_GET_UP_MODIFIER = 0x5F,
		HKB_NODE_TYPE_JIGGLER_MODIFIER = 0x60,
		HKB_NODE_TYPE_SENSE_HANDLE_MODIFIER = 0x61,
		HKB_NODE_TYPE_SEQUENCE = 0x62,
		HKB_NODE_TYPE_AI_STEERING_MODIFIER = 0x63,
		HKB_NODE_TYPE_AI_CONTROL_MODIFIER = 0x64,
		HKB_NODE_TYPE_ROCKETBOX_CHARACTER_CONTROLLER_MODIFIER = 0x65,
		HKB_NODE_TYPE_LEAN_ROCKETBOX_CHARACTER_CONTROLLER_MODIFIER = 0x66,
		HKB_NODE_TYPE_OTHER_MODIFIER = 0x67
	};

	struct hkbNode : public hkbBindable
	{
		enum CloneState : int8_t
		{
			CLONE_STATE_DEFAULT = 0x0,
			CLONE_STATE_TEMPLATE = 0x1,
			CLONE_STATE_CLONE = 0x2
		};

		uint32_t userData;
		hkStringPtr name;
		uint16_t id;
		hkEnum<CloneState, int8_t> cloneState;
		hkEnum<hkbNodeType, uint8_t> type;
		hkbNodeInfo* nodeInfo;
	};

	struct hkbGeneratorPartitionInfo
	{
		uint32_t boneMask[8];
		uint32_t partitionMask[1];
		int16_t numBones;
		int16_t numMaxPartitions;
	};

	struct hkbGeneratorSyncInfo
	{
		uint8_t syncPoints[0x80];
		float duration;
		float localTime;
		float playbackSpeed;
		int8_t numSyncPoints;
		bool isCyclic;
		bool isMirrored;
		bool isAdditive;
		uint8_t activeInterval[0x14];
	};
	static_assert(sizeof(hkbGeneratorSyncInfo) == 0xA4);

	struct hkbGenerator : public hkbNode
	{
		hkbGeneratorPartitionInfo partitionInfo;
		hkbGeneratorSyncInfo* syncInfo;
		int8_t pad[4];
	};

	struct hkbEventBase
	{
		int32_t id;
		hkbEventPayload* payload;
	};

	struct hkbEventProperty : public hkbEventBase
	{};

	struct hkbClipTrigger
	{
		float localTime;
		hkbEventProperty event;
		hkBool relativeToEndOfClip;
		hkBool acyclic;
		hkBool isAnnotation;
	};

	struct hkbClipTriggerArray : public hkReferencedObject
	{
		hkArray<hkbClipTrigger, hkContainerHeapAllocator> triggers;
	};

	struct hkbClipGenerator : public hkbGenerator
	{
		hkStringPtr animationBundleName;
		hkStringPtr animationName;
		hkRefPtr<hkbClipTriggerArray> triggers;
		uint32_t userPartitionMask;
		float cropStartAmountLocalTime;
		float cropEndAmountLocalTime;
		float startTime;
		float playbackSpeed;
		float enforcedDuration;
		float userControlledTimeFraction;
	};

	struct __declspec(novtable) hkbCharacter : public hkReferencedObject
	{
		virtual void getNearbyCharacters(float, hkArray<hkbCharacter*, hkContainerHeapAllocator>*) {}

		void* unk10;
		hkArray<hkbCharacter*, hkContainerHeapAllocator> nearbyCharacters;
		uint32_t userData;
		int16_t currentLod;
		int16_t numTracksInLod;
		hkbGeneratorOutput* generatorOutput;
		hkStringPtr name;
		hkRefPtr<hkbRagdollDriver> ragdollDriver;
		hkRefPtr<hkbRagdollInterface> ragdollInterface;
		hkRefPtr<hkbCharacterControllerDriver> characterControllerDriver;
		hkRefPtr<hkbFootIkDriver> footIkDriver;
		hkRefPtr<hkbHandIkDriver> handIkDriver;
		hkRefPtr<hkbDockingDriver> dockingDriver;
		hkRefPtr<hkReferencedObject> aiControlDriver;
		hkRefPtr<hkbCharacterSetup> setup;
		hkRefPtr<hkbBehaviorGraph> behaviorGraph;
		hkRefPtr<hkbProjectData> projectData;
		hkRefPtr<hkbAnimationBindingSet> animationBindingSet;
		hkbSpatialQueryInterface* spatialQueryInterface;
		hkbWorld* world;
		hkArray<hkaBoneAttachment*, hkContainerHeapAllocator> boneAttachments;
		hkbEventQueue* m_eventQueue;
		void* characterLuaState;
		hkbProjectAssetManager* assetManager;
		int32_t capabilities;
		int32_t effectiveCapabilities;
	};
	static_assert( offsetof(hkbCharacter, hkbCharacter::behaviorGraph) == 0x80 );

	struct hkPointerMap
	{
		void* elements;
		int32_t numElements;
		int32_t hashMod;
	};

	struct hkbBehaviorGraph : public hkbGenerator
	{
		enum VariableMode : int8_t
		{
		};

		struct GlobalTransitionData;

		hkEnum<VariableMode, int8_t> variableMode;
		hkArray<uint16_t, hkContainerHeapAllocator> uniqueIdPool;
		hkPointerMap* idToStateMachineTemplateMap;
		hkArray<int32_t, hkContainerHeapAllocator> mirroredExternalIdMap;
		hkPseudoRandomGenerator* pseudoRandomGenerator;
		hkRefPtr<hkbGenerator> rootGenerator;
		hkRefPtr<hkbBehaviorGraphData> data;
		hkRefPtr<hkbBehaviorGraph> _template;
		hkbGenerator* rootGeneratorClone;
		hkArray<hkbNodeInfo*, hkContainerHeapAllocator>* activeNodes;
		hkRefPtr<hkbBehaviorGraph::GlobalTransitionData> globalTransitionData;
		hkRefPtr<hkbSymbolIdMap> eventIdMap;
		hkRefPtr<hkbSymbolIdMap> attributeIdMap;
		hkRefPtr<hkbSymbolIdMap> variableIdMap;
		hkRefPtr<hkbSymbolIdMap> characterPropertyIdMap;
		hkbVariableValueSet* variableValueSet;
		hkPointerMap* nodeTemplateToCloneMap;
		hkPointerMap* stateListenerTemplateToCloneMap;
		hkArray<hkbNode*, hkContainerHeapAllocator> recentlyCreatedClones;
		hkbNodePartitionInfo* nodePartitionInfo;
		int32_t numIntermediateOutputs;
		hkArray<int16_t, hkContainerHeapAllocator> intermediateOutputSizes;
		hkArray<hkbGenerateNodesJob*, hkContainerHeapAllocator> jobs;
		hkArray<void*, hkContainerHeapAllocator> allPartitionMemory;
		hkArray<int32_t, hkContainerHeapAllocator> internalToRootVariableIdMap;
		hkArray<int32_t, hkContainerHeapAllocator> internalToCharacterCharacterPropertyIdMap;
		hkArray<int32_t, hkContainerHeapAllocator> internalToRootAttributeIdMap;
		uint16_t nextUniqueId;
		hkBool isActive;
		hkBool isLinked;
		hkBool updateActiveNodes;
		hkBool updateActiveNodesForEnable;
		hkBool checkNodeValidity;
		hkBool stateOrTransitionChanged;

		void setActiveGeneratorLocalTime(const hkbContext* a_context, hkbGenerator* a_gen, float a_time)
		{
			using func_t = decltype(&hkbBehaviorGraph::setActiveGeneratorLocalTime);
			REL::Relocation<func_t> func{ REL::ID(992878) };
			return func(this, a_context, a_gen, a_time);
		}

	    hkbNode* getNodeClone(hkbNode* a_node)
		{
			using func_t = decltype(&hkbBehaviorGraph::getNodeClone);
			REL::Relocation<func_t> func{ REL::ID(326555) };
			return func(this, a_node);
		}
	};
	static_assert(offsetof(hkbBehaviorGraph, hkbBehaviorGraph::rootGenerator) == 0xC0);
}
