#pragma once

namespace Data
{
	class IdentifiableObject
	{
	public:
		std::string id;
		int32_t loadPriority = 0;

		/*static bool StringToFormId(const std::string_view in, uint32_t& out)
		{
			if (in.size() < 1) {
				return false;
			}

			try {
				out = std::stoul(std::format("0x{}", in), nullptr, 16);
				return true;
			} catch (...) {
				return false;
			}
		}*/
#ifdef min
#	define min_was_defined 1
#	undef min
#endif
		static bool StringToFormId(const std::string_view in, uint32_t& out)
		{
			if (in.size() < 1) {
				return false;
			}

			std::string_view trimmed = in;
			if (in.length() > 6) {
				trimmed = in.substr(in.length() - 6);
			}
			
			trimmed.remove_prefix(std::min(trimmed.size(), trimmed.find_first_not_of('0')));
			if (!trimmed.empty() && trimmed.front() == 'x') {
				trimmed.remove_prefix(1);
			}
			
			try {
				out = std::stoul(std::string(trimmed), nullptr, 16);  // Используем std::string для преобразования
				return true;
			} catch (const std::invalid_argument&) {
				return false;  // Неверный формат
			} catch (const std::out_of_range&) {
				return false;  // Число вне диапазона
			}
		}
#ifdef min_was_defined
#	undef min_was_defined
#	define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


		struct StringFormInfo
		{
			std::string source = "";
			std::string form = "";
		};

		static StringFormInfo FormToStrings(RE::TESForm* form) {
			StringFormInfo result;
			if (!form)
				return result;

			auto file = form->GetDescriptionOwnerFile();
			uint32_t id = form->formID;
			if (file->GetCompileIndex() == 0xFE) {
				id &= 0x00000FFF;
			} else {
				id &= 0x00FFFFFF;
			}

			result.source = file->filename;
			result.form = std::format("{:X}", id);
			return result;
		}

		template <typename T>
		static T* StringsToForm(const std::string_view& source, const std::string_view& form, bool verbose = true) {
			uint32_t formId = 0;
			if (!StringToFormId(form, formId)) {
				return nullptr;
			}

			auto rForm = RE::TESDataHandler::GetSingleton()->LookupForm<T>(formId, source);
			if (rForm == nullptr) {
				if (verbose)
					logger::warn("'{}'->'{:08X}' does not point to a valid {}.", source, formId, typeid(T).name());
				return nullptr;
			}

			return rForm;
		}

		void SetID(const IdentifiableObject& other) {
			id = other.id;
			loadPriority = other.loadPriority;
		}

		bool ParseID(XMLUtil::Mapper& m) {
			m(&id, ""s, true, true, "Node has no 'id' attribute!", "id");
			m.GetMinMax(&loadPriority, 0, true, false, "", INT32_MIN, INT32_MAX, "loadPriority");
			return m;
		}
	};

	template <typename T>
	class LinkableForm
	{
		bool isLinked = false;
		std::unique_ptr<std::pair<std::string, std::string>> info = nullptr;
		T* form = nullptr;

	public:
		LinkableForm() = default;

		LinkableForm(const IdentifiableObject::StringFormInfo& i)
		{
			info = std::make_unique<std::pair<std::string, std::string>>(i.source, i.form);
		}

		LinkableForm(const std::string& _source, const std::string& _form)
		{
			info = std::make_unique<std::pair<std::string, std::string>>(_source, _form);
		}

		LinkableForm(LinkableForm<T>&& other) noexcept :
			isLinked(other.isLinked), info(std::move(other.info)), form(other.form)
		{
			other.form = nullptr;
		}

		LinkableForm(const LinkableForm<T>& other) :
			isLinked(other.isLinked), info(other.info ? std::make_unique<std::pair<std::string, std::string>>(*other.info) : nullptr), form(other.form) {}
		
		LinkableForm<T>& operator=(const LinkableForm<T>& other)
		{
			if (this != &other)
			{
				isLinked = other.isLinked;
				info = other.info ? std::make_unique<std::pair<std::string, std::string>>(*other.info) : nullptr;
				form = other.form;
			}
			return *this;
		}

		LinkableForm<T>& operator=(LinkableForm<T>&& other) noexcept
		{
			if (this != &other)
			{
				isLinked = other.isLinked;
				info = std::move(other.info);
				form = other.form;

				other.form = nullptr;
			}
			return *this;
		}

		bool operator<(const LinkableForm<T>& other) const
		{
			if (!info && !other.info) {
				return false;
			}
			if (!info) {
				return true;
			}
			if (!other.info) {
				return false;
			}

			uint32_t thisFormId = 0;
			uint32_t otherFormId = 0;

			if (!IdentifiableObject::StringToFormId(info->second, thisFormId)) {
				return false;
			}

			if (!IdentifiableObject::StringToFormId(other.info->second, otherFormId)) {
				return true;
			}

			if (thisFormId != otherFormId) {
				return thisFormId < otherFormId;
			}

			return info->first < other.info->first;
		}

		~LinkableForm() {}

		void set(const std::string& _source, const std::string& _form)
		{
			isLinked = false;
			info = std::make_unique<std::pair<std::string, std::string>>(_source, _form);
		}

		T* get(bool verbose = true)
		{
			if (!isLinked && info) {
				T* formPtr = IdentifiableObject::StringsToForm<T>(info->first, info->second, verbose);
				//info = nullptr;
				form = formPtr;
				isLinked = true;
			}

			return form;
		}

		bool operator==(const LinkableForm& f) const
		{
			if (!info || !f.info) {
				return false;  // Если один из объектов не инициализирован, возвращаем false
			}

			auto compare_formid = [](const std::string& a, const std::string& b) {
				uint32_t a_ = 0, b_ = 0;
				IdentifiableObject::StringToFormId(a, a_);
				IdentifiableObject::StringToFormId(b, b_);
				return a_ == b_;
			};

			 // Сравнение Form ID
			uint32_t a_ = 0, b_ = 0;
			if (!IdentifiableObject::StringToFormId(info->second, a_) ||
				!IdentifiableObject::StringToFormId(f.info->second, b_)) {
				return false;  // Если преобразование не удалось, возвращаем false
			}

			// Сравнение source с приведением к нижнему регистру
			return info->first == f.info->first && 
				std::equal(info->first.begin(), info->first.end(), f.info->first.begin(), f.info->first.end(),
				[](unsigned char a, unsigned char b) {
					return std::tolower(a) == std::tolower(b);
				});
		}
	};
}
