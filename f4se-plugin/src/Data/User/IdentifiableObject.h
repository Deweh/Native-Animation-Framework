#pragma once

namespace Data
{
	class IdentifiableObject
	{
	public:
		std::string id;
		int32_t loadPriority = 0;

		static bool StringToFormId(const std::string_view in, uint32_t& out)
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
		}

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
		LinkableForm() {}

		LinkableForm(const std::string& _source, const std::string& _form) {
			info = std::make_unique<std::pair<std::string, std::string>>();
			info->first = _source;
			info->second = _form;
		}

		LinkableForm(LinkableForm<T>&& other) {
			isLinked = other.isLinked;
			info = std::move(other.info);
			form = other.form;
		}

		~LinkableForm() {}

		void set(const std::string& _source, const std::string& _form) {
			isLinked = false;
			info = std::make_unique<std::pair<std::string, std::string>>();
			info->first = _source;
			info->second = _form;
		}

		T* get(bool verbose = true)
		{
			if (!isLinked) {
				T* formPtr = IdentifiableObject::StringsToForm<T>(info->first, info->second, verbose);
				info = nullptr;
				form = formPtr;
				isLinked = true;
			}

			return form;
		}
	};
}
