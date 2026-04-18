#pragma once
#include <NAFicator/Objects/ObjectsHeader.h>
#include <NAFicator/Objects/ConditionSet.h>
#include <NAFicator/ThreadPool/ThreadPool.h>


namespace logger = F4SE::log;

extern RE::TESForm* get_form_from_string(const std::string& xFormID, const std::string& plugin);
extern RE::TESForm* get_form_by_editor_id(std::string editor_id);

namespace NAFicator
{
	static inline std::string get_unknown_id()
	{
		static size_t counter_unknown_id = 0;
		return "UNKNOWN_ID"s + std::to_string(++counter_unknown_id);
	}
	
	enum obj_type
	{
		kNone,
		kAction,
		kAnimation,
		kAnimationGroup,
		kEquipmentSet,
		kFurniture,
		kMfgSet,
		kMorphSet,
		kOverlay,
		kPosition,
		kPositionTree,
		kProtectedEquipment,
		kRace,
		kTag
	};

	struct CaseInsensitivePairObjTypeStringHash
	{
		std::size_t operator()(const std::pair<obj_type, std::string>& t) const;
	};

	struct CaseInsensitivePairObjTypeStringEqual
	{
		bool operator()(const std::pair<obj_type, std::string>& lhs, const std::pair<obj_type, std::string>& rhs) const;
	};
	
	class ParsedObject : public concurrency::concurrent_unordered_map<std::pair<obj_type, 
		std::string>, std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>, 
		CaseInsensitivePairObjTypeStringHash, CaseInsensitivePairObjTypeStringEqual>
	{
	protected:
		obj_type type;
		bool valid;
		int loadPriority = 0;
		std::string_view root_node;
		std::string id;
		std::string filename;
		std::string log;
		friend void priority_insert(std::shared_ptr<ParsedObject> ele);
	public:
		
		ParsedObject(obj_type t) :
			filename(""), type(t), root_node(get_type_view()), loadPriority(0), valid(true) {}

		virtual bool parse(Data::XMLUtil::Mapper&) = 0;
		virtual Data::XMLUtil::Mapper& parse_id(Data::XMLUtil::Mapper&) = 0;
		virtual std::ostringstream& ssprint(std::ostringstream&) const = 0;
		virtual bool is_valid() = 0;
		void set_invalid(const std::string& to_log);
		std::string_view get_type_view();
		inline obj_type get_type() { return type; };
		const std::string& get_id() const;
		const int get_priority() const;
	};
	using super = concurrency::concurrent_unordered_map<std::pair<obj_type, std::string>, 
		std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>, 
		CaseInsensitivePairObjTypeStringHash, CaseInsensitivePairObjTypeStringEqual>;

	static super parsed_objects;
	std::string_view get_type_view(obj_type);
	obj_type get_type(std::string_view);
	void priority_insert(std::shared_ptr<ParsedObject> ele);
	bool parse_XML_files(std::vector<std::shared_ptr<std::stringstream>>& xfiles);
	bool parse_xml(std::shared_ptr<std::stringstream> x);
	std::string& normalize_form_id(std::string& form_id);
	std::string& normalize_source(std::string& source);

	static inline std::vector<std::mutex> print_locker;
}

namespace fixes
{
	using namespace NAFicator;

	enum class CheckFlags : uint8_t
	{
		None = 0b000,       
		Animation = 0b001,  
		Group = 0b010,      
		Tree = 0b100        
	};
	CheckFlags operator|(CheckFlags lhs, CheckFlags rhs);
	bool operator&(CheckFlags lhs, CheckFlags rhs);

	void hkxficate();
	
	void for_every_type(obj_type t, std::function<void(std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& obj)> apply, bool only_valid = true);
	std::ostringstream& get_stream(obj_type t);
	void check_and_clear_furniture_bad_links();
	void print_all();
	auto skip(obj_type t);

	void replace_formid_to_hkx(std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>> obj);
	void remove_objects_linked_to_this_animation(const std::string& animation_id);
	
	void remove_missed_animations();
	void remove_positions_with_missed_links(CheckFlags flags);
	void remove_animation_groups_with_missed_animation_links();
	void remove_position_trees_with_missed_animation_links();
	void remove_positions_with_missed_animation_groups_links();
	void remove_positions_with_missed_position_trees_links();

	void update_tags();

	void merge_positions_optionals();

	//RE::BSAutoLock lock(l);
	void for_every_type_mt(obj_type t, std::function<void(std::pair<RE::BSSpinLock, std::shared_ptr<ParsedObject>>& obj)> apply, bool only_valid = true);
}
