#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#ifndef  MAX_PATH
#	define MAX_PATH 260
#endif  // ! MAX_PATH

struct F4SESerializationInterface;

// Class is similar to BSFixedString, it is meant to not interfere with base game string caching when loading data early on
class F4EEFixedString
{
public:
	F4EEFixedString() :
		m_internal() { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }
	F4EEFixedString(const char* str) :
		m_internal(str) { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }
	F4EEFixedString(const std::string& str) :
		m_internal(str) { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }
	F4EEFixedString(const RE::BSFixedString& str) :
		m_internal(str.c_str()) { m_hash = hash_lower(m_internal.c_str(), m_internal.size()); }

	bool operator==(const F4EEFixedString& x) const
	{
		if (m_internal.size() != x.m_internal.size())
			return false;

		if (_stricmp(m_internal.c_str(), x.m_internal.c_str()) == 0)
			return true;

		return false;
	}

	uint32_t length() { return m_internal.size(); }

	operator RE::BSFixedString() const { return RE::BSFixedString(m_internal.c_str()); }
	RE::BSFixedString AsBSFixedString() const { return operator RE::BSFixedString(); }

	const char* c_str() const { return operator const char*(); }
	operator const char*() const { return m_internal.c_str(); }

	size_t hash_lower(const char* str, size_t count)
	{
		const size_t _FNV_offset_basis = 14695981039346656037ULL;
		const size_t _FNV_prime = 1099511628211ULL;

		size_t _Val = _FNV_offset_basis;
		for (size_t _Next = 0; _Next < count; ++_Next) {  // fold in another byte
			_Val ^= (size_t)tolower(str[_Next]);
			_Val *= _FNV_prime;
		}
		return _Val;
	}

	size_t GetHash() const
	{
		return m_hash;
	}

protected:
	std::string m_internal;
	size_t m_hash;
};

typedef std::shared_ptr<F4EEFixedString> StringTableItem;
typedef std::weak_ptr<F4EEFixedString> WeakTableItem;

namespace std
{
	template <>
	struct hash<F4EEFixedString>
	{
		size_t operator()(const F4EEFixedString& x) const
		{
			return x.GetHash();
		}
	};
	template <>
	struct hash<StringTableItem>
	{
		size_t operator()(const StringTableItem& x) const
		{
			return x->GetHash();
		}
	};
}

class StringTable
{
public:
	enum
	{
		kSerializationVersion = 1,
	};

	void Save(const F4SESerializationInterface* intfc, uint32_t kVersion);
	bool Load(const F4SESerializationInterface* intfc, uint32_t kVersion, std::unordered_map<uint32_t, StringTableItem>& stringTable);
	void Revert();

	StringTableItem GetString(const F4EEFixedString& str);

	uint32_t GetStringID(const StringTableItem& str);

	void RemoveString(const F4EEFixedString& str);

protected:
	std::unordered_map<F4EEFixedString, WeakTableItem> m_table;
	std::vector<WeakTableItem> m_tableVector;
	RE::BSSpinLock m_lock;
};

struct ModInfo  // referred to by game as TESFile
{
	ModInfo(){};
	~ModInfo(){};

	uint64_t unk00[10];         // 000
	RE::BSFile* file;           // 050
	uint64_t unk58;             // 058
	uint64_t unk60;             // 060
	void* unk68;                // 068
	char name[MAX_PATH];        // 070
	char directory[MAX_PATH];   // 174
	uint64_t unk278[0xB8 / 8];  // 278
	struct Dependency
	{
		void* unk00;       // 00
		const char* name;  // 08
		Dependency* next;  // 10
	};
};
