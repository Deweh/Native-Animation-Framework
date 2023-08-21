#pragma once
#include <cmath>

#ifndef PI
#	define PI 3.1415926545
#endif

#ifndef SHORT_PI
#	define SHORT_PI 3.1415926545f
#endif

#define RETURN_EASE_FUNCTION(func, e) \
	case Function::e: \
	return func(t);

namespace Easing
{
	enum Function : uint8_t
	{
		None,
		InSine,
		OutSine,
		InOutSine,
		InQuad,
		OutQuad,
		InOutQuad,
		InCubic,
		OutCubic,
		InOutCubic,
		InQuart,
		OutQuart,
		InOutQuart,
		InQuint,
		OutQuint,
		InOutQuint,
		InExpo,
		OutExpo,
		InOutExpo,
		InCirc,
		OutCirc,
		InOutCirc,
		InBack,
		OutBack,
		InOutBack,
		InElastic,
		OutElastic,
		InOutElastic,
		InBounce,
		OutBounce,
		InOutBounce
	};

	double easeNone(double t) {
		return t;
	}

	double easeInSine(double t)
	{
		return sin(1.5707963 * t);
	}

	double easeOutSine(double t)
	{
		return 1 + sin(1.5707963 * (--t));
	}

	double easeInOutSine(double t)
	{
		return 0.5 * (1 + sin(3.1415926 * (t - 0.5)));
	}

	double easeInQuad(double t)
	{
		return t * t;
	}

	double easeOutQuad(double t)
	{
		return t * (2 - t);
	}

	double easeInOutQuad(double t)
	{
		return t < 0.5 ? 2 * t * t : t * (4 - 2 * t) - 1;
	}

	double easeInCubic(double t)
	{
		return t * t * t;
	}

	double easeOutCubic(double t)
	{
		return 1 + (--t) * t * t;
	}

	double easeInOutCubic(double t)
	{
		return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2;
	}

	double easeInQuart(double t)
	{
		t *= t;
		return t * t;
	}

	double easeOutQuart(double t)
	{
		t = (--t) * t;
		return 1 - t * t;
	}

	double easeInOutQuart(double t)
	{
		if (t < 0.5) {
			t *= t;
			return 8 * t * t;
		} else {
			t = (--t) * t;
			return 1 - 8 * t * t;
		}
	}

	double easeInQuint(double t)
	{
		double t2 = t * t;
		return t * t2 * t2;
	}

	double easeOutQuint(double t)
	{
		double t2 = (--t) * t;
		return 1 + t * t2 * t2;
	}

	double easeInOutQuint(double t)
	{
		double t2;
		if (t < 0.5) {
			t2 = t * t;
			return 16 * t * t2 * t2;
		} else {
			t2 = (--t) * t;
			return 1 + 16 * t * t2 * t2;
		}
	}

	double easeInExpo(double t)
	{
		return (pow(2, 8 * t) - 1) / 255;
	}

	double easeOutExpo(double t)
	{
		return 1 - pow(2, -8 * t);
	}

	double easeInOutExpo(double t)
	{
		if (t < 0.5) {
			return (pow(2, 16 * t) - 1) / 510;
		} else {
			return 1 - 0.5 * pow(2, -16 * (t - 0.5));
		}
	}

	double easeInCirc(double t)
	{
		return 1 - sqrt(1 - t);
	}

	double easeOutCirc(double t)
	{
		return sqrt(t);
	}

	double easeInOutCirc(double t)
	{
		if (t < 0.5) {
			return (1 - sqrt(1 - 2 * t)) * 0.5;
		} else {
			return (1 + sqrt(2 * t - 1)) * 0.5;
		}
	}

	double easeInBack(double t)
	{
		return t * t * (2.70158 * t - 1.70158);
	}

	double easeOutBack(double t)
	{
		return 1 + (--t) * t * (2.70158 * t + 1.70158);
	}

	double easeInOutBack(double t)
	{
		if (t < 0.5) {
			return t * t * (7 * t - 2.5) * 2;
		} else {
			return 1 + (--t) * t * 2 * (7 * t + 2.5);
		}
	}

	double easeInElastic(double t)
	{
		double t2 = t * t;
		return t2 * t2 * sin(t * PI * 4.5);
	}

	double easeOutElastic(double t)
	{
		double t2 = (t - 1) * (t - 1);
		return 1 - t2 * t2 * cos(t * PI * 4.5);
	}

	double easeInOutElastic(double t)
	{
		double t2;
		if (t < 0.45) {
			t2 = t * t;
			return 8 * t2 * t2 * sin(t * PI * 9);
		} else if (t < 0.55) {
			return 0.5 + 0.75 * sin(t * PI * 4);
		} else {
			t2 = (t - 1) * (t - 1);
			return 1 - 8 * t2 * t2 * sin(t * PI * 9);
		}
	}

	double easeInBounce(double t)
	{
		return pow(2, 6 * (t - 1)) * abs(sin(t * PI * 3.5));
	}

	double easeOutBounce(double t)
	{
		return 1 - pow(2, -6 * t) * abs(cos(t * PI * 3.5));
	}

	double easeInOutBounce(double t)
	{
		if (t < 0.5) {
			return 8 * pow(2, 8 * (t - 1)) * abs(sin(t * PI * 7));
		} else {
			return 1 - 8 * pow(2, -8 * t) * abs(sin(t * PI * 7));
		}
	}

	double Ease(double t, Function f) {
		switch (f) {
			RETURN_EASE_FUNCTION(easeNone, None);
			RETURN_EASE_FUNCTION(easeInSine, InSine);
			RETURN_EASE_FUNCTION(easeOutSine, OutSine);
			RETURN_EASE_FUNCTION(easeInOutSine, InOutSine);
			RETURN_EASE_FUNCTION(easeInQuad, InQuad);
			RETURN_EASE_FUNCTION(easeOutQuad, OutQuad);
			RETURN_EASE_FUNCTION(easeInOutQuad, InOutQuad);
			RETURN_EASE_FUNCTION(easeInCubic, InCubic);
			RETURN_EASE_FUNCTION(easeOutCubic, OutCubic);
			RETURN_EASE_FUNCTION(easeInOutCubic, InOutCubic);
			RETURN_EASE_FUNCTION(easeInQuart, InQuart);
			RETURN_EASE_FUNCTION(easeOutQuart, OutQuart);
			RETURN_EASE_FUNCTION(easeInOutQuart, InOutQuart);
			RETURN_EASE_FUNCTION(easeInQuint, InQuint);
			RETURN_EASE_FUNCTION(easeOutQuint, OutQuint);
			RETURN_EASE_FUNCTION(easeInOutQuint, InOutQuint);
			RETURN_EASE_FUNCTION(easeInExpo, InExpo);
			RETURN_EASE_FUNCTION(easeOutExpo, OutExpo);
			RETURN_EASE_FUNCTION(easeInOutExpo, InOutExpo);
			RETURN_EASE_FUNCTION(easeInCirc, InCirc);
			RETURN_EASE_FUNCTION(easeOutCirc, OutCirc);
			RETURN_EASE_FUNCTION(easeInOutCirc, InOutCirc);
			RETURN_EASE_FUNCTION(easeInBack, InBack);
			RETURN_EASE_FUNCTION(easeOutBack, OutBack);
			RETURN_EASE_FUNCTION(easeInOutBack, InOutBack);
			RETURN_EASE_FUNCTION(easeInElastic, InElastic);
			RETURN_EASE_FUNCTION(easeOutElastic, OutElastic);
			RETURN_EASE_FUNCTION(easeInOutElastic, InOutElastic);
			RETURN_EASE_FUNCTION(easeInBounce, InBounce);
			RETURN_EASE_FUNCTION(easeOutBounce, OutBounce);
			RETURN_EASE_FUNCTION(easeInOutBounce, InOutBounce);
		default:
			return t;
		}
	}
}
