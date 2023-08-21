#pragma once

namespace Data
{
	//Simple animation cache, a very lightweight archiving system for caching animation data.
	//Designed to reduce working memory footprint of potentially large animation objects,
	//and also reduce I/O overhead of storing each animation object as a separate file.
	//Does not support removing/editing files without rebuilding.
	//File name & file size are stored before each file.
	//Could possibly benefit from zlib compression.
	class AnimCache
	{
	public:
		enum Mode
		{
			kRead,
			kWrite,
			kOverwrite
		};
		
		struct FileTableEntry
		{
			uint64_t offset;
			uint64_t size;
		};

		inline static const std::string cachePath{ USERDATA_DIR + "_AnimCache.bin" };
		
		static bool IsOpen() {
			std::unique_lock l{ lock };
			return fileHandle.is_open();
		}

		static bool IsLoaded() {
			std::unique_lock l{ lock };
			return loaded;
		}

		static bool SetMode(Mode a_newMode, bool reset = false) {
			std::unique_lock l{ lock };
			if (!reset && IsOpen() && fileMode == a_newMode && fileHandle.good())
				return true;

			if (IsOpen())
				fileHandle.close();

			fileHandle.clear();
			std::ios_base::openmode baseMode = std::ios::binary;
			switch (a_newMode) {
			case kRead:
				baseMode |= std::ios::in;
				break;
			case kWrite:
				baseMode |= (std::ios::in | std::ios::out);
				break;
			case kOverwrite:
				baseMode |= (std::ios::out | std::ios::trunc);
				break;
			}

			fileHandle.open(cachePath, baseMode);
			if (!fileHandle.is_open()) {
				logger::warn("Failed to open {}", cachePath);
				return false;
			}

			fileMode = a_newMode;
			return true;
		}

		static bool Load() {
			std::unique_lock l{ lock };
			if (IsLoaded())
				Clear();

			if (!std::filesystem::exists(cachePath)) {
				return false;
			}

			if (!SetMode(kRead)) {
				return false;
			}

			fileHandle.seekg(0, std::ios::beg);
			while (fileHandle.peek() != EOF) {
				std::string filename;
				if (!ReadString(&filename)) {
					logger::warn("Failed to read filename from anim cache!");
					Clear();
					return false;
				}

				uint64_t size;
				if (!ReadValue(&size)) {
					logger::warn("Failed to read file size from anim cache!");
					Clear();
					return false;
				}

				uint64_t offset = fileHandle.tellg();

				if (!fileHandle.seekg(offset + size)) {
					logger::warn("File size in anim cache is beyond end of file!");
					Clear();
					return false;
				}

				fileTable.insert({ filename, { offset, size } });
			}

			loaded = true;
			return true;
		}

		static bool AddFile(const std::string& filename, const std::string& data) {
			std::unique_lock l{ lock };
			if (!std::filesystem::exists(cachePath)) {
				SetMode(kOverwrite);
			}

			if (!SetMode(kWrite)) {
				return false;
			}

			if (fileTable.contains(filename)) {
				return false;
			}

			fileHandle.seekg(0, std::ios::end);

			if (!WriteString(&filename)) {
				logger::warn("Failed to write file name to anim cache!");
				return false;
			}

			uint64_t size = data.size();
			if (!WriteValue(&size)) {
				logger::warn("Failed to write file size to anim cache!");
				return false;
			}

			uint64_t offset = fileHandle.tellg();

			if (!fileHandle.write(data.data(), size)) {
				logger::warn("Failed to write file to anim cache!");
				return false;
			}

			fileTable.insert({ filename, { offset, size } });
			fileHandle.flush();

			loaded = true;
			return true;
		}

		static std::string GetFile(const std::string& filename) {
			std::unique_lock l{ lock };
			std::vector<char> result;

			auto iter = fileTable.find(filename);
			if (iter == fileTable.end())
				return "";

			if (!SetMode(kRead))
				return "";

			result.resize(iter->second.size);

			if (!fileHandle.seekg(iter->second.offset)) {
				return "";
			}
			
			if (!fileHandle.read(result.data(), iter->second.size)) {
				result.clear();
			}

			return std::move(std::string(result.begin(), result.end()));
		}

		static void Clear() {
			std::unique_lock l{ lock };
			if (IsOpen())
				fileHandle.close();

			fileTable.clear();
			loaded = false;
		}

		static void Delete() {
			std::unique_lock l{ lock };
			Clear();
			try {
				std::filesystem::remove(cachePath);
			} catch (...) {}
		}
		
	private:
		template <typename T>
		static bool ReadValue(T* out)
		{
			if (fileHandle.read(reinterpret_cast<char*>(out), sizeof(T))) {
				return true;
			} else {
				return false;
			}
		}

		static bool ReadString(std::string* out)
		{
			std::vector<char> temp;
			uint16_t strSize;

			if (!ReadValue(&strSize))
				return false;

			temp.resize(strSize);

			if (!fileHandle.read(temp.data(), strSize))
				return false;

			(*out) = std::move(std::string(temp.begin(), temp.end()));
			return true;
		}

		template <typename T>
		static bool WriteValue(T* in)
		{
			if (fileHandle.write(reinterpret_cast<char*>(in), sizeof(T))) {
				return true;
			} else {
				return false;
			}
		}

		static bool WriteString(const std::string* in)
		{
			if (in->size() > UINT16_MAX) {
				logger::warn("Cannot write string to anim cache, length exceeds max.");
				return false;
			}

			uint16_t shortSize = static_cast<uint16_t>(in->size());
			if (!WriteValue(&shortSize))
				return false;

			if (fileHandle.write(in->data(), in->size())) {
				return true;
			} else {
				return false;
			}
		}

		inline static safe_mutex lock;
		inline static Mode fileMode = kRead;
		inline static std::unordered_map<std::string, FileTableEntry> fileTable;
		inline static bool loaded = false;
		inline static std::fstream fileHandle;
	};
}
