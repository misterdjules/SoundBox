#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>

#include "audioconfig.h"
#include "simplepeakdetector.h"
#include "Clip.h"

#define FREQ_LP_BEAT		150.0f								// Low Pass filter frequency, in HZ
#define T_FILTER			1.0f / (2.0f * M_PI * FREQ_LP_BEAT)	// Low Pass filter time constant
#define BEAT_RELEASE_TIME	0.2f								// Release time of envelope detector, in second

SimplePeakDetector::SimplePeakDetector()
{
	Reset();
}

void SimplePeakDetector::Reset()
{
	m_Filter1Out		= .0;
    m_Filter2Out		= .0;
    m_EnvelopePeak		= .0;
    m_PeakTrigger		= false;
    m_PrevPeakPulse		= false;

	m_PeakFilter = 1.0 / (DEFAULT_SAMPLE_RATE * T_FILTER);
    m_PeakRelease = exp(-1.0f / (DEFAULT_SAMPLE_RATE * BEAT_RELEASE_TIME));
}

void SimplePeakDetector::ProcessAudio(const float* inputSamples, unsigned int nbSamples, std::vector<Peak>& outPeaks)
{
    assert(nbSamples == INPUT_WINDOW_SIZE);
	
	Reset();

	for (unsigned int sampleIndex = 0; sampleIndex < nbSamples; ++sampleIndex)
	{
		double envelopeIn;

		// Step 1 : 2nd order low pass filter (made of two 1st order RC filter)
		m_Filter1Out = m_Filter1Out + (m_PeakFilter * (inputSamples[sampleIndex] - m_Filter1Out));
		m_Filter2Out = m_Filter2Out + (m_PeakFilter * (m_Filter1Out - m_Filter2Out));

		// Step 2 : peak detector
		envelopeIn = fabs(m_Filter2Out);
		if (envelopeIn > m_EnvelopePeak) 
		{
			m_EnvelopePeak = envelopeIn; // Attack time = 0
		}
		else
		{
			m_EnvelopePeak *= m_PeakRelease;
			m_EnvelopePeak += (1.0 - m_PeakRelease) * envelopeIn;
		}

		// Step 3 : Schmitt trigger
		if (!m_PeakTrigger)
		{
			if (m_EnvelopePeak > .5)
			{
				m_PeakTrigger = true;
			}
		}
		else
		{
			if (m_EnvelopePeak < .3) 
			{
				m_PeakTrigger = false;
			}
		}

		// Step 4 : rising edge detector		
		if ((m_PeakTrigger) && (!m_PrevPeakPulse))
		{			
			outPeaks.push_back(Peak(sampleIndex, sampleIndex));
		}

		m_PrevPeakPulse = m_PeakTrigger;
	}
}

bool SimplePeakDetector::GetPeaks(const float* samples, unsigned int nbSamples, const AudioInfo& audioInfo, std::vector<Peak>& outPeaks)
{    
    if (!AudioInfo::CheckAudioInfo(audioInfo))
    {
        return false;
    }

    if (nbSamples != INPUT_WINDOW_SIZE)
	{
		return false;
	}

    ProcessAudio(samples, nbSamples, outPeaks);    

    return true;
}