#include <cmath>
#include <limits>

class MathUtils
{
public:
	template <typename _FloatingPointType>
	static bool AlmostEqualWithTolerance(_FloatingPointType A, _FloatingPointType B, _FloatingPointType maxRelativeError, _FloatingPointType maxAbsoluteError)
	{
		// Check regarding absolute error first
		if (fabs(A - B) < maxAbsoluteError)
		{
			return true;		
		}

		// Then check regarding relative error
		_FloatingPointType relativeError;
		if (fabs(A) > fabs(B))
		{
			relativeError = fabs((A - B) / A);
		}
		else
		{
			relativeError = fabs((A - B) / B);
		}

		if (relativeError < maxRelativeError)
		{
			return true;
		}

		return false;
	}
	
	template <typename _NumType> 
	static _NumType LinearMap(	_NumType value, 
								_NumType originalRangeLowBound, 
								_NumType originalRangeHighBound, 
								_NumType targetRangeLowBound, 
								_NumType targetRangeHighBound)
	{
		_NumType divisor = originalRangeHighBound - originalRangeLowBound;
		if (!divisor)
		{
			return 0;
		}

		_NumType num = (value - originalRangeLowBound) * (targetRangeHighBound - targetRangeLowBound);

		return targetRangeLowBound + (num / divisor);
	}

	static long Round(double value)
	{		
		if (value < LONG_MIN - 0.5)
		{
			return LONG_MIN;
		}

		if (value > LONG_MAX)
		{
			return LONG_MAX;
		}

		if (value - std::floor(value) < 0.5)
		{
			return static_cast<long>(std::floor(value));
		}

		return static_cast<long>(std::ceil(value));
	}

	static bool IsValidTime(double timeValue)
	{
		return	timeValue	!=	std::numeric_limits<double>::infinity()			&&
				timeValue	!=	std::numeric_limits<double>::quiet_NaN()		&&
				timeValue	!=	std::numeric_limits<double>::signaling_NaN()	&&
				timeValue	!=	std::numeric_limits<double>::denorm_min();
	}
};