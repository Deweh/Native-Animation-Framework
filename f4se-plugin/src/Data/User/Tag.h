#pragma once

namespace Data
{
	class TagHolder
	{
	public:
		void ParseTags(XMLUtil::Mapper& m) {
			std::string combinedTags;
			m(&combinedTags, XMLUtil::Mapper::emptyStr, false, false, "", "tags");
			if (combinedTags.size() > 0) {
				Utility::TransformStringToLower(combinedTags);
				Utility::ForEachSubstring(combinedTags, ",", [&](const std::string_view& strv) {
					tags.emplace(strv);
				});
			}
		}

		void Merge(const TagHolder& other) {
			tags.insert(other.tags.begin(), other.tags.end());
		}

		void MergeAndClear(TagHolder& other) {
			Merge(other);
			other.tags.clear();
		}

		std::set<std::string> tags;
	};

	class TagData : public TagHolder
	{
	public:
		bool replace = false;
		RE::BSSpinLock lock;

		static void Parse(XMLUtil::Mapper& m)
		{
			std::string position;
			bool replace = false;
			m(&replace, false, true, false, "", "replace");
			m(&position, XMLUtil::Mapper::emptyStr, false, true, "tag node has no 'position' attribute!", "position");
			if (position.size() < 1)
				return;

			auto& ele = Datas[position];
			RE::BSAutoLock l{ ele.lock };
			ele.replace = replace;

			if (replace)
				ele.tags.clear();

			ele.ParseTags(m);
		}

		inline static concurrency::concurrent_unordered_map<std::string, TagData> Datas;
	};
}
