#pragma once
#include <math.h>
#define PIf 3.14159265358979323846f

namespace dh
{
	/*****
	MIT License

	Copyright (c) 2021 Daniel Holden

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
	******/

	static inline float clampf(float x, float min, float max)
	{
		return x > max ? max : x < min ? min :
			                                x;
	}

	static inline float minf(float x, float y)
	{
		return x < y ? x : y;
	}

	static inline float maxf(float x, float y)
	{
		return x > y ? x : y;
	}

	static inline float squaref(float x)
	{
		return x * x;
	}

	static inline float lerpf(float x, float y, float a)
	{
		return (1.0f - a) * x + a * y;
	}

	static inline float signf(float x)
	{
		return x > 0.0f ? 1.0f : x < 0.0f ? -1.0f :
			                                0.0f;
	}

	static inline float fast_negexpf(float x)
	{
		return 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
	}

	static inline float fast_atanf(float x)
	{
		float z = fabs(x);
		float w = z > 1.0f ? 1.0f / z : z;
		float y = (PIf / 4.0f) * w - w * (w - 1.0f) * (0.2447f + 0.0663f * w);
		return copysign(z > 1.0f ? PIf / 2.0f - y : y, x);
	}

	static inline int clamp(int x, int min, int max)
	{
		return x < min ? min : x > max ? max :
			                                x;
	}

	struct vec3
	{
		vec3() :
			x(0.0f), y(0.0f), z(0.0f) {}
		vec3(float _x, float _y, float _z) :
			x(_x), y(_y), z(_z) {}

		float x, y, z;
	};

	static inline vec3 operator+(float s, vec3 v)
	{
		return vec3(v.x + s, v.y + s, v.z + s);
	}

	static inline vec3 operator+(vec3 v, float s)
	{
		return vec3(v.x + s, v.y + s, v.z + s);
	}

	static inline vec3 operator+(vec3 v, vec3 w)
	{
		return vec3(v.x + w.x, v.y + w.y, v.z + w.z);
	}

	static inline vec3 operator-(float s, vec3 v)
	{
		return vec3(v.x - s, v.y - s, v.z - s);
	}

	static inline vec3 operator-(vec3 v, float s)
	{
		return vec3(v.x - s, v.y - s, v.z - s);
	}

	static inline vec3 operator-(vec3 v, vec3 w)
	{
		return vec3(v.x - w.x, v.y - w.y, v.z - w.z);
	}

	static inline vec3 operator*(float s, vec3 v)
	{
		return vec3(v.x * s, v.y * s, v.z * s);
	}

	static inline vec3 operator*(vec3 v, float s)
	{
		return vec3(v.x * s, v.y * s, v.z * s);
	}

	static inline vec3 operator*(vec3 v, vec3 w)
	{
		return vec3(v.x * w.x, v.y * w.y, v.z * w.z);
	}

	static inline vec3 operator/(vec3 v, float s)
	{
		return vec3(v.x / s, v.y / s, v.z / s);
	}

	static inline vec3 operator/(float s, vec3 v)
	{
		return vec3(s / v.x, s / v.y, s / v.z);
	}

	static inline vec3 operator/(vec3 v, vec3 w)
	{
		return vec3(v.x / w.x, v.y / w.y, v.z / w.z);
	}

	static inline vec3 operator-(vec3 v)
	{
		return vec3(-v.x, -v.y, -v.z);
	}

	struct quat
	{
		quat() :
			w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
		quat(float _w, float _x, float _y, float _z) :
			w(_w), x(_x), y(_y), z(_z) {}

		float w, x, y, z;
	};

	static inline quat operator*(quat q, float s)
	{
		return quat(q.w * s, q.x * s, q.y * s, q.z * s);
	}

	static inline quat operator*(float s, quat q)
	{
		return quat(q.w * s, q.x * s, q.y * s, q.z * s);
	}

	static inline quat operator+(quat q, quat p)
	{
		return quat(q.w + p.w, q.x + p.x, q.y + p.y, q.z + p.z);
	}

	static inline quat operator-(quat q, quat p)
	{
		return quat(q.w - p.w, q.x - p.x, q.y - p.y, q.z - p.z);
	}

	static inline quat operator/(quat q, float s)
	{
		return quat(q.w / s, q.x / s, q.y / s, q.z / s);
	}

	static inline quat operator-(quat q)
	{
		return quat(-q.w, -q.x, -q.y, -q.z);
	}

	static inline float quat_length(quat q)
	{
		return sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
	}

	static inline quat quat_normalize(quat q, const float eps = 1e-8f)
	{
		return q / (quat_length(q) + eps);
	}

	static inline quat quat_inv(quat q)
	{
		return quat(-q.w, q.x, q.y, q.z);
	}

	static inline quat quat_mul(quat q, quat p)
	{
		return quat(
			p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z,
			p.w * q.x + p.x * q.w - p.y * q.z + p.z * q.y,
			p.w * q.y + p.x * q.z + p.y * q.w - p.z * q.x,
			p.w * q.z - p.x * q.y + p.y * q.x + p.z * q.w);
	}

	static inline quat quat_inv_mul(quat q, quat p)
	{
		return quat_mul(quat_inv(q), p);
	}

	static inline quat quat_mul_inv(quat q, quat p)
	{
		return quat_mul(q, quat_inv(p));
	}

	static inline quat quat_abs(quat x)
	{
		return x.w < 0.0 ? -x : x;
	}

	static inline quat quat_exp(vec3 v, float = 1e-8f)
	{
		float halfangle = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

		if (halfangle == 0.0f) {
			return quat_normalize(quat(1.0f, halfangle, halfangle, halfangle));
		} else {
			float c = cosf(halfangle);
			float s = sinf(halfangle) / halfangle;
			return quat(c, s * v.x, s * v.y, s * v.z);
		}
	}

	static inline vec3 quat_log(quat q, float = 1e-8f)
	{
		float length = sqrtf(1.0f - (q.w * q.w));

		if (q.w >= 1.0f) {
			return vec3(0.0f, 0.0f, 0.0f);
		} else {
			float halfangle = acosf(clampf(q.w, -1.0f, 1.0f));
			return halfangle * (vec3(q.x, q.y, q.z) / length);
		}
	}

	static inline quat quat_from_scaled_angle_axis(vec3 v, float eps = 1e-8f)
	{
		return quat_exp(v / 5.0f, eps);
	}

	static inline vec3 quat_to_scaled_angle_axis(quat q, float eps = 1e-8f)
	{
		return 5.0f * quat_log(q, eps);
	}

	static inline vec3 quat_differentiate_angular_velocity(
		quat next, quat curr, float dt, float eps = 1e-8f)
	{
		return quat_to_scaled_angle_axis(
					quat_abs(quat_mul(next, quat_inv(curr))), eps) /
			    dt;
	}

	static inline quat quat_from_angle_axis(float angle, vec3 axis)
	{
		float c = cosf(angle / 2.0f);
		float s = sinf(angle / 2.0f);
		return quat(c, s * axis.x, s * axis.y, s * axis.z);
	}

	static inline void quat_to_angle_axis(quat q, float& angle, vec3& axis, float eps = 1e-8f)
	{
		float length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z);

		if (length < eps) {
			angle = 0.0f;
			axis = vec3(1.0f, 0.0f, 0.0f);
		} else {
			angle = 2.0f * acosf(clampf(q.w, -1.0f, 1.0f));
			axis = vec3(q.x, q.y, q.z) / length;
		}
	}

	void quat_hermite(
		quat& rot,
		float x,
		quat r0,
		quat r1,
		vec3 v0,
		vec3 v1)
	{
		float w1 = 3 * x * x - 2 * x * x * x;
		float w2 = x * x * x - 2 * x * x + x;
		float w3 = x * x * x - x * x;

		vec3 r1_sub_r0 = quat_to_scaled_angle_axis(quat_abs(quat_mul_inv(r1, r0)));
		rot = quat_normalize(quat_mul(quat_from_scaled_angle_axis(w1 * r1_sub_r0 + w2 * v0 + w3 * v1), r0));
	}

	void quat_catmull_rom_velocity(
		vec3& v1,
		vec3& v2,
		quat r0,
		quat r1,
		quat r2,
		quat r3)
	{
		vec3 r1_sub_r0 = quat_to_scaled_angle_axis(quat_abs(quat_mul_inv(r1, r0)));
		vec3 r2_sub_r1 = quat_to_scaled_angle_axis(quat_abs(quat_mul_inv(r2, r1)));
		vec3 r3_sub_r2 = quat_to_scaled_angle_axis(quat_abs(quat_mul_inv(r3, r2)));

		v1 = (r1_sub_r0 + r2_sub_r1) / 2;
		v2 = (r2_sub_r1 + r3_sub_r2) / 2;
	}
}
