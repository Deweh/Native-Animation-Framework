#pragma once

namespace FaceAnimation
{
	namespace FaceUpdateHook
	{
		extern bool LoadAndPlayAnimationNoCheck(RE::ActorHandle targetActor, std::string id, bool loop, bool havokSync);
		extern void StopAnimation(RE::ActorHandle targetActor, bool animOverride);
	}
}

namespace Data
{
	class FaceAnim : public IdentifiableObject
	{
	public:
		inline static std::atomic<uint64_t> nextFileId = 0;
		std::string fileName;

		static std::optional<std::string> BuildBinary(const FaceAnimation::AnimationData& animData, std::optional<std::string> nameOverride = std::nullopt) {
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
			m.GetMinMax(&data.duration, 0, true, true, "FaceAnim 'frames' attribute must be between 1 and 2,000,000.", 1, 2000000, "frames");
			m.GetArray([&](XMLUtil::Mapper& m) {
				int32_t frame = 0;
				m.GetMinMax(&frame, 0, false, true, "FaceAnim key 'frame' attribute must be between 0 and 2,000,000.", 0, 2000000, "frame");
				m.GetArray([&](XMLUtil::Mapper& m) {
					auto n = m.GetCurrentName();
					if (n == "morph") {
						uint8_t mId = 0;
						m(&mId, (uint8_t)0, false, true, "FaceAnim morph has no 'id' attribute!", "id");
						int32_t mVal = 0;
						m(&mVal, 0, false, true, "FaceAnim morph has no 'value' attribute!", "value");
						data.GetTimeline(mId)->keys[frame].value = static_cast<float>(mVal) * 0.01f;
					} else if (n == "eyes") {
						FaceAnimation::EyeVector v;
						m(&v.u, 0.0, false, true, "FaceAnim eyes node has no 'x' attribute!", "x");
						m(&v.v, 0.0, false, true, "FaceAnim eyes node has no 'y' attribute!", "y");
						v.ConvertRange(true);
						data.GetTimeline(0, true, true)->keys[frame].eyesValue = v;
					}
					return m;
				});
				return m;
			},
			"key", "FaceAnim has no keyframes!");

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

		//NAF Bridge mfgSet
		bool Play(RE::ActorHandle a) const
		{
			return FaceAnimation::FaceUpdateHook::LoadAndPlayAnimationNoCheck(a, id, true, false);
		}

		//NAF Bridge mfgSet
		void Stop(RE::ActorHandle a) const
		{
			FaceAnimation::FaceUpdateHook::StopAnimation(a, false);
		}
	};
}
