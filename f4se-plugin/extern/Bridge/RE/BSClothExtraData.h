//#pragma once
//
//#include <RE/NetImmerse/NiExtraData.h>
//#include <RE/NetImmerse/NiObject.h>
//#include <RE/NiRTTI_IDs.h>
//#include <RE/RTTI.h>
//#include <RE/RTTI_IDs.h>
//#include <RE/VTABLE_IDs.h>
//
//namespace RE
//{
//	class __declspec(novtable) BSClothExtraData :
//		public NiExtraData  // Корректная иерархия: NiExtraData → NiObject → NiRefObject
//	{
//	public:
//		// RTTI и VTABLE идентификаторы
//		static constexpr auto RTTI{ RTTI::BSClothExtraData };
//		static constexpr auto VTABLE{ VTABLE::BSClothExtraData };
//		static constexpr auto Ni_RTTI{ Ni_RTTI::BSClothExtraData };
//
//		//===============================================================
//		// Настройки ткани (Cloth Settings)
//		//===============================================================
//		static SettingT<bool> bLoadLocalSetupData;                   // 0
//		static SettingT<bool> bDrawClothDeformedBones;               // 0x18
//		static SettingT<bool> bDrawParticleStepPositions;            // 0x30
//		static SettingT<bool> bAnimClothLOD;                         // 0x88
//		static SettingT<bool> bEnablePrediction;                     // 0xA0
//		static SettingT<bool> bEnableScaling;                        // 0xB8
//		static SettingT<float> fMaxRootDistanceBeforeTeleport;       // 0xD0
//		static SettingT<float> fMaxRootAngleBeforeTeleport;          // 0xE8
//		static SettingT<float> fMaxBoneSpeedBeforeDefaultStiffness;  // 0x100
//
//		//===============================================================
//		// Строковые константы для инициализации настроек
//		//===============================================================
//		struct Data
//		{
//			const char* bLoadLocalSetupDataStr = "bLoadLocalSetupData:Cloth";                // 0x40
//			const char* bDrawClothDeformedBonesStr = "bDrawClothDeformedBones:Cloth";        // 0x58
//			const char* bDrawParticleStepPositionsStr = "bDrawParticleStepPositions:Cloth";  // 0x70
//			const char* bAnimClothLODStr = "bAnimClothLOD:Cloth";                            // 0x88
//			const char* bEnablePredictionStr = "bEnablePrediction:Cloth";                    // 0xA0
//			const char* bEnableScalingStr = "bEnableScaling:Cloth";                          // 0xB8
//			const char* fMaxRootDistanceStr = "fMaxRootDistanceBeforeTeleport:Cloth";        // 0xD0
//			const char* fMaxRootAngleStr = "fMaxRootAngleBeforeTeleport:Cloth";              // 0xE8
//			const char* fMaxBoneSpeedStr = "fMaxBoneSpeedBeforeDefaultStiffness:Cloth";      // 0x100
//		};
//		static Data strings;
//
//		//===============================================================
//		// Виртуальные методы NiExtraData
//		//===============================================================
//		virtual const char* GetName() const override
//		{
//			return "BSClothExtraData";  // Тип имени из RTTI (.?AVBSClothExtraData@@)
//		}
//
//		// Методы
//
//	};
//}
