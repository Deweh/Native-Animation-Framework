#pragma once
#include "FaceAnimation/AnimationData.h"

namespace Data
{
	class MfgSet : public IdentifiableObject
	{
	public:
		inline static std::atomic<uint64_t> nextFileId = 0;
		std::string fileName;

		static std::optional<std::string> BuildBinary(const FaceAnimation::AnimationData& animData, std::optional<std::string> nameOverride = std::nullopt)
		{
			std::string name;
			if (!nameOverride.has_value()) {
				name = std::format("{}", nextFileId++);
			} else {
				name = nameOverride.value();
			}

			std::ostringstream buffer(std::ios::binary);

			try {
				cereal::BinaryOutputArchive outArchive(buffer);
				outArchive(animData);
			} catch (std::exception ex) {
				logger::warn("Failed to save AnimationData. Full message: {}", ex.what());
				return std::nullopt;
			}

			AnimCache::AddFile(name, buffer.str());

			return name;
		}

		static bool Parse(XMLUtil::Mapper& m, FaceAnim& out, bool buildBinary = true, FaceAnimation::AnimationData* outData = nullptr, FaceAnimation::FrameBasedAnimData* outFrameData = nullptr)
		{
			out.ParseID(m);

			if (!outData && !outFrameData && buildBinary && XMLCache::IsCacheValid()) {
				out.fileName = XMLCache::primaryCache.animDataMap[out.id].filename;
				return m;
			}

			FaceAnimation::FrameBasedAnimData data;
			data.duration = 1;
			m.GetArray([&](XMLUtil::Mapper& m) {
				int32_t frame = 0;

				uint8_t mId = 0;
				m(&mId, (uint8_t)0, false, true, "MfgSet morphID has no 'morphID' attribute!", "morphID");

				int32_t mVal = 0;
				m(&mVal, 0, false, true, "MfgSet intensity has no 'intensity' attribute!", "intensity");
				data.GetTimeline(mId)->keys[frame].value = static_cast<float>(mVal) * 0.01f;
				return m;
			},
				"setting", "MfgSet has no setting!");

			if (outFrameData != nullptr) {
				(*outFrameData) = data;
			}

			if (m && (buildBinary || outData != nullptr)) {
				auto rData = data.ToRuntimeData();

				if (buildBinary) {
					if (XMLCache::IsCacheValid()) {
						out.fileName = XMLCache::primaryCache.animDataMap[out.id].filename;
					} else {
						auto name = BuildBinary(rData);
						if (!name.has_value()) {
							return false;
						} else {
							XMLCache::AddAnimInfoToCache(out.id, name.value(), out.loadPriority);
							out.fileName = name.value();
						}
					}
				}

				if (outData != nullptr) {
					(*outData) = rData;
				}
			}

			return m;
		}
	};
}
