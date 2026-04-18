#pragma once
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

namespace Scene
{
	class offset_optional : public RE::NiPoint3
	{
		RE::NiPoint3 np3;
		float angle;
		bool hasVal;

	public:
		
		offset_optional()
		{
			np3.x = 0;
			np3.y = 0;
			np3.z = 0;
			angle = 0;
			hasVal = 0;
		}

		offset_optional(float x)
		{
			np3.x = x;
			np3.y = x;
			np3.z = x;
			angle = 0;
			hasVal = 1;
		}

		offset_optional(float x, float y, float z)
		{
			np3.x = x;
			np3.y = y;
			np3.z = z;
			angle = 0;
			hasVal = 1;
		}

		offset_optional(float x, float y, float z, float angle_z)
		{
			np3.x = x;
			np3.y = y;
			np3.z = z;
			angle = angle_z;
			hasVal = 1;
		}

		offset_optional(const std::vector<float> vec)
		{
			if (vec.size() == 1)
			{
				np3.x = vec[0];
				np3.y = vec[0];
				np3.z = vec[0];
				angle = 0;
				hasVal = 1;
			} else if (vec.size() == 3)
			{
				np3.x = vec[0];
				np3.y = vec[1];
				np3.z = vec[2];
				angle = 0;
				hasVal = 1;
			} else if (vec.size() >= 4) {
				np3.x = vec[0];
				np3.y = vec[1];
				np3.z = vec[2];
				angle = vec[3];
				hasVal = 1;
			} 
			else
			{
				np3.x = 0;
				np3.y = 0;
				np3.z = 0;
				angle = 0;
				hasVal = 0;
			}
		}


		offset_optional& operator=(const RE::NiPoint3& v)
		{
			np3.x = v.x;
			np3.y = v.y;
			np3.z = v.z;
			hasVal = 1;
			return *this;
		}

		offset_optional& operator=(const offset_optional& v)
		{
			if (v.hasVal) {
				np3.x = v.np3.x;
				np3.y = v.np3.y;
				np3.z = v.np3.z;
				angle = v.angle;
				hasVal = true;
			} else {
				np3.x = 0;
				np3.y = 0;
				np3.z = 0;
				angle = 0;
				hasVal = 0;
			}
			return *this;
		}

		bool operator==(const offset_optional& v) const
		{
			if (hasVal == v.hasVal == false)
				return true;
			if (hasVal == v.hasVal && np3 == v.np3 && angle == v.angle)
				return true;
			return false;
		}

		bool operator!=(const offset_optional& v) const
		{
			return !(*this == v);
		}

		//operator RE::NiPoint3() const
		//{
		//	if (hasVal)
		//		return np3;
		//	else
		//		return RE::NiPoint3();
		//}

		//operator RE::NiPoint3A() const
		//{
		//	if (hasVal)
		//		return np3a;
		//	else
		//		return RE::NiPoint3A();
		//}
	public:
		bool has_value() const
		{
			return hasVal;
		}

		 RE::NiPoint3& value()
		{
			return np3;
		}

		 float& valueA()
		 {
			 return angle;
		 }

		void set_has_value(bool v)
		{
			hasVal = v;
		}

		void clear()
		{
			np3.x = 0;
			np3.y = 0;
			np3.z = 0;
			angle = 0;
			hasVal = 0;
		}
	};

	std::vector<std::string> SplitString(const std::string& fullString, const std::string& delimiter)
	{
		std::vector<std::string> substrings;
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

	std::vector<offset_optional> offset_from_string(const std::string& str)
	{
		std::vector<offset_optional> res;
		std::vector<std::string> strings = SplitString(str, ";");
		for (auto& off_str : strings)
		{
			size_t g = off_str.find(';');
			while (g != std::string::npos) off_str.erase(g, 1), g = off_str.find(';');
				
			auto sub = SplitString(off_str, ",");
			std::vector<float> floats;
			for (auto s : sub)
			{
				try {
					float f = stof(s);
					floats.push_back(f);
				} catch (...) {
					floats.push_back(0.f);
				}
			}
			offset_optional offset(floats);
			if (offset.has_value())
				res.push_back(offset);
		}
		return res;
	}

	std::string offset_to_string(std::vector<offset_optional>& offset_arr)
	{
		std::string res("");
		for (auto offset_opt = offset_arr.begin(); offset_opt != offset_arr.end(); ++offset_opt)
		{
			if (offset_opt->has_value()) {
				res += std::to_string(offset_opt->value().x);
				res += ",";
				res += std::to_string(offset_opt->value().y);
				res += ",";
				res += std::to_string(offset_opt->value().z);
				res += ",";
				res += std::to_string(offset_opt->valueA());
			}
			if (offset_arr.end() - 1 != offset_opt)
				res += ";";
		}
		return res;
	}
}


//namespace Scene
//{
//	struct offset_optional
//	{
//		struct Value
//		{
//			float x = 0, y = 0, z = 0, a = 0;
//
//			Value(std::vector<float> vec)
//			{
//				x = vec.size() > 0 ? vec[0] : 0;
//				y = vec.size() > 1 ? vec[1] : 0;
//				z = vec.size() > 2 ? vec[2] : 0;
//				a = vec.size() > 3 ? vec[3] : 0;
//			}
//
//			Value() :
//				x(0), y(0), z(0), a(0){};
//
//			Value(RE::NiPoint3 p) :
//				x(p.x), y(p.y), z(p.z), a(0) {}
//
//			Value(float x, float y, float z) :
//				x(x), y(y), z(z), a(0) {}
//
//			Value(float x, float y, float z, float a) :
//				x(x), y(y), z(z), a(a) {}
//
//			Value(const float& val) :
//				x(val), y(val), z(val){};
//
//			Value(const float val[3]) :
//				x(val[0]), y(val[2]), z(val[3]), a(0){};
//
//			Value(const float val[4]) :
//				x(val[0]), y(val[2]), z(val[3]), a(val[4]){};
//
//			Value(const RE::NiPoint3& p) :
//				x(p.x), y(p.y), z(p.z), a(0) {}
//
//			Value(const float& x, const float& y, const float& z) :
//				x(x), y(y), z(z), a(0) {}
//
//			Value(const float& x, const float& y, const float& z, const float& a) :
//				x(x), y(y), z(z), a(a) {}
//
//			Value(Value& copy) :
//				x(copy.x), y(copy.y), z(copy.z), a(copy.a){};
//
//			Value(const Value& copy) :
//				x(copy.x), y(copy.y), z(copy.z), a(copy.a){};
//
//			Value& operator=(std::vector<float> vec)
//			{
//				x = vec.size() > 0 ? vec[0] : 0;
//				y = vec.size() > 1 ? vec[1] : 0;
//				z = vec.size() > 2 ? vec[2] : 0;
//				a = vec.size() > 3 ? vec[3] : 0;
//				return *this;
//			}
//
//			Value& operator=(const float val[3])
//			{
//				x = val[0], y = val[1], z = val[2], a = 0;
//				return *this;
//			}
//
//			Value& operator=(const float val[4])
//			{
//				x = val[0], y = val[1], z = val[2], a = val[3];
//				return *this;
//			}
//
//			Value& operator=(const Value& val)
//			{
//				x = val.x, y = val.y, z = val.z, a = val.a;
//				return *this;
//			}
//
//			Value& operator=(const float& val)
//			{
//				x = val, y = val, z = val, a = 0;
//				return *this;
//			}
//
//			bool operator==(const Value& val) const
//			{
//				if ((x == val.x) && (y == val.y) && (z == val.z) && (a == val.a))
//					return true;
//				return false;
//			}
//
//			bool operator==(const RE::NiPoint3& p) const
//			{
//				if ((x == p.x) && (y == p.y) && (z == p.z))
//					return true;
//				return false;
//			}
//
//			operator RE::NiPoint3() const
//			{
//				return RE::NiPoint3(x, y, z);
//			}
//		};
//
//		void operator=(const RE::NiPoint3& v)
//		{
//			val.x = v.x;
//			val.y = v.y;
//			val.z = v.z;
//			val.a = 0;
//			hasVal = true;
//		}
//
//		void operator=(const offset_optional& v)
//		{
//			val.x = v.val.x;
//			val.y = v.val.y;
//			val.z = v.val.z;
//			val.a = v.val.a;
//			hasVal = true;
//		}
//
//		bool operator==(const offset_optional& v) const
//		{
//			if (hasVal == v.hasVal == false)
//				return true;
//			if (hasVal == v.hasVal && val.x == v.val.x && val.y == v.val.y && val.z == v.val.z && val.a == v.val.a)
//				return true;
//			return false;
//		}
//
//		bool operator!=(const offset_optional& v) const
//		{
//			return !(v.val == val);
//		}
//
//		operator RE::NiPoint3() const
//		{
//			if (hasVal)
//				return RE::NiPoint3(val);
//			else
//				return RE::NiPoint3();
//		}
//
//		//void operator=(const std::nullopt_t&)
//		//{
//		//	hasVal = false;
//		//}
//
//		bool has_value() const
//		{
//			return hasVal;
//		}
//
//		Value& value()
//		{
//			return val;
//		}
//
//		RE::NiPoint3 get_NiPoint3()
//		{
//			return (val);
//		}
//
//		float get_angle()
//		{
//			return val.a;
//		}
//
//		void set_has_value(bool v)
//		{
//			hasVal = v;
//		}
//
//		void clear()
//		{
//			val.x = 0;
//			val.y = 0;
//			val.z = 0;
//			val.a = 0;
//			hasVal = true;
//		}
//
//		offset_optional() :
//			val(), hasVal(false) {}
//
//		offset_optional(Value& val) :
//			val(val), hasVal(true) {}
//
//		offset_optional(Value& val, bool& hasVal) :
//			val(val), hasVal(hasVal) {}
//
//		offset_optional(RE::NiPoint3 val, bool hasVal) :
//			val(val), hasVal(hasVal) {}
//
//		offset_optional(offset_optional& copy) :
//			val(copy.val), hasVal(copy.hasVal) {}
//
//		offset_optional(const offset_optional& copy) :
//			hasVal(copy.hasVal)
//		{
//			if (hasVal) {
//				val.x = copy.val.x;
//				val.y = copy.val.y;
//				val.z = copy.val.z;
//				val.a = copy.val.a;
//			}
//		}
//
//		offset_optional(std::vector<float> vec)
//		{
//			hasVal = vec.size() > 0 ? val = vec, true : false;
//		}
//
//	private:
//		Value val = 0.f;
//		bool hasVal = false;
//	};
//}
