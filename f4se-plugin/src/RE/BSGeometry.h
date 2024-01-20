#pragma once

namespace RE
{
	class BSSkin_Instance;

	class BSGeometryData
	{
	public:
		uint64_t vertexDesc;

		struct VertexData
		{
			ID3D11Buffer* d3d11Buffer;  // 00
			uint8_t* vertexBlock;         // 08
			uint64_t unk10;             // 10
			uint64_t unk18;               // 18
			uint64_t unk20;             // 20
			uint64_t unk28;               // 28
			uint64_t unk30;             // 30
			volatile int32_t refCount;   // 38
		};

		struct TriangleData
		{
			ID3D11Buffer* d3d11Buffer;  // 00
			uint16_t* triangles;          // 08
			uint64_t unk10;               // 10
			uint64_t unk18;               // 18
			uint64_t unk20;               // 20
			uint64_t unk28;               // 28
			uint64_t unk30;               // 30
			volatile int32_t refCount;   // 38
		};

		VertexData* vertexData;      // 08
		TriangleData* triangleData;  // 10
		volatile int32_t refCount;    // 18
	};

	class BSGeometry : public NiAVObject
	{
	public:
		virtual void Unk_39();
		virtual void Unk_3A();
		virtual void Unk_3B();
		virtual void Unk_3C();
		virtual void Unk_3D();
		virtual void Unk_3E();
		virtual void Unk_3F();
		virtual void Unk_40();

		NiBound kModelBound;                       // 120
		NiPointer<NiProperty> effectState;         // 130
		NiPointer<NiProperty> shaderProperty;      // 138
		NiPointer<BSSkin_Instance> skinInstance;   // 140

		union VertexDesc
		{
			/*
			struct
			{
				uint8_t szVertexData: 4;
				uint8_t szVertex: 4;
				uint8_t oTexCoord0: 4;
				uint8_t oTexCoord1: 4;
				uint8_t oNormal: 4;
				uint8_t oTangent: 4;
				uint8_t oColor: 4;
				uint8_t oSkinningData: 4;
				uint8_t oLandscapeData: 4;
				uint8_t oEyeData: 4;
				uint16_t vertexFlags: 16;
				uint8_t unused: 8;
			};
			*/
			uint64_t vertexDesc;
		};

		enum : uint64_t
		{
			kFlag_Unk1 = (1ULL << 40),
			kFlag_Unk2 = (1ULL << 41),
			kFlag_Unk3 = (1ULL << 42),
			kFlag_Unk4 = (1ULL << 43),
			kFlag_Vertex = (1ULL << 44),
			kFlag_UVs = (1ULL << 45),
			kFlag_Unk5 = (1ULL << 46),
			kFlag_Normals = (1ULL << 47),
			kFlag_Tangents = (1ULL << 48),
			kFlag_VertexColors = (1ULL << 49),
			kFlag_Skinned = (1ULL << 50),
			kFlag_Unk6 = (1ULL << 51),
			kFlag_MaleEyes = (1ULL << 52),
			kFlag_Unk7 = (1ULL << 53),
			kFlag_FullPrecision = (1ULL << 54),
			kFlag_Unk8 = (1ULL << 55),
		};

		BSGeometryData* geometryData;  // 148
		uint64_t vertexDesc;           // 150

		uint16_t GetVertexSize() const { return (vertexDesc << 2) & 0x3C; }

		int8_t ucType;     // 158
		bool Registered;  // 159
		uint16_t pad15A;    // 15A
		uint32_t unk15C;    // 15C

		RE::BSShaderProperty* QShaderProperty()
		{
			using func_t = decltype(&BSGeometry::QShaderProperty);
			REL::Relocation<func_t> func{ REL::ID(1286348) };
			return func(this);
		}
	};
	static_assert(sizeof(BSGeometry) == 0x160);

	class BSTriShape : public BSGeometry
	{
	public:
		uint32_t numTriangles;  // 160
		uint16_t numVertices;   // 164
		uint16_t unk166;        // 166
		float unk168;         // 168
		float unk16C;         // 16C
	};
	static_assert(sizeof(BSTriShape) == 0x170);

	class BSShaderMaterial
	{
	public:
		virtual ~BSShaderMaterial(){};

		virtual BSShaderMaterial* Create() { return nullptr; };
		virtual void CopyMembers(const BSShaderMaterial*) { return; };
		virtual void ComputeCRC32(uint32_t, bool) { return; };
		virtual BSShaderMaterial* GetDefault() { return nullptr; };
		virtual uint32_t GetType() { return 0; };
		virtual uint32_t GetFeature() { return 0; };
		virtual void Compare(const BSShaderMaterial&) { return; };
		virtual void IsCopy(const BSShaderMaterial*) { return; };

		volatile uint32_t m_refCount;   // 08
		NiPoint2 textCoordOffset[2];  // 0C
		NiPoint2 textCoordScale[2];   // 1C
		int32_t unk2C;                 // 2C
		uint32_t uiHashKey;             // 30
		uint32_t uiUniqueCode;          // 34

		void SetOffsetUV(float u, float v)
		{
			textCoordOffset[0].x = u;
			textCoordOffset[1].x = u;
			textCoordOffset[0].y = v;
			textCoordOffset[1].y = v;
		}
	};

	class BSLightingShaderProperty : public BSShaderProperty
	{
	public:
		NiColorA projectedUVColor;                 // 70
		NiColorA projectedUVParams;                // 80
		BSRenderPass* depthMapRenderPassListA[3];  // 90
		BSRenderPass* depthRenderPass;             // A8
		BSRenderPass* forwardPassList;             // B0
		NiColor* emissiveColor;                    // B8
		BSFixedString rootName;                     // C0
		float emitColorScale;                      // C8
		float forcedDarkness;                      // CC
		float specularLODFade;                     // D0
		float envmapLODFade;                       // D4
		uint32_t uiBaseTechniqueID;                   // D8
		uint32_t clearCommandBufferCount;          // DC
		uint16_t usDebugTintIndex;                    // E0
		uint16_t uiStencilRef;                     // E2
		uint32_t unkE4;                             // E4

	};
	static_assert(sizeof(BSLightingShaderProperty) == 0xE8);
}
