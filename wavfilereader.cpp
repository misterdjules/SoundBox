#include <fstream>
#include <iostream>

#include "wavfilereader.h"
#include "audioformats.h"

#define WAV_FORMAT_CODE_IEEE_FLOAT 0x0003

bool WavFileReader::ReadFormat(std::istream& inputStream, AudioInfo& outAudioInfo)
{   
    if (!CheckFirstFormatBlock(inputStream))
    {
        return false;
    }
    
    if (!CheckAudioFormatBlock(inputStream, outAudioInfo))
    {
        return false;
    }

    return true;
}

bool WavFileReader::ReadSamples(std::istream& inputStream,  const AudioInfo& audioInfo, unsigned int nbSamplesToRead, float* outSamples, unsigned int& outNbSamplesRead)
{
    if (!AudioInfo::CheckAudioInfo(audioInfo))
	{
		return false;
	}
	
	inputStream.read(reinterpret_cast<char*>(outSamples), nbSamplesToRead * (audioInfo.m_BitsPerSample / 8));
		
	if (!inputStream && (inputStream.gcount() / (audioInfo.m_BitsPerSample / 8)) != audioInfo.m_NbSamples)
	{		
		return false;
	}

	outNbSamplesRead = nbSamplesToRead;
	return true;
}

bool WavFileReader::CheckFirstFormatBlock(std::istream& inputStream)
{
    if (!inputStream)
    {
        return false;
    }

    // First, check for first 4 bytes "RIFF" (0x52, 0x49, 0x46, 0x46) in the input file
    char riffBuff[5];
    inputStream.read(riffBuff, 4);
    riffBuff[4] = '\0';
    if (strcmp(riffBuff, "RIFF"))
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    // Then get the file size
    unsigned int fileSize = 0;
    inputStream.read(reinterpret_cast<char*>(&fileSize), 4);

    if (!inputStream)
    {
        return false;
    }

    // Check the "WAVE" format
    char fromatIDBuff[5];
    inputStream.read(fromatIDBuff, 4);
    fromatIDBuff[4] = '\0';
    if (strcmp(fromatIDBuff, "WAVE"))
    {
        return false;
    }

    return true;
}

bool WavFileReader::CheckAudioFormatBlock(std::istream& inputStream, AudioInfo& outAudioInfo)
{
    if (!inputStream)
    {
        return false;
    }

    char fromatBlockIDBuff[5];
    inputStream.read(fromatBlockIDBuff, 4);
    fromatBlockIDBuff[4] = '\0';
    if (strcmp(fromatBlockIDBuff, "fmt "))
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    unsigned int blockSize = 0;
    inputStream.read(reinterpret_cast<char*>(&blockSize), 4);
    if (blockSize != 0x10)
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }


    unsigned short audioFormat = 0;
    inputStream.read(reinterpret_cast<char*>(&audioFormat), 2);
    if (audioFormat != WAV_FORMAT_CODE_IEEE_FLOAT)
    {
        // we currently support only floating point samples in our 
        // beat detection code
        return false;
    }


    if (!inputStream)
    {
        return false;
    }

    inputStream.read(reinterpret_cast<char*>(&outAudioInfo.m_NumChannels), 2);
    if (outAudioInfo.m_NumChannels == 0)
    {
        return false;
    }    

    if (!inputStream)
    {
        return false;
    }
    
    inputStream.read(reinterpret_cast<char*>(&outAudioInfo.m_SampleRate), 4);
    if (outAudioInfo.m_SampleRate == 0)
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    unsigned int bytesPerSec = 0;
    inputStream.read(reinterpret_cast<char*>(&bytesPerSec), 4);
    if (bytesPerSec == 0)
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    unsigned short bytesPerBlock = 0;
    inputStream.read(reinterpret_cast<char*>(&bytesPerBlock), 2);
    if (bytesPerBlock == 0)
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }
    
    inputStream.read(reinterpret_cast<char*>(&outAudioInfo.m_BitsPerSample), 2);
    if (outAudioInfo.m_BitsPerSample    != 8    && 
        outAudioInfo.m_BitsPerSample    != 16   && 
        outAudioInfo.m_BitsPerSample    != 24   && 
        outAudioInfo.m_BitsPerSample    != 32)
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }


    char factIDBuff[5];
    inputStream.read(factIDBuff, 4);
    factIDBuff[4] = '\0';
    if (strcmp(factIDBuff, "fact"))
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    unsigned int chunkSize = 0;
    inputStream.read(reinterpret_cast<char*>(&chunkSize), 4);
    if (chunkSize < 4)
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    unsigned int nbSamplesPerChannel = 0;
    inputStream.read(reinterpret_cast<char*>(&nbSamplesPerChannel), 4);
    if (nbSamplesPerChannel == 0)
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    // Skipping bytes until we get to the start of the "data" ID,
    // where data starts. We don't care about the rest of the metadata for now
    char c = inputStream.peek();
    while (inputStream && c != 'd')
    {
        inputStream.ignore();
        c = inputStream.peek();        
    }    
    
    if (!inputStream)
    {
        return false;
    }    
    
    char dataIDBuff[5];
    inputStream.read(dataIDBuff, 4);
    dataIDBuff[4] = '\0';
    if (strcmp(dataIDBuff, "data"))
    {
        return false;
    }

    if (!inputStream)
    {
        return false;
    }

    unsigned int dataSize = 0;
    inputStream.read(reinterpret_cast<char*>(&dataSize), 4);
    if (dataSize == 0)
    {
        return false;
    }

    outAudioInfo.m_NbSamples = dataSize / (outAudioInfo.m_NumChannels * (outAudioInfo.m_BitsPerSample / 8));

    return true;
}