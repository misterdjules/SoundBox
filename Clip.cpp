//****************************************************************************************
// File:    Clip.cpp
//
// Author:  Julien Gilli
//****************************************************************************************
#include <fstream>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <functional>
#include <cmath>

#include "audioconfig.h"
#include "Clip.h"
#include "simplepeakdetector.h"
#include "wavfilereader.h"
#include "mathutils.h"

#define TIME_RELATIVE_TOLERANCE (1.0 / (DEFAULT_SAMPLE_RATE * 10))
#define TIME_ABSOLUTE_TOLERANCE (1.0 / (DEFAULT_SAMPLE_RATE * 10))

WarpMarker::WarpMarker(double sampleTime, double beatTime)
: m_SampleTime(sampleTime), m_BeatTime(beatTime) 
{
	m_SampleIndex = MathUtils::Round(sampleTime * DEFAULT_SAMPLE_RATE);
}

WarpMarker::WarpMarker(unsigned int sampleIndex, double beatTime)
: m_SampleIndex(sampleIndex), m_BeatTime(beatTime)
{
	
}
bool WarpMarker::operator==(const WarpMarker& rhs) const
{
	if (this == &rhs)
	{
		return true;
	}
	
	return	m_SampleIndex	== rhs.m_SampleIndex	&& 
			m_BeatTime		== rhs.m_BeatTime		&&
			m_SampleTime	== rhs.m_SampleTime;
}

AClip::~AClip()
{	
	// deallocate WarpMarker instances
	std::map<unsigned int, WarpMarker*>::iterator itWarpMarkers = m_SampleIndexToWarpMarker.begin();
	std::map<unsigned int, WarpMarker*>::iterator itWarpMarkersEnd = m_SampleIndexToWarpMarker.end();
	for (; itWarpMarkers != itWarpMarkersEnd; ++itWarpMarkers)
	{
		WarpMarker* warpMarkerToDelete = itWarpMarkers->second;
		if (warpMarkerToDelete)
		{
			delete warpMarkerToDelete;
			itWarpMarkers->second = 0;
		}
	}

	// Then it's safe to clear both maps
	m_SampleIndexToWarpMarker.clear();
	m_BeatTimeToWarpMarker.clear();
}

bool AClip::SampleIndexKeyAlreadyExists(unsigned int sampleIndex) const
{
	// A simple find might not be sufficient here to insure that our data is valid
	// Adding two warp markers to adjacent samples, or samples that are very close 
	// to each other in the physical signal may not be a good idea...
	std::map<unsigned int, WarpMarker*>::const_iterator itSampleIndexToWarpMarker = m_SampleIndexToWarpMarker.find(sampleIndex);
	if (itSampleIndexToWarpMarker != m_SampleIndexToWarpMarker.end())
	{
		return true;
	}

	return false;
}

	
bool AClip::BeatTimeKeyAlreadyExists(double beatTime) const
{	
	// As for AClip::SampleIndexKeyAlreadyExists, it could be necessary to perform a less naive test to
	// determine if a beat time keu already exists. Moreover, due to floating point arithmetics, the user 
	// code which calls this method might use a beatTime value that is slightly different but should point to the 
	// same warp marker.
	std::map<double, WarpMarker*>::const_iterator itBeatTimeToWarpMarker = m_BeatTimeToWarpMarker.find(beatTime);
	if (itBeatTimeToWarpMarker != m_BeatTimeToWarpMarker.end())
	{
		return true;
	}

	return false;
}

bool AClip::ValidateWarpMarkerForAdd(const WarpMarker& warpMarkerToAdd)
{		
	double sampleTimeToAdd = warpMarkerToAdd.GetSampleTime();
	double beatTimeToAdd = warpMarkerToAdd.GetBeatTime();
	
	// Check that sample time for the warp marker to be added is within bounds of the physical signal
	// Beat time can't be negative, as it doesn't make sense to warp "in the past", but we could want 
	// to warp to any time in the future
	if (sampleTimeToAdd < 0.0 || 
		(!MathUtils::AlmostEqualWithTolerance(sampleTimeToAdd, GetDuration(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE) && sampleTimeToAdd > GetDuration()) || 
		beatTimeToAdd < 0.0)
	{
		return false;
	}

	if (m_SampleIndexToWarpMarker.empty() && m_BeatTimeToWarpMarker.empty())
	{
		// We're about to add the first warp marker, all necessary checks are done
		return true;
	}

	if (SampleIndexKeyAlreadyExists(warpMarkerToAdd.GetSampleIndex()))
	{
		return false;
	}

	if (BeatTimeKeyAlreadyExists(warpMarkerToAdd.GetBeatTime()))
	{
		return false;
	}	

	WarpMarker lowBoundWarpMarkerSampleTime;
	WarpMarker highBoundWarpMarkerSampleTime;
	if (!FindBoundingWarpMarkersForSampleTime(warpMarkerToAdd.GetSampleTime(), lowBoundWarpMarkerSampleTime, highBoundWarpMarkerSampleTime))
	{
		// We're adding our warp marker outside of two bouding warp markers, all necessary checks are done
		return true;
	}

	WarpMarker lowBoundWarpMarkerBeatTime;
	WarpMarker highBoundWarpMarkerBeatTime;
	if (!FindBoundingWarpMarkersForBeatTime(warpMarkerToAdd.GetBeatTime(), lowBoundWarpMarkerBeatTime, highBoundWarpMarkerBeatTime))
	{
		// We're adding our warp marker within bouding warp markers regarding sample time, but not beat time, 
		// there's something wrong.
		return false;
	}

	if (lowBoundWarpMarkerBeatTime != lowBoundWarpMarkerSampleTime || highBoundWarpMarkerBeatTime != highBoundWarpMarkerSampleTime)
	{
		// The two bounding warp markers we found when searching using beat time as search criteria are different
		// than those we found when using sample time as search criteria, there's something wrong
		return false;
	}

	if (MathUtils::AlmostEqualWithTolerance(sampleTimeToAdd, lowBoundWarpMarkerSampleTime.GetSampleTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)	||
		MathUtils::AlmostEqualWithTolerance(sampleTimeToAdd, highBoundWarpMarkerSampleTime.GetSampleTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)	||
		MathUtils::AlmostEqualWithTolerance(beatTimeToAdd, lowBoundWarpMarkerSampleTime.GetBeatTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)		||
		MathUtils::AlmostEqualWithTolerance(beatTimeToAdd, lowBoundWarpMarkerSampleTime.GetBeatTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE))
	{
		// The warp marker we're trying to add is too similar to existing ones, so let's not add it
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
	if (!AudioInfo::CheckAudioInfo(m_AudioInfo))
    {
		return false;
	}

	double duration = GetDuration();
	if (!AddWarpMarker(duration, duration))
	{
		return false;
	}

	return true;
}

bool AClip::GetFirstWarpMarker(WarpMarker& outFirstWarpMarker) const
{
	if (m_SampleIndexToWarpMarker.empty())
	{
		return false;
	}

	outFirstWarpMarker = *(m_SampleIndexToWarpMarker.begin()->second);
	return true;
}

bool AClip::GetLastWarpMarker(WarpMarker& outLastWarpMarker) const
{
	if (m_SampleIndexToWarpMarker.empty())
	{
		return false;
	}

	outLastWarpMarker = *(m_SampleIndexToWarpMarker.rbegin()->second);	
	return true;
}

bool AClip::AddWarpMarker(double sampleTime, double beatTime)
{
	if (!MathUtils::IsValidTime(sampleTime) || !MathUtils::IsValidTime(beatTime))
	{
		return false;
	}

	WarpMarker* warpMarkerToAdd = new WarpMarker(sampleTime, beatTime);
	
	if (!ValidateWarpMarkerForAdd(*warpMarkerToAdd))
	{
		return false;
	}
		
	m_SampleIndexToWarpMarker[warpMarkerToAdd->GetSampleIndex()] = warpMarkerToAdd; 
	m_BeatTimeToWarpMarker[beatTime] = warpMarkerToAdd; 

	m_LowAndHighBoundWarpMarkersCacheIsValid = false;

	return true;
}


bool AClip::FindBoundingWarpMarkersForSampleIndex(unsigned int sampleIndex, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker)
{    
    if (m_SampleIndexToWarpMarker.size() < 2)
    {
        // we need at least 2 bounding warp markers in the clip to find those
        // bounding the sample at "sampleIndex"
        // They should have been created after loading the audio data by calling AClip::AddDefaultWarpMarkers()
        return false;
    }
    
    // First, find low bound warp marker
	std::map<unsigned int, WarpMarker*>::iterator itJustAfterLowBoundWarpMarker = m_SampleIndexToWarpMarker.lower_bound(sampleIndex);
	if (itJustAfterLowBoundWarpMarker == m_SampleIndexToWarpMarker.end())
	{
		return false;
	}

	std::map<unsigned int, WarpMarker*>::iterator itLowBoundWarpMarkerCandidate;
	if (itJustAfterLowBoundWarpMarker == m_SampleIndexToWarpMarker.begin() ||
		itJustAfterLowBoundWarpMarker->second && itJustAfterLowBoundWarpMarker->second->GetSampleIndex() == sampleIndex)
	{
		itLowBoundWarpMarkerCandidate = itJustAfterLowBoundWarpMarker;
	}
	else
	{
		itLowBoundWarpMarkerCandidate = --itJustAfterLowBoundWarpMarker;
	}

	WarpMarker* lowBoundWarpMarkerFound = itLowBoundWarpMarkerCandidate->second;
	if (!lowBoundWarpMarkerFound)
	{
		return false;
	}

	unsigned int lowBoundWarpMarkerSampleIndex = lowBoundWarpMarkerFound->GetSampleIndex();
    if (lowBoundWarpMarkerSampleIndex > sampleIndex)
	{
		return false;		
	}

	lowBoundMarker = *lowBoundWarpMarkerFound;
	    
	// Then find high boung warp marker
	std::map<unsigned int, WarpMarker*>::const_iterator itFoundHighBoundWarpMarker = m_SampleIndexToWarpMarker.upper_bound(sampleIndex);
	if (itFoundHighBoundWarpMarker == m_SampleIndexToWarpMarker.end())
	{
		return false;
	}

	WarpMarker* highBoundWarpMarkerFound = itFoundHighBoundWarpMarker->second;
	if (!highBoundWarpMarkerFound)
	{
		return false;
	}

	unsigned int highBoundWarpMarkerSampleIndex = highBoundWarpMarkerFound->GetSampleIndex();
	if (highBoundWarpMarkerSampleIndex <= sampleIndex)
	{
		return false;
	}	
    
	highBoundMarker = *highBoundWarpMarkerFound;

	return true;
}

bool AClip::FindBoundingWarpMarkersForSampleTime(double sampleTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker)
{
	unsigned int sampleIndex = MathUtils::Round(sampleTime * DEFAULT_SAMPLE_RATE);
	return FindBoundingWarpMarkersForSampleIndex(sampleIndex, lowBoundMarker, highBoundMarker);
}

bool AClip::FindBoundingWarpMarkersForBeatTime(double beatTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker)
{
	if (m_BeatTimeToWarpMarker.size() < 2)
    {
        // we need at least 2 bounding warp markers in the clip to find those
        // bounding the beat time at "beatTime"
        // They should have been created after loading the audio data by calling AClip::AddDefaultWarpMarkers()
        return false;
    }
    
    // First, find low bound warp marker
	std::map<double, WarpMarker*>::iterator itJustAfterLowBoundWarpMarkerCandidate = m_BeatTimeToWarpMarker.lower_bound(beatTime);
	if (itJustAfterLowBoundWarpMarkerCandidate == m_BeatTimeToWarpMarker.end())
	{
		return false;
	}

	std::map<double, WarpMarker*>::iterator itLowBoundWarpMarker;
	std::map<double, WarpMarker*>::iterator lowestBound = m_BeatTimeToWarpMarker.begin();

	if (itJustAfterLowBoundWarpMarkerCandidate == lowestBound ||
		itJustAfterLowBoundWarpMarkerCandidate->second && MathUtils::AlmostEqualWithTolerance(itJustAfterLowBoundWarpMarkerCandidate->second->GetBeatTime(), beatTime, TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE))
	{
		itLowBoundWarpMarker = itJustAfterLowBoundWarpMarkerCandidate;
	}
	else
	{
		itLowBoundWarpMarker = --itJustAfterLowBoundWarpMarkerCandidate;
	}	

	WarpMarker* lowBoundWarpMarkerFound = itLowBoundWarpMarker->second;
	if (!lowBoundWarpMarkerFound)
	{
		return false;
	}

	double lowBoundWarpMarkerBeatTime = lowBoundWarpMarkerFound->GetBeatTime();
	if (lowBoundWarpMarkerBeatTime > beatTime && !MathUtils::AlmostEqualWithTolerance(lowBoundWarpMarkerBeatTime, beatTime, TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE))
	{
		return false;		
	}

	lowBoundMarker = *lowBoundWarpMarkerFound;
	    
	// Then find high boung warp marker
	std::map<double, WarpMarker*>::iterator itFoundHighBoundWarpMarker = m_BeatTimeToWarpMarker.upper_bound(beatTime);
	if (itFoundHighBoundWarpMarker == m_BeatTimeToWarpMarker.end())
	{
		return false;
	}
	
	WarpMarker* highBoundWarpMarkerFound = itFoundHighBoundWarpMarker->second;
	if (!highBoundWarpMarkerFound)
	{
		return false;
	}

	double highBoundWarpMarkerBeatTime = highBoundWarpMarkerFound->GetBeatTime();
	if (highBoundWarpMarkerBeatTime < beatTime || MathUtils::AlmostEqualWithTolerance(highBoundWarpMarkerBeatTime, beatTime, TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE))
	{
		return false;
	}	
    
	highBoundMarker = *highBoundWarpMarkerFound;

	return true;	
}

//----------------------------------------------------------------------------------------

double AClip::BeatToSampleTime(double BeatTime)
{
    bool foundBoundingMarkers = false;
	WarpMarker lowBoundMarker, highBoundMarker;

	if (m_LowAndHighBoundWarpMarkersCacheIsValid)
	{
		if ((BeatTime > m_CurrentCachedLowBoundWarpMarker.GetBeatTime() || MathUtils::AlmostEqualWithTolerance(BeatTime, m_CurrentCachedLowBoundWarpMarker.GetBeatTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)) &&
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

		double linearMappedSampleTime = MathUtils::LinearMap(	BeatTime, 
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
		if ((SampleTime > m_CurrentCachedLowBoundWarpMarker.GetSampleTime() || MathUtils::AlmostEqualWithTolerance(SampleTime, m_CurrentCachedLowBoundWarpMarker.GetSampleTime(), TIME_RELATIVE_TOLERANCE, TIME_ABSOLUTE_TOLERANCE)) &&
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

		double linearMappedBeatTime = MathUtils::LinearMap(	SampleTime, 
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
	
	const unsigned int INPUT_WINDOW_SIZE	= SimplePeakDetector::INPUT_WINDOW_SIZE;
	const unsigned int INPUT_WINDOW_OFFSET	= SimplePeakDetector::INPUT_WINDOW_OFFSET;
	
	float samples[INPUT_WINDOW_SIZE];
	memset(samples, 0, INPUT_WINDOW_SIZE * sizeof(float));

	unsigned int samplesLeftToRead = m_AudioInfo.m_NbSamples;
	unsigned int samplesRead = 0;

	// First, read a full "window" of samples to feed the peak detector
	if (!WavFileReader::ReadSamples(wavInputStream, m_AudioInfo, 
									samplesLeftToRead > INPUT_WINDOW_SIZE ? INPUT_WINDOW_SIZE : samplesLeftToRead, 
									samples, samplesRead))
	{
		return false;
	}

	// Make sure samplesRead is consistent
	assert(INPUT_WINDOW_SIZE - samplesRead >= 0);
	if (samplesRead < INPUT_WINDOW_SIZE)
	{
		// Pad the "samples" array with 0 doubles in case we read less than a full window		
		memset(samples + samplesRead, 0, (INPUT_WINDOW_SIZE - samplesRead) * sizeof(float));
	}
	
	// Detect peaks for the first chunk of samples
	std::vector<Peak> foundPeaks;
	if (m_PeakDetector)
	{
		m_PeakDetector->GetPeaks(samples, INPUT_WINDOW_SIZE, m_AudioInfo, foundPeaks);
	}

	samplesLeftToRead -= samplesRead;
	
	// Until there's no samples left to read, move the "window" of samples forward in the data by INPUT_WINDOW_OFFSET samples
	// This way, we make sure that we don't miss any peak that would have overlapped two adjacent windows
	while (samplesLeftToRead && WavFileReader::ReadSamples(	wavInputStream, m_AudioInfo, 
															(samplesLeftToRead > INPUT_WINDOW_SIZE - INPUT_WINDOW_OFFSET) ? INPUT_WINDOW_SIZE - INPUT_WINDOW_OFFSET : samplesLeftToRead, 
															samples + INPUT_WINDOW_OFFSET, samplesRead))
	{
		if (m_PeakDetector)
		{
			std::vector<Peak> peaksFoundInCurrentWindow;
			m_PeakDetector->GetPeaks(samples, INPUT_WINDOW_SIZE, m_AudioInfo, peaksFoundInCurrentWindow);
			std::for_each(peaksFoundInCurrentWindow.begin(), peaksFoundInCurrentWindow.end(), Peak::OffsetByFunctor(m_AudioInfo.m_NbSamples - samplesLeftToRead));
			std::copy(peaksFoundInCurrentWindow.begin(), peaksFoundInCurrentWindow.end(), std::insert_iterator<std::vector<Peak> >(foundPeaks, foundPeaks.end()));
		}

		memcpy(samples, samples + (INPUT_WINDOW_SIZE - INPUT_WINDOW_OFFSET), SimplePeakDetector::INPUT_WINDOW_OFFSET);
		samplesLeftToRead -= samplesRead;		
	}
    
	m_Peaks = foundPeaks;

	wavInputStream.close();	

	if (samplesLeftToRead == 0)
	{
		return true;
	}

    return false;    
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
		if (ComputeBPM(m_Peaks, bpmCount))
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
	if (!AudioInfo::CheckAudioInfo(m_AudioInfo))
	{
		return 0.0;
	}

	return m_AudioInfo.m_NbSamples / m_AudioInfo.m_SampleRate;
}

//****************************************************************************************
//                                       E O F
//****************************************************************************************




