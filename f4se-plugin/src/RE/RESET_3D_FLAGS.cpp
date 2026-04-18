#include <RE/Bethesda/TESObjectREFRs.h>

namespace RE
{
	constexpr RESET_3D_FLAGS operator|(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs)
	{
		return static_cast<RESET_3D_FLAGS>(
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(lhs) |
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(rhs));
	}

	constexpr RESET_3D_FLAGS operator&(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs)
	{
		return static_cast<RESET_3D_FLAGS>(
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(lhs) &
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(rhs));
	}

	constexpr RESET_3D_FLAGS operator^(RESET_3D_FLAGS lhs, RESET_3D_FLAGS rhs)
	{
		return static_cast<RESET_3D_FLAGS>(
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(lhs) ^
			static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(rhs));
	}

	constexpr RESET_3D_FLAGS operator~(RESET_3D_FLAGS flags)
	{
		return static_cast<RESET_3D_FLAGS>(
			~static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(flags));
	}

	RESET_3D_FLAGS& operator|=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	RESET_3D_FLAGS& operator&=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	RESET_3D_FLAGS& operator^=(RESET_3D_FLAGS& lhs, RESET_3D_FLAGS rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	constexpr bool HasFlag(RESET_3D_FLAGS value, RESET_3D_FLAGS flag)
	{
		return (static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(value) &
				   static_cast<std::underlying_type_t<RESET_3D_FLAGS>>(flag)) != 0;
	}
}
