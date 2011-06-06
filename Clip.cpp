//****************************************************************************************
// File:    Clip.cpp
//
// Author:  Julien Gilli
//****************************************************************************************
#include <fstream>
#include <iostream>

#include "Clip.h"
#include "simplepeakdetector.h"
#include "wavfilereader.h"
#include "mathutils.h"

double WarpMarker::SampleTimeSelector(const WarpMarker& warpMarker)
{
	return warpMarker.GetSampleTime();
}

double WarpMarker::BeatTimeSelector(const WarpMarker& warpMarker)
{
	return warpMarker.GetBeatTime();
}

bool AClip::AddDefaultWarpMarkers()
{
    // First default warp marker at clip's first sample
    m_WarpMarkers.push_back(WarpMarker(0.0, 0.0));
    
    // Second default warp marker at end of clip
    if (!m_Samples.empty() && m_AudioInfo.m_SampleRate)
    {
        double duration = m_Samples.size() / m_AudioInfo.m_SampleRate;
        m_WarpMarkers.push_back(WarpMarker(duration, duration));

		return true;
    }

	return false;
}

bool AClip::FindBoundingWarpMarkersForTime(double time, WarpMarker::TimeSelector timeSelector, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const
{    
    if (m_WarpMarkers.size() < 2)
    {
        // we need at least 2 bounding warp markers in the clip to find those
        // bounding the beat time at "beatTime"
        // They should have been created after loading the audio data by calling AClip::AddDefaultWarpMarkers()
        return false;
    }

    lowBoundMarker = m_WarpMarkers.front();
    highBoundMarker = m_WarpMarkers.back();

    std::vector<WarpMarker>::const_iterator itWarpMakers = m_WarpMarkers.begin();
    std::vector<WarpMarker>::const_iterator itWarpMakersEnd = m_WarpMarkers.end();
    for (; itWarpMakers != itWarpMakersEnd; ++itWarpMakers)
    {
        double currentWarpMarkerTime = timeSelector(*itWarpMakers);
        if ((currentWarpMarkerTime < time || AlmostEqualWithTolerance(currentWarpMarkerTime, time, TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)) &&
			currentWarpMarkerTime > timeSelector(lowBoundMarker))
        {
            lowBoundMarker = *itWarpMakers;
			continue;
        }

        if ((currentWarpMarkerTime > time || AlmostEqualWithTolerance(currentWarpMarkerTime, time, TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE))&&
			currentWarpMarkerTime < timeSelector(highBoundMarker))
        {
            highBoundMarker = *itWarpMakers;
        }
    }

    return true;
}

bool AClip::FindBoundingWarpMarkersForSampleTime(double sampleTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const
{
	return FindBoundingWarpMarkersForTime(sampleTime, WarpMarker::SampleTimeSelector, lowBoundMarker, highBoundMarker);
}

bool AClip::FindBoundingWarpMarkersForBeatTime(double beatTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const
{
	return FindBoundingWarpMarkersForTime(beatTime, WarpMarker::BeatTimeSelector, lowBoundMarker, highBoundMarker);
}

//----------------------------------------------------------------------------------------

double AClip::BeatToSampleTime(double BeatTime)
{
    WarpMarker lowBoundMarker, highBoundMarker;
    bool foundBoundingMarkers = FindBoundingWarpMarkersForBeatTime(BeatTime, lowBoundMarker, highBoundMarker);
    if (foundBoundingMarkers)
    {
        double linearMappedSampleTime = LinearMap(BeatTime, lowBoundMarker.GetBeatTime(), highBoundMarker.GetBeatTime(), lowBoundMarker.GetSampleTime(), highBoundMarker.GetSampleTime());
        return linearMappedSampleTime;
    }

    return 0.0;
}


//----------------------------------------------------------------------------------------

double AClip::SampleToBeatTime(double SampleTime)
{
	WarpMarker lowBoundMarker, highBoundMarker;
    bool foundBoundingMarkers = FindBoundingWarpMarkersForSampleTime(SampleTime, lowBoundMarker, highBoundMarker);
    if (foundBoundingMarkers)
    {
		double linearMappedBeatTime = LinearMap(SampleTime, lowBoundMarker.GetSampleTime(), highBoundMarker.GetSampleTime(), lowBoundMarker.GetBeatTime(), highBoundMarker.GetBeatTime());
        return linearMappedBeatTime;
    }

    return 0.0;
}

bool AClip::LoadDataFromFile(const std::string& filePath)
{            
    if (!(filePath.substr(filePath.length() - 5, 4).compare(std::string(".wav"))))
    {
        std::cerr << ".wav extension missing, loading the file as Wav file anyway..." << std::endl;
        return false;
    }

    std::ifstream wavInputStream(filePath.c_str(), std::ifstream::in | std::ios::binary);
    if (!wavInputStream)
    {
        return false;
    }
    
    bool wavFormatOk = WavFileReader::ReadFormat(wavInputStream, m_AudioInfo);
    if (!wavFormatOk)
    {
        return false;
    }

    if (m_AudioInfo.m_NumChannels > 1)
    {
        std::cerr << "More than one channel is not supported at this time." << std::endl;
    }    

    bool samplesReadSuccessfuly = WavFileReader::ReadSamples(wavInputStream, m_AudioInfo, m_Samples);
    
    wavInputStream.close();

	AddDefaultWarpMarkers();

    return samplesReadSuccessfuly;
}

void AClip::Analyze()
{
       
}

bool AClip::GetBPM(unsigned int& bpmCount) const
{
    bpmCount = 0;
    if (!m_PeakDetector)
    {
        return false;
    }

    if (m_BPMCached != BPM_CACHE_INVALID)
    {
        bpmCount = m_BPMCached;
		return true;
    }

    if (m_AudioInfo.m_NbSamples)
    {
        std::vector<Peak>   peaks;
        m_PeakDetector->GetPeaks(m_Samples, m_AudioInfo, peaks);
    }

    return false;
}


//****************************************************************************************
//                                       E O F
//****************************************************************************************




