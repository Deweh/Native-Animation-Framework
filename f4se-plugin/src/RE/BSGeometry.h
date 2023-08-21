#pragma once

namespace RE
{
	class BSGeometry : public NiAVObject
	{
	public:
		RE::BSShaderProperty* QShaderProperty()
		{
			using func_t = decltype(&BSGeometry::QShaderProperty);
			REL::Relocation<func_t> func{ REL::ID(1286348) };
			return func(this);
		}
	};

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
}
