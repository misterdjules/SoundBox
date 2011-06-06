#ifndef SIMPLEPEAKDETECTOR_H
#define SIMPLEPEAKDETECTOR_H

#include <vector>

#include "peakdetector.h"

class Peak;

class SimplePeakDetector : public PeakDetector
{
private:
    double  m_KBeatFilter;              // Filter coefficient
    double  m_Filter1Out, m_Filter2Out;
    double  m_BeatRelease;              // Release time coefficient
    double  m_PeakEnv;                  // Peak enveloppe follower
    bool    m_BeatTrigger;              // Schmitt trigger output
    bool    m_PrevBeatPulse;            // Rising edge memory
    
    bool    m_BeatPulse;                // Beat detector output
    
    bool            GetBeatPulse() const { return m_BeatPulse; }
    virtual void    ProcessAudio(double input);
    void            SetSampleRate(unsigned int sampleRate);    

public:
    SimplePeakDetector();
        
    virtual bool GetPeaks(const std::vector<double>& samples, const AudioInfo& audioInfo, std::vector<Peak>& outPeaks);    
};

#endif // SIMPLEPEAKDETECTOR_H