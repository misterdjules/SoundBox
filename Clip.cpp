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

#define TIME_RELATIVE_TOLERANCE (1.0 / (44100 * 10))
#define TIME_ABSOLUTE_TOLERANCE (1.0 / (44100 * 10))

double WarpMarker::SampleTimeSelector(const WarpMarker& warpMarker)
{
	return warpMarker.GetSampleTime();
}

double WarpMarker::BeatTimeSelector(const WarpMarker& warpMarker)
{
	return warpMarker.GetBeatTime();
}

bool WarpMarker::operator==(const WarpMarker& rhs) const
{
	if (this == &rhs)
	{
		return true;
	}
	
	return m_SampleTime == rhs.m_SampleTime && m_BeatTime == rhs.m_BeatTime;
}


bool AClip::ValidateWarpMarkerForAdd(const WarpMarker& warpMarkerToAdd)
{	
	double sampleTimeToAdd = warpMarkerToAdd.GetSampleTime();
	double beatTimeToAdd = warpMarkerToAdd.GetBeatTime();
	
	if (sampleTimeToAdd < 0.0 || 
		(!AlmostEqualWithTolerance(sampleTimeToAdd, GetDuration(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE) && sampleTimeToAdd > GetDuration()) || 
		beatTimeToAdd < 0.0)
	{
		return false;
	}

	WarpMarker lowBoundWarpMarkerSampleTime;
	WarpMarker highBoundWarpMarkerSampleTime;
	if (!FindBoundingWarpMarkersForSampleTime(warpMarkerToAdd.GetSampleTime(), lowBoundWarpMarkerSampleTime, highBoundWarpMarkerSampleTime))
	{
		return true;
	}

	WarpMarker lowBoundWarpMarkerBeatTime;
	WarpMarker highBoundWarpMarkerBeatTime;
	if (!FindBoundingWarpMarkersForBeatTime(warpMarkerToAdd.GetBeatTime(), lowBoundWarpMarkerBeatTime, highBoundWarpMarkerBeatTime))
	{
		return false;
	}

	if (lowBoundWarpMarkerBeatTime != lowBoundWarpMarkerSampleTime || highBoundWarpMarkerBeatTime != highBoundWarpMarkerSampleTime)
	{
		return false;
	}

	if (AlmostEqualWithTolerance(sampleTimeToAdd, lowBoundWarpMarkerSampleTime.GetSampleTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)	||
		AlmostEqualWithTolerance(sampleTimeToAdd, highBoundWarpMarkerSampleTime.GetSampleTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)	||
		AlmostEqualWithTolerance(beatTimeToAdd, lowBoundWarpMarkerSampleTime.GetBeatTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)		||
		AlmostEqualWithTolerance(beatTimeToAdd, lowBoundWarpMarkerSampleTime.GetBeatTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE))
	{
		return false;
	}

	return true;
}

bool AClip::AddDefaultWarpMarkers()
{    
	// First default warp marker at clip's first sample
    if (!AddWarpMarker(0.0, 0.0))
	{
		return false;
	}
    
    // Second default warp marker at end of clip
    if (!m_Samples.empty() && m_AudioInfo.m_SampleRate)
    {
        double duration = m_Samples.size() / m_AudioInfo.m_SampleRate;
        if (!AddWarpMarker(duration, duration))
		{
			return false;
		}

		return true;
    }

	return false;
}

bool AClip::GetFirstWarpMarker(WarpMarker& outFirstWarpMarker) const
{
	if (m_WarpMarkers.empty())
	{
		return false;
	}

	outFirstWarpMarker = *m_WarpMarkers.begin();
	double minSampleTime = outFirstWarpMarker.GetSampleTime();
	
	std::vector<WarpMarker>::const_iterator itWarpMarkers = m_WarpMarkers.begin();
	std::vector<WarpMarker>::const_iterator itWarpMarkersEnd = m_WarpMarkers.end();
	for (; itWarpMarkers != itWarpMarkersEnd; ++itWarpMarkers)
	{
		double currentWarpMarkerSampleTime = itWarpMarkers->GetSampleTime();
		if (currentWarpMarkerSampleTime < minSampleTime)
		{
			minSampleTime = currentWarpMarkerSampleTime;
			outFirstWarpMarker = *itWarpMarkers;
		}
	}

	return true;
}

bool AClip::GetLastWarpMarker(WarpMarker& outLastWarpMarker) const
{
	if (m_WarpMarkers.empty())
	{
		return false;
	}

	outLastWarpMarker = *m_WarpMarkers.begin();
	double maxSampleTime = outLastWarpMarker.GetSampleTime();
	
	std::vector<WarpMarker>::const_iterator itWarpMarkers = m_WarpMarkers.begin();
	std::vector<WarpMarker>::const_iterator itWarpMarkersEnd = m_WarpMarkers.end();
	for (; itWarpMarkers != itWarpMarkersEnd; ++itWarpMarkers)
	{
		double currentWarpMarkerSampleTime = itWarpMarkers->GetSampleTime();
		if (currentWarpMarkerSampleTime > maxSampleTime)
		{
			maxSampleTime = currentWarpMarkerSampleTime;
			outLastWarpMarker = *itWarpMarkers;
		}
	}

	return true;
}

bool AClip::AddWarpMarker(double sampleTime, double beatTime)
{
	if (!ValidateWarpMarkerForAdd(WarpMarker(sampleTime, beatTime)))
	{
		return false;
	}

	m_WarpMarkers.push_back(WarpMarker(sampleTime, beatTime)); 
	m_LowAndHighBoundWarpMarkersCacheIsValid = false;

	return true;
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

    if (!GetFirstWarpMarker(lowBoundMarker))
	{
		return false;
	}

    if (!GetLastWarpMarker(highBoundMarker))
	{
		return false;
	}

    std::vector<WarpMarker>::const_iterator itWarpMarkers = m_WarpMarkers.begin();
    std::vector<WarpMarker>::const_iterator itWarpMarkersEnd = m_WarpMarkers.end();
    for (; itWarpMarkers != itWarpMarkersEnd; ++itWarpMarkers)
    {
        double currentWarpMarkerTime = timeSelector(*itWarpMarkers);
        if ((currentWarpMarkerTime < time || AlmostEqualWithTolerance(currentWarpMarkerTime, time, TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)) &&
			currentWarpMarkerTime > timeSelector(lowBoundMarker))
        {
            lowBoundMarker = *itWarpMarkers;
			continue;
        }

        if (currentWarpMarkerTime > time && currentWarpMarkerTime < timeSelector(highBoundMarker))
        {
            highBoundMarker = *itWarpMarkers;
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
    bool foundBoundingMarkers = false;
	WarpMarker lowBoundMarker, highBoundMarker;

	if (m_LowAndHighBoundWarpMarkersCacheIsValid)
	{
		if ((BeatTime > m_CurrentCachedLowBoundWarpMarker.GetBeatTime() || AlmostEqualWithTolerance(BeatTime, m_CurrentCachedLowBoundWarpMarker.GetBeatTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)) &&
			BeatTime < m_CurrentCachedHighBoundWarpMarker.GetBeatTime())
		{
			lowBoundMarker = m_CurrentCachedLowBoundWarpMarker;
			highBoundMarker = m_CurrentCachedHighBoundWarpMarker;
			foundBoundingMarkers = true;
		}
	}

	if (!foundBoundingMarkers)
	{
		foundBoundingMarkers = FindBoundingWarpMarkersForBeatTime(BeatTime, lowBoundMarker, highBoundMarker);
	}

    if (foundBoundingMarkers)
    {        
		m_CurrentCachedLowBoundWarpMarker = lowBoundMarker;
		m_CurrentCachedHighBoundWarpMarker = highBoundMarker;
		m_LowAndHighBoundWarpMarkersCacheIsValid = true;

		double linearMappedSampleTime = LinearMap(	BeatTime, 
													lowBoundMarker.GetBeatTime(), highBoundMarker.GetBeatTime(), 
													lowBoundMarker.GetSampleTime(), highBoundMarker.GetSampleTime());
        return linearMappedSampleTime;
    }

    return 0.0;
}


//----------------------------------------------------------------------------------------

double AClip::SampleToBeatTime(double SampleTime)
{
	bool foundBoundingMarkers = false;
	WarpMarker lowBoundMarker, highBoundMarker;

	if (m_LowAndHighBoundWarpMarkersCacheIsValid)
	{
		if ((SampleTime > m_CurrentCachedLowBoundWarpMarker.GetSampleTime() || AlmostEqualWithTolerance(SampleTime, m_CurrentCachedLowBoundWarpMarker.GetSampleTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)) &&
			SampleTime < m_CurrentCachedHighBoundWarpMarker.GetSampleTime())
		{
			lowBoundMarker = m_CurrentCachedLowBoundWarpMarker;
			highBoundMarker = m_CurrentCachedHighBoundWarpMarker;
			foundBoundingMarkers = true;
		}
	}
	
	if (!foundBoundingMarkers)
	{		
		foundBoundingMarkers = FindBoundingWarpMarkersForSampleTime(SampleTime, lowBoundMarker, highBoundMarker);		
	}

	if (foundBoundingMarkers)
	{
		m_CurrentCachedLowBoundWarpMarker = lowBoundMarker;
		m_CurrentCachedHighBoundWarpMarker = highBoundMarker;
		m_LowAndHighBoundWarpMarkersCacheIsValid = true;

		double linearMappedBeatTime = LinearMap(SampleTime, 
			lowBoundMarker.GetSampleTime(), highBoundMarker.GetSampleTime(), 
			lowBoundMarker.GetBeatTime(), highBoundMarker.GetBeatTime());
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

bool AClip::ComputeBPM(const std::vector<Peak>& peaks, double& outBpmCount) const
{
	// Compute the average distance between peaks as a very very simplistic 
	// approximation of BPM	
	if (peaks.size() <= 1)
	{
		return false;
	}

	if (!AudioInfo::CheckAudioInfo(m_AudioInfo))
	{
		return false;
	}

	double clipSampleRate = m_AudioInfo.m_SampleRate;
	if (clipSampleRate == 0.0)
	{
		return false;
	}

	Peak prevPeak = *peaks.begin();
	double diffBetweenPeaksSum = 0.0;

	std::vector<Peak>::const_iterator itPeaks = peaks.begin();
	std::vector<Peak>::const_iterator itPeaksEnd = peaks.end();
	for (++itPeaks; itPeaks != itPeaksEnd; ++itPeaks)
	{
		double prevPeakTime = prevPeak.GetPeakSampleIndex() / clipSampleRate;	
		diffBetweenPeaksSum += (itPeaks->GetPeakSampleIndex() / clipSampleRate) - prevPeakTime;
		prevPeak = *itPeaks;
	}

	double avgTimeBetweenPeaks = diffBetweenPeaksSum / peaks.size();
	if (avgTimeBetweenPeaks == 0.0)
	{
		return false;
	}

	outBpmCount = 60.0 / avgTimeBetweenPeaks;
	
	return true;
}

bool AClip::GetBPM(double& bpmCount) // non const because we actually modify the AClip instance
{
    bpmCount = 0;
    if (!m_PeakDetector)
    {
        return false;
    }

    if (m_BPMCached)
    {
        bpmCount = m_BPMCachedValue;
		return true;
    }

    if (m_AudioInfo.m_NbSamples)
    {
        std::vector<Peak>   peaks;
        m_PeakDetector->GetPeaks(m_Samples, m_AudioInfo, peaks);
		
		if (ComputeBPM(peaks, bpmCount))
		{
			m_BPMCached = true;
			m_BPMCachedValue = bpmCount;
			return true;
		}
    }

    return false;
}

double AClip::GetDuration() const
{ 
	if (m_Samples.empty())
	{
		return 0.0;
	}

	return m_Samples.size() / m_AudioInfo.m_SampleRate;
}

//****************************************************************************************
//                                       E O F
//****************************************************************************************




