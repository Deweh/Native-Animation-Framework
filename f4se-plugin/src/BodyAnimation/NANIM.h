#pragma once
#include "Misc/JSONUtil.h"

namespace BodyAnimation
{
	class NANIM
	{
	public:
		using jt = JUtil::jt;

		struct Version
		{
			inline static uint32_t currentValue = 1;
			uint32_t value = currentValue;

			bool from_json(const nlohmann::json& j)
			{
				if ((JUtil::ContainsType(j, "version", jt::number_integer) || 
					JUtil::ContainsType(j, "version", jt::number_unsigned))
					&& j["version"] < UINT32_MAX && j["version"] <= currentValue) {

					value = j["version"];
					return true;
				}
				return false;
			}

			void to_json(nlohmann::json& j) const
			{
				j["version"] = value;
			}
		};

		struct AnimationData
		{
			struct MetaData
			{
				struct IKTargetParent
				{
					std::string chain;
					std::string targetAnim;
					std::string targetNode;
				};

				std::map<std::string, std::vector<std::string>> data;

				bool from_json(const nlohmann::json& j)
				{
					for (auto& pair : j.items()) {
						if (JUtil::IsTypedArray(pair.value(), jt::string)) {
							auto& arr = data[pair.key()];
							for (auto& v : pair.value()) {
								arr.push_back(v);
							}
						}
					}
					return true;
				}

				void to_json(nlohmann::json& j) const
				{
					for (const auto& pair : data) {
						auto& arr = j[pair.first];
						for (const auto& v : pair.second) {
							arr.push_back(v);
						}
					}
				}
			};

			struct AnimationKey
			{
				float time = 0.0f;
				RE::NiPoint3 position;
				RE::NiQuaternion rotation;

				bool from_json(const nlohmann::json& j)
				{
					JUtil ju(&j);
					if (!j.is_object() || !ju.ContainsType("time", jt::number_float) ||
						!ju.ContainsTypedArray("rotation", jt::number_float, 4) ||
						!ju.ContainsTypedArray("position", jt::number_float, 3)) {
						return false;
					}
					auto& rot = j["rotation"];
					auto& pos = j["position"];
					time = j["time"];

					if (time < 0) {
						return false;
					}

					position.x = pos[0];
					position.y = pos[1];
					position.z = pos[2];

					rotation.w = rot[0];
					rotation.x = rot[1];
					rotation.y = rot[2];
					rotation.z = rot[3];

					return true;
				}

				void to_json(nlohmann::json& j) const
				{
					j["time"] = time;
					j["position"] = { position.x, position.y, position.z };
					j["rotation"] = { rotation.w, rotation.x, rotation.y, rotation.z };
				}
			};

			using AnimationTimeline = std::vector<AnimationKey>;

			float duration;
			std::map<std::string, AnimationTimeline> timelines;
			MetaData meta;

			bool from_json(const nlohmann::json& j)
			{
				JUtil ju(&j);
				if (!ju.ContainsType("duration", jt::number_float) || !ju.ContainsType("timelines", jt::object)) {
					return false;
				}

				if (ju.ContainsType("metadata", jt::object)) {
					meta.from_json(j["metadata"]);
				}

				duration = j["duration"];
				if (duration < 0.01f) {
					return false;
				}

				for (auto& tl : j["timelines"].items()) {
					if (!tl.value().is_array()) {
						continue;
					}
					auto& targetTL = timelines[tl.key()];
					for (auto& k : tl.value()) {
						AnimationKey curKey;
						if (!curKey.from_json(k)) {
							continue;
						}
						targetTL.push_back(curKey);
					}
				}

				return true;
			}

			void to_json(nlohmann::json& j) const
			{
				if (!meta.data.empty()) {
					meta.to_json(j["metadata"]);
				}
				j["duration"] = duration;
				auto& jTimelines = j["timelines"];
				for (auto& tl : timelines) {
					auto& curTl = jTimelines[tl.first];
					for (auto& k : tl.second) {
						nlohmann::json val;
						k.to_json(val);
						curTl.push_back(val);
					}
				}
			}
		};

		struct AnimationCollection
		{
			std::map<std::string, AnimationData> value;

			bool from_json(const nlohmann::json& j) {
				if (!JUtil::ContainsType(j, "animations", jt::object)) {
					return false;
				}

				for (auto& pair : j["animations"].items()) {
					AnimationData d;
					if (!d.from_json(pair.value())) {
						continue;
					}
					if (value.contains(pair.key())) {
						logger::warn("NANIM contains duplicate animation {}, last one will override others.", pair.key());
					}
					value[pair.key()] = d;
				}
				return true;
			}

			void to_json(nlohmann::json& j) const {
				auto& anims = j["animations"];
				for (auto& pair : value) {
					nlohmann::json element;
					pair.second.to_json(element);
					anims[pair.first] = element;
				}
			}
		};

		bool LoadFromFile(const std::string& fileName) {
			try {
				zstr::ifstream file(fileName, std::ios::binary);

				if (!file.good() || file.fail()) {
					logger::warn("Failed to open {}", fileName);
					return false;
				}

				return LoadFromStream(file);
			} catch (const std::exception&) {
				logger::warn("Failed to open {}", fileName);
				return false;
			}
		}

		bool LoadFromStream(std::istream& s) {
			nlohmann::json data;
			try {
				data = nlohmann::json::from_bson(s);
			} catch (std::exception ex) {
				logger::warn("Failed to load NANIM: {}", ex.what());
				return false;
			}

			return data.is_object() && version.from_json(data) && animations.from_json(data);
		}

		bool SaveToFile(const std::string& fileName) const {
			try {
				zstr::ofstream file(fileName, std::ios::binary, Z_BEST_COMPRESSION);

				if (!file.good() || file.fail()) {
					logger::warn("Failed to open {}", fileName);
					return false;
				}

				return SaveToStream(file);
			} catch (const std::exception&) {
				logger::warn("Failed to open {}", fileName);
				return false;
			}
		}

		bool SaveToStream(std::ostream& s) const {
			nlohmann::json output;
			version.to_json(output);
			animations.to_json(output);

			try {
				nlohmann::json::to_bson(output, s);
			} catch (std::exception ex) {
				logger::warn("Failed to save NANIM: {}", ex.what());
				return false;
			}

			return true;
		}

		bool GetAnimation(const std::string& name, const std::vector<std::string>& graphNodeList, std::unique_ptr<NodeAnimation>& animOut) const
		{
			auto nodeMap = Utility::VectorToIndexMap(graphNodeList);

			if (auto iter = animations.value.find(name); iter != animations.value.end()) {
				auto& data = iter->second;
				animOut = std::make_unique<NodeAnimation>();
				animOut->duration = data.duration;
				animOut->timelines.resize(graphNodeList.size());

				for (auto& pair : data.timelines) {
					auto mapIdx = nodeMap.find(pair.first);
					//If our target graph node list doesn't have any node with the corresponding name, ignore its timeline.
					if (mapIdx == nodeMap.end())
						continue;

					auto& targetTL = animOut->timelines[mapIdx->second];
					for (auto& k : pair.second) {
						auto& targetVal = targetTL.keys[k.time].value;
						targetVal.rotate = k.rotation;
						targetVal.translate = k.position;
					}
				}

				return true;
			}

			return false;
		}

		bool GetAnimationAsPose(const std::string& name, const std::vector<std::string>& graphNodeList, std::vector<NodeTransform>& poseOut) const {
			auto nodeMap = Utility::VectorToIndexMap(graphNodeList);

			if (auto iter = animations.value.find(name); iter != animations.value.end()) {
				auto& data = iter->second;
				poseOut.resize(graphNodeList.size(), NodeTransform::Identity());

				for (auto& pair : data.timelines) {
					auto mapIdx = nodeMap.find(pair.first);
					//If our target graph node list doesn't have any node with the corresponding name, ignore its timeline.
					if (mapIdx == nodeMap.end() || pair.second.size() < 1)
						continue;

					auto& targetVal = poseOut[mapIdx->second];
					auto& targetFrame = pair.second[0];
					targetVal.rotate = targetFrame.rotation;
					targetVal.translate = targetFrame.position;
				}

				return true;
			}

			return false;
		}

		std::vector<std::string> GetAnimationList() const
		{
			std::vector<std::string> result;
			for (auto& pair : animations.value) {
				result.push_back(pair.first);
			}
			return result;
		}

		void SetAnimation(const std::string& name, const std::vector<std::string>& graphNodeList, const NodeAnimation* anim)
		{
			auto& data = animations.value[name];
			data.timelines.clear();
			data.duration = anim->duration;
			for (size_t i = 0; i < anim->timelines.size() && i < graphNodeList.size(); i++) {
				auto& runtimeTL = anim->timelines[i];
				auto& targetTL = data.timelines[graphNodeList[i]];
				for (auto& k : runtimeTL.keys) {
					targetTL.emplace_back(k.first, k.second.value.translate, k.second.value.rotate);
				}
			}
		}

		void SetAnimationFromPose(const std::string& name, float duration, const std::vector<std::string>& graphNodeList, const std::vector<NodeTransform>& pose)
		{
			auto& data = animations.value[name];
			data.timelines.clear();
			data.duration = duration;
			for (size_t i = 0; i < graphNodeList.size() && i < pose.size(); i++) {
				auto& tl = data.timelines[graphNodeList[i]];
				auto& p = pose[i];
				if (p.IsIdentity())
					continue;
				tl.emplace_back(0.00001f, p.translate, p.rotate);
			}
		}

		void SetEmptyAnimation(const std::string& name, float duration) {
			auto& data = animations.value[name];
			data.timelines.clear();
			data.duration = duration;
		}

		bool RemoveAnimation(const std::string& name) {
			if (auto iter = animations.value.find(name); iter != animations.value.end()) {
				animations.value.erase(iter);
				return true;
			}
			return false;
		}

		bool ChangeAnimationName(const std::string& curName, const std::string& newName) {
			if (animations.value.contains(newName)) {
				return false;
			}
			if (auto iter = animations.value.find(curName); iter != animations.value.end()) {
				auto val = animations.value.extract(iter);
				val.key() = newName;
				animations.value.insert(std::move(val));
				return true;
			} else {
				return false;
			}
		}

		std::optional<AnimationData::MetaData> GetMetaData(const std::string& name) const
		{
			if (auto iter = animations.value.find(name); iter != animations.value.end()) {
				return iter->second.meta;
			}
			return std::nullopt;
		}

		bool SetMetaData(const std::string& name, const AnimationData::MetaData& d) {
			if (auto iter = animations.value.find(name); iter != animations.value.end()) {
				iter->second.meta = d;
				return true;
			}
			return false;
		}

		Version version;
		AnimationCollection animations;
	};
}
