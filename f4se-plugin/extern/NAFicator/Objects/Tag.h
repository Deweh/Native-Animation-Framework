#pragma once
#include "ParsedObject.h"

namespace NAFicator
{
	class TagHolder
	{
	public:
		void parse_tags(Data::XMLUtil::Mapper& m);
		void merge_tags(const TagHolder& other);
		void merge_and_clear_tags(TagHolder& other);

		std::set<std::string> tags;
	};

	class Tag :
		public ParsedObject,
		public TagHolder
	{
		RE::BSSpinLock lock;

	public:
		bool replace = false;
		Tag(Data::XMLUtil::Mapper& m) :
			ParsedObject(kTag) { parse(m); };

		bool parse(Data::XMLUtil::Mapper&) override;
		Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper&) override;
		std::ostringstream& ssprint(std::ostringstream&) const override;
		bool is_valid() override;
	};
}
