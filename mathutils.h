#define TIME_RELATIVE_TOLERANCE 1.0 / 44100
#define TIME_ABSOLUTE_TOLERANCE 1.0 / 44100

#include <cmath>

template <typename _FloatingPointType>
bool AlmostEqualWithTolerance(_FloatingPointType A, _FloatingPointType B, _FloatingPointType maxRelativeError, _FloatingPointType maxAbsoluteError)
{
	if (fabs(A - B) < maxAbsoluteError)
	{
		return true;		
	}
	
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
_NumType LinearMap(_NumType value, _NumType originalRangeLowBound, 
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