#ifndef WAVFILEREADER_H_
#define WAVFILEREADER_H_

#include <string>
#include <istream>
#include <vector>

#include "audioformats.h"

class WavFileReader
{

private:
    static bool CheckFirstFormatBlock(std::istream& inputFileStream);
    static bool CheckAudioFormatBlock(std::istream& inputFileStream, AudioInfo& outAudioInfo);

public:   
    static bool ReadFormat(std::istream& inputStream, AudioInfo& outAudioInfo);
	static bool ReadSamples(std::istream& inputStream,  const AudioInfo& audioInfo, unsigned int nbSamplesToRead, float* outSamples, unsigned int& outNbSamplesRead);
}; 

#endif // WAVFILEREADER_H_