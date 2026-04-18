#pragma once

namespace Data {
	class XMLUtil
	{
	public:
		class Mapper
		{
			pugi::xml_node rootNode;  //NAF Bridge
			pugi::xml_node defaultsNode;
			pugi::xml_node currentNode;
			std::string_view fileName;
			bool successful = true;

		public:
			inline static std::string emptyStr = "";
			bool verbose = true;

			//NAF Bridge
			Mapper(const pugi::xml_node& defaults, const pugi::xml_node& node, const std::string_view fName)
			{
				rootNode = node;
				defaultsNode = defaults;
				currentNode = node;
				fileName = fName;
			}

			Mapper GetRoot()
			{
				return Mapper(defaultsNode, rootNode, fileName);
			}

			pugi::xml_node GetDefaultsNode()
			{
				return defaultsNode;
			}

			/*Mapper(const pugi::xml_node& defaults, const pugi::xml_node& node, const std::string_view fName) {
				defaultsNode = defaults;
				currentNode = node;
				fileName = fName;
			}*/
			//NAF Bridge end 

			void LogError(const std::string_view& errMsg) {
				if (verbose)
					logger::warn("[{} - Char:{}] {}", GetFileName(), currentNode.offset_debug(), errMsg);
			}

			void CustomFail(const std::string_view& errMsg) {
				successful = false;
				LogError(errMsg);
			}

			std::string_view GetFileName() const {
				return fileName;
			}

			pugi::xml_node GetCurrentNode() const {
				return currentNode;
			}

			std::string GetCurrentName() const {
				return currentNode.name();
			}

			void SetCurrentNode(const pugi::xml_node* node) {
				currentNode = *node;
			}

			bool DownNode(const std::string_view name, const std::string_view& errMsg, bool required = true)
			{
				if (!successful) {
					return false;
				}

				auto next = currentNode.child(name.data());
				if (next.empty()) {
					if (required) {
						LogError(errMsg);
						successful = false;
					}
					return false;
				}
				currentNode = next;
				return true;
			}

			void UpNode() {
				if (!successful) {
					return;
				}

				currentNode = currentNode.parent();
			}

			template <typename T>
			std::vector<T> ParseArray(const std::string& nodeName, const std::string_view& errMsg, bool required = true)
			{
				std::vector<T> result;
				GetArray([&](XMLUtil::Mapper& m) {
					T obj;
					obj.Parse(m);

					if (m)
						result.push_back(obj);

					return m;
				}, nodeName, errMsg, required);
				return result;
			}

			void GetArray(const std::function<bool(Mapper&)> func, const std::optional<std::string_view> name = std::nullopt, const std::string_view& errMsg = "", bool required = true, const std::function<void(size_t)> initFunc = nullptr)
			{
				if (!successful) {
					return;
				}

				const auto topNode = currentNode;
				
				if (name.has_value()) {
					auto children = topNode.children(name.value().data());
					if (children.empty() && required) {
						successful = false;
						LogError(errMsg);
						return;
					}
					if (initFunc != nullptr) {
						initFunc(std::distance(children.begin(), children.end()));
					}
					for (const auto& node : children) {
						SetCurrentNode(&node);
						if (!func(*this)) {
							break;
						}
					}
				} else {
					auto children = topNode.children();
					if (initFunc != nullptr) {
						initFunc(std::distance(children.begin(), children.end()));
					}
					for (const auto& node : children) {
						SetCurrentNode(&node);
						if (!func(*this)) {
							break;
						}
					}
				}
				
				SetCurrentNode(&topNode);
			}

			operator bool() const {
				return successful;
			}

			void ResetSuccessFlag() {
				successful = true;
			}

			template <typename T, class... Args>
			bool operator()(T* outVal,
				const T& defaultVal,
				bool allowDefault,
				bool required,
				const std::string_view& errMsg,
				Args... names)
			{
				return Get(outVal, defaultVal, allowDefault, required, errMsg, names...);
			}

			template <typename T, class... Args>
			void GetMinMax(T* outVal,
				const T& defaultVal,
				bool allowDefault,
				bool required,
				const std::string_view& errMsg,
				const T min,
				const T max,
				Args... names)
			{
				Get(outVal, defaultVal, allowDefault, required, errMsg, names...);
				if ((*outVal) < min || (*outVal) > max) {
					if (required) {
						LogError(errMsg);
						successful = false;
					} else {
						(*outVal) = std::clamp((*outVal), min, max);
					}
				}
			}

			template <typename T>
			void InternalGet(const pugi::xml_node* node, const std::string_view name, T* val, bool* b, const std::string_view optNode = "")
			{
				if (!(*b)) {
					bool hasOpt = false;
					pugi::xml_node oNode;
					if (optNode.size() > 0) {
						hasOpt = true;
						oNode = node->child(optNode.data());
					}
					if (GetValue(node, name, val) || (hasOpt && GetValue(&oNode, name, val))) {
						(*b) = true;
					}
				}
			}

			template <typename T, class... Args>
			bool GetOptNode(T* outVal,
				const T& defaultVal,
				const std::string& optionalNode,
				bool allowDefault,
				bool required,
				const std::string_view& errMsg,
				Args... names)
			{
				if (!successful) {
					return false;
				}

				bool found = false;

				([&] {
					InternalGet(&currentNode, names, outVal, &found, optionalNode);
				}(), ...);

				if (found) {
					return true;
				} else if (!allowDefault) {
					if (required) {
						successful = false;
						LogError(errMsg);
					} else {
						(*outVal) = defaultVal;
					}
					return false;
				}

				([&] {
					InternalGet(&defaultsNode, names, outVal, &found);
				}(), ...);

				if (found) {
					return true;
				} else {
					if (required) {
						successful = false;
						LogError(errMsg);
					} else {
						(*outVal) = defaultVal;
					}
					return false;
				}
			}

			template <typename T, class... Args>
			bool Get(T* outVal,
				const T& defaultVal,
				bool allowDefault,
				bool required,
				const std::string_view& errMsg,
				Args... names)
			{
				if (!successful) {
					return false;
				}

				bool found = false;

				([&] {
					InternalGet(&currentNode, names, outVal, &found);
				}(), ...);

				if (found) {
					return true;
				} else if (!allowDefault) {
					if (required) {
						successful = false;
						LogError(errMsg);
					} else {
						(*outVal) = defaultVal;
					}
					return false;
				}

				([&] {
					InternalGet(&defaultsNode, names, outVal, &found);
				}(), ...);

				if (found) {
					return true;
				} else {
					if (required) {
						successful = false;
						LogError(errMsg);
					} else {
						(*outVal) = defaultVal;
					}
					return false;
				}
			}
		};

		template <typename T>
		static bool GetValue(const pugi::xml_node* node, const std::string_view name, T* resOut);

		template <typename J>
		static bool GetValue(const pugi::xml_node* node, const std::string_view name, std::optional<J>* resOut)
		{
			(*resOut).emplace();
			if (!GetValue(node, name, &(*resOut).value())) {
				(*resOut).reset();
				return false;
			} else {
				return true;
			}
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, std::string* resOut)
		{
			auto* res = node->attribute(name.data()).as_string();
			if (res[0] != '\0') {
				(*resOut) = res;
				return true;
			} else {
				if (resOut->size() > 0)
					resOut->clear();
				return false;
			}
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, ActorGender* resOut)
		{
			auto* val = node->attribute(name.data()).as_string();
			if (strcmp(val, "M") == 0) {
				(*resOut) = Male;
			} else if (strcmp(val, "F") == 0) {
				(*resOut) = Female;
			} else {
				(*resOut) = Any;
			}
			return true;
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, int32_t* resOut)
		{
			(*resOut) = node->attribute(name.data()).as_int(INT32_MIN);
			return (*resOut) > INT32_MIN;
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, uint32_t* resOut)
		{
			(*resOut) = node->attribute(name.data()).as_uint(UINT32_MAX);
			return (*resOut) < UINT32_MAX;
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, uint8_t* resOut)
		{
			int32_t val = node->attribute(name.data()).as_int(UINT8_MAX);
			if (val > UINT8_MAX) {
				val = UINT8_MAX;
			}
			(*resOut) = static_cast<uint8_t>(val);
			return (*resOut) < UINT8_MAX;
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, float* resOut)
		{
			(*resOut) = node->attribute(name.data()).as_float(-500.0f);
			return (*resOut) > -499.999f;
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, double* resOut)
		{
			(*resOut) = node->attribute(name.data()).as_double(-10);
			return (*resOut) > -9.999;
		}

		template <>
		bool GetValue(const pugi::xml_node* node, const std::string_view name, bool* resOut)
		{
			auto* val = node->attribute(name.data()).as_string();
			if (val[0] == '\0') {
				return false;
			}
			if (_stricmp(val, "false") == 0 || strcmp(val, "0") == 0) {
				(*resOut) = false;
			} else if (_stricmp(val, "true") == 0 || strcmp(val, "1") == 0) {
				(*resOut) = true;
			} else {
				return false;
			}
			return true;
		}
	};
}
