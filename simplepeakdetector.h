#ifndef SIMPLEPEAKDETECTOR_H
#define SIMPLEPEAKDETECTOR_H

#include <vector>

#include "peakdetector.h"

class Peak;

class SimplePeakDetector : public PeakDetector
{
private:
    double  m_PeakFilter;				// Filter coefficient
    double  m_Filter1Out, m_Filter2Out;
    double  m_PeakRelease;              // Release time coefficient
    double  m_EnvelopePeak;             // Envelope follower
    bool    m_PeakTrigger;              // Schmitt trigger output
    bool    m_PrevPeakPulse;            // Rising edge memory
    
    bool    m_PeakPulse;                // Peak detector output
        
	void Reset();	

	virtual void    ProcessAudio(const float* inputSamples, unsigned int nbSamples, std::vector<Peak>& outPeaks);

public:
	
    static const unsigned int INPUT_WINDOW_SIZE		= 65536;
	static const unsigned int INPUT_WINDOW_OFFSET	= 4096;	

	SimplePeakDetector();
        
    virtual bool GetPeaks(const float* samples, unsigned int nbSamples, const AudioInfo& audioInfo, std::vector<Peak>& outPeaks);
};

#endif // SIMPLEPEAKDETECTOR_H