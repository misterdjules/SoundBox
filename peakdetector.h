#ifndef PEAKDETECTOR_H_
#define PEAKDETECTOR_H_

#include <vector>

#include "audioformats.h"
#include "soundfeatures.h"

class PeakDetector
{
public:
    virtual bool GetPeaks(const std::vector<double>& samples, const AudioInfo& audioInfo, std::vector<Peak>& outPeaks) = 0;    
};

#endif // PEAKDETECTOR_H_