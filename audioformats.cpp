#include "audioformats.h"

bool AudioInfo::CheckAudioInfo(const AudioInfo& audioInfo)
{
    if (audioInfo.m_SampleRate == 0)
    {
        return false;
    }

    return true;
}