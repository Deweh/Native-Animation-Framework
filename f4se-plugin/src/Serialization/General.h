#pragma once

namespace Serialization
{
	namespace General
	{
		const F4SE::SerializationInterface* s_intfc = nullptr;

		template <class T>
		T* DeserializeForm(uint32_t formId)
		{
			if (!s_intfc) {
				return nullptr;
			}

			auto formResolvedId = s_intfc->ResolveFormID(formId);

			if (formResolvedId.has_value()) {
				return RE::TESForm::GetFormByID<T>(formResolvedId.value());
			} else {
				return nullptr;
			}
		}

		template <typename T>
		RE::BSPointerHandle<T> EmptyHandle()
		{
			return RE::BSPointerHandle<T>(RE::BSUntypedPointerHandle(0));
		}

		template <typename T>
		uint32_t SerializeHandle(RE::BSPointerHandle<T> hndl)
		{
			auto frm = hndl.get().get();
			if (frm != nullptr) {
				return frm->formID;
			} else {
				logger::warn(FMT_STRING("Handle {:x} no longer points to a valid form, saving empty ID to co-save."), hndl.native_handle());
				return 0;
			}
		}
		
		template <typename T>
		RE::BSPointerHandle<T> DeserializeHandle(uint32_t formId)
		{
			auto frm = DeserializeForm<T>(formId);
			if (frm != nullptr) {
				return  RE::BSPointerHandle<T>(RE::BSUntypedPointerHandle(frm->GetHandle().native_handle()));
			} else {
				logger::warn(FMT_STRING("Ref ID from co-save is not valid. Empty handle will be used."));
				return EmptyHandle<T>();
			}
		}

		template <typename T>
		void SaveRecord(std::string_view rcrd, const T& data)
		{
			std::ostringstream buffer(std::ios::binary);

			try
			{
				cereal::BinaryOutputArchive archive(buffer);
				archive(data);
			}
			catch (std::exception& e)
			{
				logger::warn("Failed to serialize {} record data due to serialization error. Full Message: {}", rcrd, e.what());
				return;
			}

			std::string strData = buffer.str();
			size_t size = strData.size();

			if (size <= UINT32_MAX) {
				uint32_t smallSize = static_cast<uint32_t>(size);
				if (!s_intfc->WriteRecordData(&smallSize, sizeof(smallSize))) {
					logger::warn("Failed to write {} record size. Co-save might become corrupted.", rcrd);
					return;
				}
				if (!s_intfc->WriteRecordData(strData.data(), smallSize)) {
					logger::warn("Failed to write {} record data. Co-save might become corrupted.", rcrd);
				}
			} else {
				logger::warn("{} record data is over supported limit. Data will not be saved!", rcrd);
			}
		}

		template <typename T>
		bool LoadRecord(std::string_view rcrd, T& dataOut)
		{
			uint32_t size;
			if (s_intfc->ReadRecordData(&size, sizeof(size)) != sizeof(size)) {
				logger::warn("Failed to read {} record size. Data might be corrupted.", rcrd);
				return false;
			}

			std::vector<uint8_t> binaryData;
			binaryData.resize(size);
			if (s_intfc->ReadRecordData(&binaryData[0], size) != size) {
				logger::warn("Failed to read {} record data. Data might be corrupted.", rcrd);
				return false;
			}

			std::istringstream buffer(std::string(binaryData.begin(), binaryData.end()), std::ios::binary);

			try
			{
				cereal::BinaryInputArchive archive(buffer);
				archive(dataOut);
			}
			catch (std::exception& e)
			{
				logger::warn("Failed to read {} record data due to serialization error. Full Message: {}", rcrd, e.what());
				return false;
			}

			return true;
		}

		template <typename T>
		class SerializableForm
		{
		public:
			SerializableForm()
			{
			}

			SerializableForm(RE::TESForm* form)
			{
				base = form;
				if (base != nullptr) {
					data = base->As<T>();
				}
			}

			T* get() const {
				return data;
			}

			operator T* () const {
				return data;
			}

			T* operator->() const
			{
				return data;
			}

			void operator=(RE::TESForm* other)
			{
				base = other;
				data = nullptr;
				if (base != nullptr) {
					data = base->As<T>();
				}
			}

			void operator=(std::nullptr_t)
			{
				base = nullptr;
				data = nullptr;
			}

			bool operator==(SerializableForm other) const
			{
				return base == other.base;
			}

			bool operator==(void* other) const
			{
				return base == other;
			}

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				if (base == nullptr) {
					uint32_t emptyId = 0;
					ar(emptyId);
				} else {
					ar(base->formID);
				}
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				base = nullptr;
				data = nullptr;

				uint32_t baseId;
				ar(baseId);

				if (baseId != 0) {
					base = DeserializeForm<RE::TESForm>(baseId);
					if (base != nullptr) {
						data = base->As<T>();
					}
				}
			}
		private:
			RE::TESForm* base = nullptr;
			T* data = nullptr;
		};

		typedef SerializableForm<RE::TESForm> SerializableBaseForm;

		template <typename T>
		class SerializableHandle
		{
		public:
			SerializableHandle()
			{
			}

			SerializableHandle(RE::BSPointerHandle<T> hndl)
			{
				data = hndl;
			}

			operator RE::BSPointerHandle<T>() const
			{
				return data;
			}

			void reset() {
				data = EmptyHandle<T>();
			}

			RE::NiPointer<T> get() const {
				return data.get();
			}

			uint32_t hash() const
			{
				return data.native_handle_const();
			}

			RE::BSPointerHandle<T> native() const {
				return data;
			}

			void operator=(RE::BSPointerHandle<T> other)
			{
				data = other;
			}

			bool operator==(SerializableHandle<T> other) const
			{
				return data.native_handle_const() == other.data.native_handle_const();
			}

			operator bool() const {
				return data.native_handle_const() > 0;
			}

			template <class Archive>
			void save(Archive& ar, const uint32_t) const
			{
				uint32_t formId = SerializeHandle(data);
				ar(formId);
			}

			template <class Archive>
			void load(Archive& ar, const uint32_t)
			{
				uint32_t formId;
				ar(formId);

				data = DeserializeHandle<T>(formId);
			}

		private:
			RE::BSPointerHandle<T> data = EmptyHandle<T>();
		};

		typedef SerializableHandle<RE::Actor> SerializableActorHandle;
		typedef SerializableHandle<RE::TESObjectREFR> SerializableRefHandle;
	}
}

namespace std
{
	template <>
	struct std::hash<Serialization::General::SerializableHandle<RE::Actor>>
	{
		std::size_t operator()(const Serialization::General::SerializableHandle<RE::Actor>& hndl) const
		{
			return hndl.hash();
		}
	};

	template <>
	struct std::hash<Serialization::General::SerializableHandle<RE::TESObjectREFR>>
	{
		std::size_t operator()(const Serialization::General::SerializableHandle<RE::TESObjectREFR>& hndl) const
		{
			return hndl.hash();
		}
	};
}
