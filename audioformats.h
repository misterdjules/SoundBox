#ifndef AUDIOFORMATS_H_
#define AUDIOFORMATS_H_

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

    static bool CheckAudioInfo(const AudioInfo&);
};

#endif // AUDIOFORMATS_H_