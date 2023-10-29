#pragma once

class JUtil
{
public:
	using jt = nlohmann::json::value_t;

	inline static bool IsTypedArray(const nlohmann::json& j, jt ty, std::optional<size_t> size = std::nullopt)
	{
		if (!j.is_array() || (size.has_value() && j.size() != size.value())) {
			return false;
		}

		for (auto& n : j) {
			if (n.type() != ty) {
				return false;
			}
		}
		return true;
	}

	inline static bool ContainsType(const nlohmann::json& j, const std::string_view& name, jt ty)
	{
		return (j.contains(name) && j[name].type() == ty);
	}

	inline static bool ContainsTypedArray(const nlohmann::json& j, const std::string_view& name, jt ty, std::optional<size_t> size = std::nullopt) {
		return ContainsType(j, name, jt::array) && IsTypedArray(j[name], ty, size);
	}

	JUtil(const nlohmann::json* j) {
		localJ = j;
	}

	const nlohmann::json* localJ = nullptr;

	bool IsTypedArray(jt ty, std::optional<size_t> size = std::nullopt) {
		return IsTypedArray(*localJ, ty, size);
	}

	bool ContainsType(const std::string_view& name, jt ty) {
		return ContainsType(*localJ, name, ty);
	}

	bool ContainsTypedArray(const std::string_view& name, jt ty, std::optional<size_t> size = std::nullopt) {
		return ContainsTypedArray(*localJ, name, ty, size);
	}
};
