#pragma once

namespace Data
{
	//Simple file memory-store. Only used during data init & immediately flushed afterwards.
	//Combines XMLs into single bin file to greatly lessen I/O overhead for load orders with
	//upwards of 100s of XML files.
	//Returns as invalid if any .xml files in the Data/NAF directory have a newer timestamp
	//than the cache file.
	//Memory footprint is inconsequential as this is only populated during game pre-load,
	//before any game assets have been loaded.
	//Could possibly benefit from zlib compression.
	class XMLCache
	{
	public:
		struct CacheEntry
		{
			std::string filename;
			std::string data;

			template <class Archive>
			void serialize(Archive& ar)
			{
				ar(filename, data);
			}
		};

		struct AnimInfoData
		{
			std::string filename;
			uint32_t loadPriority;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(filename, loadPriority);
			}
		};

		struct Cache
		{
			std::vector<CacheEntry> files;
			std::unordered_map<std::string, AnimInfoData> animDataMap;
			uint64_t nextFaceAnimId = 0;

			template <class Archive>
			void serialize(Archive& ar, const uint32_t)
			{
				ar(files, animDataMap, nextFaceAnimId);
			}
		};

		inline static const std::string cachePath{ USERDATA_DIR + "_XMLCache.bin" };
		inline static safe_mutex lock;
		inline static Cache primaryCache;

		static bool IsCacheValid(const std::vector<std::pair<const std::string, const std::filesystem::file_time_type>>& xmlFiles = {})
		{
			std::unique_lock l{ lock };
			if (cacheValid.has_value())
				return cacheValid.value();

			if (!std::filesystem::exists(cachePath)) {
				cacheValid = false;
				return false;
			}

			auto cacheTime = std::filesystem::last_write_time(cachePath);
			for (auto& pair : xmlFiles) {
				if (pair.second > cacheTime) {
					cacheValid = false;
					return false;
				}
			}

			cacheValid = true;
			return true;
		}

		static bool LoadCache() {
			std::unique_lock l{ lock };
			try {
				zstr::ifstream file(cachePath, std::ios::binary);
				if (file.fail() || !file.good()) {
					logger::warn("Failed to open {}", cachePath);
					cacheValid = false;
					return false;
				}

				cereal::BinaryInputArchive archive(file);
				archive(primaryCache);
			} catch (const std::exception& e) {
				logger::warn("Failed to load cache. Full Message: {}", e.what());
				cacheValid = false;
				primaryCache = Cache();
				return false;
			}

			return true;
		}

		static bool AddFileToCache(const std::string& filename) {
			std::unique_lock l{ lock };
			std::ifstream file(filename);
			if (!file.is_open()) {
				logger::warn("Failed to open {}", cachePath);
				return false;
			}

			primaryCache.files.push_back({
				filename,
				std::move(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()))
			});
			return true;
		}

		static void AddAnimInfoToCache(const std::string& id, const std::string& filename, uint32_t loadPriority)
		{
			std::unique_lock l{ lock };
			auto iter = primaryCache.animDataMap.find(id);
			if (iter != primaryCache.animDataMap.end() && iter->second.loadPriority < loadPriority) {
				iter->second.filename = filename;
			} else if (iter == primaryCache.animDataMap.end()) {
				primaryCache.animDataMap.insert({ id, { filename, loadPriority } });
			}
		}

		static void Flush() {
			std::unique_lock l{ lock };
			if (!IsCacheValid()) {
				try {
					zstr::ofstream file(cachePath, std::ios::binary, Z_BEST_SPEED);
					if (file.fail() || !file.good()) {
						logger::warn("Failed to open {}", cachePath);
						return;
					}
				
					cereal::BinaryOutputArchive archive(file);
					archive(primaryCache);
				} catch (const std::exception& e) {
					logger::warn("Failed to save cache. Full Message: {}", e.what());
				}
			}

			primaryCache = Cache();
			cacheValid = std::nullopt;
		}

		static void Delete() {
			std::unique_lock l{ lock };
			primaryCache = Cache();
			cacheValid = std::nullopt;

			try {
				std::filesystem::remove(cachePath);
			} catch (...) {}
		}

	private:
		inline static std::optional<bool> cacheValid = std::nullopt;
	};
}
