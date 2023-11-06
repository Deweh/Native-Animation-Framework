#pragma once

namespace Scene
{
	using namespace Serialization::General;

	typedef SerializableForm<RE::TESIdleForm> SerializableIdle;
	typedef std::variant<float, SerializableIdle, bool, std::string, uint64_t, Data::ActionSet, std::pair<RE::NiPoint3, float>> ActorPropertyValue;

	enum PropType
	{
		kIdle = 0,
		kScale = 1,
		kSynced = 2,
		kFaceAnim = 3,
		kOrder = 4,
		kStartEquipSet = 5,
		kStopEquipSet = 6,
		kAction = 7,
		kDynIdle = 8,
		kLoopFaceAnim = 9,
		kHadReadyWeapon = 10,
		kWasCommandable = 11,
		kOffset = 12,
		kDynIdleID = 13
	};

	class ActorProperty
	{
	public:
		ActorPropertyValue value = 0.0f;

		template <typename T>
		bool is()
		{
			return std::holds_alternative<T>(value);
		}

		template <typename T>
		bool is(T& out)
		{
			bool res = std::holds_alternative<T>(value);
			if (res) {
				out = get<T>();
			}
			return res;
		}

		template <typename T>
		T get()
		{
			return std::get<T>(value);
		}

		template <class Archive>
		void serialize(Archive& ar, const uint32_t)
		{
			ar(value);
		}
	};

	typedef std::unordered_map<PropType, ActorProperty> ActorPropertyMap;
	typedef std::unordered_map<SerializableActorHandle, ActorPropertyMap> SceneActorsMap;

	template <typename T>
	std::optional<T> GetProperty(const ActorPropertyMap& m, PropType p) {
		auto iter = m.find(p);
		if (iter != m.end() && std::holds_alternative<T>(iter->second.value)) {
			return get<T>(iter->second.value);
		} else {
			return std::nullopt;
		}
	}

	std::vector<RE::NiPointer<RE::Actor>> GetActorsInOrder(const SceneActorsMap& actors)
	{
		std::vector<RE::NiPointer<RE::Actor>> result;
		result.resize(actors.size());
		for (auto& iter : actors) {
			if (auto i = GetProperty<uint64_t>(iter.second, kOrder); i.has_value() && i.value() < result.size()) {
				result[i.value()] = iter.first.get();
			}
		}
		return result;
	}

	std::vector<SerializableActorHandle> GetActorHandlesInOrder(const SceneActorsMap& actors) {
		std::vector<SerializableActorHandle> result;
		result.resize(actors.size());
		for (auto& iter : actors) {
			if (auto i = GetProperty<uint64_t>(iter.second, kOrder); i.has_value() && i.value() < result.size()) {
				result[i.value()] = iter.first;
			}
		}
		return result;
	}
}
