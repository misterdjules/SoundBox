#define _USE_MATH_DEFINES
#include <cmath>

#include "simplepeakdetector.h"
#include "Clip.h"

#define FREQ_LP_BEAT 150.0f                             // Low Pass filter frequency, in HZ
#define T_FILTER 1.0f / (2.0f * M_PI * FREQ_LP_BEAT)    // Low Pass filter time constant
#define BEAT_RELEASE_TIME 0.1f                          // Release time of envelope detector, in second

SimplePeakDetector::SimplePeakDetector()
{
    m_Filter1Out  = .0;
    m_Filter2Out  = .0;
    m_PeakEnv     = .0;
    m_BeatTrigger = false;
    m_PrevBeatPulse = false;
    
    SetSampleRate(44100);
}

void SimplePeakDetector::SetSampleRate(unsigned int sampleRate)
{
    m_KBeatFilter = 1.0 / (sampleRate * T_FILTER);
    m_BeatRelease = exp(-1.0f / (sampleRate * BEAT_RELEASE_TIME));
}

void SimplePeakDetector::ProcessAudio(double input)
{
    double envIn;

    // Step 1 : 2nd order low pass filter (made of two 1st order RC filter)
    m_Filter1Out = m_Filter1Out + (m_KBeatFilter * (input - m_Filter1Out));
    m_Filter2Out = m_Filter2Out + (m_KBeatFilter * (m_Filter1Out - m_Filter2Out));

    // Step 2 : peak detector
    envIn = fabs(m_Filter2Out);
    if (envIn > m_PeakEnv) 
    {
        m_PeakEnv = envIn; // Attack time = 0
    }
    else
    {
        m_PeakEnv *= m_BeatRelease;
        m_PeakEnv += (1.0 - m_BeatRelease) * envIn;
    }

    // Step 3 : Schmitt trigger
    if (!m_BeatTrigger)
    {
        if (m_PeakEnv > .5)
        {
            m_BeatTrigger=true;
        }
    }
    else
    {
        if (m_PeakEnv < .15) 
        {
            m_BeatTrigger = false;
        }
    }

    // Step 4 : rising edge detector
    m_BeatPulse = false;
    if ((m_BeatTrigger) && (!m_PrevBeatPulse))
    {
        m_BeatPulse = true;
    }
    
    m_PrevBeatPulse = m_BeatTrigger;
}

bool SimplePeakDetector::GetPeaks(const std::vector<double>& samples, const AudioInfo& audioInfo, std::vector<Peak>& outPeaks)
{    
    if (!AudioInfo::CheckAudioInfo(audioInfo))
    {
        return false;
    }

    std::vector<double>::const_iterator itSamples = samples.begin();
    std::vector<double>::const_iterator itSamplesEnd = samples.end();
    
    for (unsigned int currentSampleIndex = 0; itSamples != itSamplesEnd; ++itSamples, ++currentSampleIndex)
    {
        ProcessAudio(*itSamples);
        if (GetBeatPulse())
        {
            Peak peak(currentSampleIndex, static_cast<double>(currentSampleIndex) / audioInfo.m_SampleRate);
            outPeaks.push_back(peak);
        }
    }

    return true;
}