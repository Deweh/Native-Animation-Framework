#pragma once
#include <Windows.h>
#include <stdio.h>
#include <shared_mutex>
#include <random>

#define ALPHANUMERIC_UNDERSCORE_HYPHEN "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"

class Utility
{
public:
	inline static bool PCFreqAcquired = false;
	inline static double PCFreq = 0.0;
	inline static __int64 CounterStart = 0;

	template <typename T>
	static std::map<T, size_t> VectorToIndexMap(const std::vector<T>& vk)
	{
		std::map<T, size_t> result;
		for (size_t i = 0; i < vk.size(); i++) {
			result[vk[i]] = i;
		}
		return result;
	}

	template <typename T>
	static bool VectorContains(const std::vector<T>& vk, T ele)
	{
		return std::find(vk.begin(), vk.end(), ele) != vk.end();
	}

	static bool FirstValidValue(std::string& out, std::vector<std::string> in)
	{
		for (auto& s : in) {
			if (s.size() > 0) {
				out = s;
				return true;
			}
		}

		return false;
	}

	static void GetPCFreq()
	{
		if (!PCFreqAcquired) {
			LARGE_INTEGER li;
			if (QueryPerformanceFrequency(&li)) {
				PCFreq = double(li.QuadPart) / 1000.0;
				PCFreqAcquired = true;
			} else {
				logger::warn("QueryPerformanceFrequency failed!");
			}
		}
	}

	static void StartPerformanceCounter()
	{
		GetPCFreq();
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		CounterStart = li.QuadPart;
	}

	static double GetPerformanceCounter()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return (double(li.QuadPart - (CounterStart)) / PCFreq) / 1000.0;
	}

	static double GetPerformanceCounterMS()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return (double(li.QuadPart - (CounterStart)) / PCFreq);
	}

	static int64_t CreatePerfCounter()
	{
		GetPCFreq();
		LARGE_INTEGER result;
		QueryPerformanceCounter(&result);
		return result.QuadPart;
	}

	static double QueryPerfCounterTime(int64_t counter)
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return (double(li.QuadPart - (counter)) / PCFreq);
	}

	static void TransformStringToLower(std::string& str) {
		std::transform(str.begin(), str.end(), str.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });
	}

	static std::string StringRestrictChars(const std::string_view& str, const std::string_view& charsToKeep) {
		std::string result;
		for (const char& c : str) {
			if (charsToKeep.find(c) != std::string::npos) {
				result.push_back(c);
			}
		}
		return result;
	}

	static std::string StringToLower(std::string_view str) {
		std::string res(str);
		TransformStringToLower(res);
		return res;
	}

	static std::string StringToLower(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });
		return str;
	}

	static bool StringEndsWith(const std::string_view& fullString, const std::string_view& ending)
	{
		if (fullString.size() >= ending.size()) {
			return (fullString.rfind(ending, fullString.size() - ending.size()) != std::string::npos);
		} else {
			return false;
		}
	}

	static bool StringStartsWith(const std::string_view& fullString, const std::string_view& start)
	{
		if (fullString.size() >= start.size()) {
			return (fullString.rfind(start, 0) != std::string::npos);
		} else {
			return false;
		}
	}

	static void ForEachSubstring(const std::string_view& fullString, const std::string_view& delimiter,
		std::function<void(const std::string_view&)> func, std::function<void(size_t)> initFunc = nullptr) {
		size_t start = 0;
		size_t end = fullString.find(delimiter);

		if (initFunc != nullptr) {
			size_t originalEnd = end;
			size_t size = 0;
			while (end != std::string::npos) {
				size++;
				start = end + delimiter.length();
				end = fullString.find(delimiter, start);
			}

			if (start < fullString.length()) {
				size++;
			}

			initFunc(size);
			start = 0;
			end = originalEnd;
		}

		while (end != std::string::npos) {
			func(fullString.substr(start, end - start));
			start = end + delimiter.length();
			end = fullString.find(delimiter, start);
		}

		if (start < fullString.length()) {
			func(fullString.substr(start));
		}
	}

	static std::vector<std::string_view> SplitString(const std::string_view& fullString, const std::string_view& delimiter) {
		std::vector<std::string_view> substrings;
		size_t start = 0;
		size_t end = fullString.find(delimiter);

		while (end != std::string::npos) {
			substrings.push_back(fullString.substr(start, end - start));
			start = end + delimiter.length();
			end = fullString.find(delimiter, start);
		}

		if (start < fullString.length()) {
			substrings.push_back(fullString.substr(start));
		}

		return substrings;
	}

	static float StringToFloat(const std::string_view& str) {
		float result = 0;
		try {
			result = std::stof(str.data());
		}
		catch (...) {
			return 0.0f;
		}
		return result;
	}

	template <typename T>
	static T SelectRandom(std::vector<T> elements)
	{
		return elements[RandomNumber<size_t>(0, elements.size() - 1)];
	}

	template <typename T>
	static T SelectWeightedRandom(const std::vector<std::pair<uint32_t, T>>& weightsAndElements)
	{
		if (weightsAndElements.size() < 1) {
			return T();
		}

		uint32_t sum = 0;
		for (auto& i : weightsAndElements) {
			sum += i.first;
		}

		int64_t num = RandomNumber<int64_t>(0, sum - 1);

		for (auto& i : weightsAndElements) {
			if (num < i.first)
				return i.second;
			num -= i.first;
		}
		return T();
	}

	template <typename T>
	static T RandomNumber(T minNum, T maxNum)
	{
		static thread_local std::mt19937 generator((std::random_device())());
		std::uniform_int_distribution<T> distr(minNum, maxNum);
		return distr(generator);
	}

	static std::optional<uint32_t> StringToUInt32(const std::string& s) {
		uint32_t result;
		try {
			result = std::stoul(s);
		}
		catch (...) {
			return std::nullopt;
		}
		return result;
	}
};

class holder_aware_mutex : public std::mutex
{
public:
	void lock()
	{
		std::mutex::lock();
		m_holder = std::this_thread::get_id();
	}

	void unlock()
	{
		m_holder = std::thread::id();
		std::mutex::unlock();
	}

	bool caller_is_holder() {
		return m_holder == std::this_thread::get_id();
	}

	~holder_aware_mutex() {
		if (!caller_is_holder()) {
			lock();
		}

		unlock();
	}

private:
	std::atomic<std::thread::id> m_holder;
};

class safe_mutex : public std::recursive_mutex
{
public:
	~safe_mutex()
	{
		lock();

		while (count > 0) {
			unlock();
		}
	}

	void lock()
	{
		std::recursive_mutex::lock();
		count += 1;
	}

	void unlock()
	{
		count -= 1;
		std::recursive_mutex::unlock();
	}

private:
	std::atomic<uint64_t> count = 0;
};

typedef std::shared_lock<std::shared_mutex> ReadLock;
typedef std::unique_lock<std::shared_mutex> WriteLock;
typedef std::unique_lock<safe_mutex> SafeRWLock;

template <class T>
class ThreadSafeAccessor
{
public:
	template <class T, class L>
	class Accessor
	{
	public:
		Accessor(L _lock, T* _data) {
			lock = std::move(_lock);
			data = _data;
		}

		void release() {
			lock = L();
			released = true;
		}

		T* operator->() {
			return data;
		}

		T& operator*() {
			return *data;
		}
	private:
		bool released = false;
		T* data;
		L lock;
	};

	ThreadSafeAccessor()
	{
	}

	ThreadSafeAccessor(T _data) {
		data = _data;
	}

	Accessor<T, WriteLock> operator*()
	{
		return GetWriteAccess();
	}

	void Reset(T _data) {
		std::unique_lock l{ lock };
		data = _data;
	}

	Accessor<T, SafeRWLock> GetAccess() {
		std::unique_lock l{ lock };
		return Accessor<T, SafeRWLock>(std::move(l), &data);
	}

	Accessor<T, SafeRWLock> GetReadAccess() {
		return GetAccess();
	}

	Accessor<T, SafeRWLock> GetWriteAccess()
	{
		return GetAccess();
	}

	void operator=(T other) {
		Reset(other);
	}

private:
	T data;
	safe_mutex lock;
};

template <typename T>
class Singleton
{
public:
	static T* GetSingleton() {
		static T inst;
		return &inst;
	}

protected:
	Singleton(){};
	~Singleton(){};		

private:
	inline static T* singletonInstance = GetSingleton();
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;
};
