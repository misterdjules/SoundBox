#ifndef AUDIOFORMATS_H_
#define AUDIOFORMATS_H_

/**
 * AudioInfo instances store informations related to the actual sound data associated to 
 * instances of AClip, such as:
 *  - The sample rate
 *  - The number of bits per sample
 *  - The number of channels.
 * 
 */
struct AudioInfo
{
    unsigned int    m_SampleRate;
    unsigned short  m_BitsPerSample;
    unsigned short  m_NumChannels;
    unsigned int    m_NbSamples;

    AudioInfo() 
        :   m_SampleRate(0), 
            m_BitsPerSample(0),
            m_NumChannels(0),
            m_NbSamples(0)
    {}

	// Returns true if the data contained in an AudioInfo instance is consistent, false otherwise.
	// For instance, it would return false if m_BitsPerSample was set to an unsupported value, or 
	// if m_NumChannels was 0.
    static bool CheckAudioInfo(const AudioInfo&);
};

#endif // AUDIOFORMATS_H_