//****************************************************************************************
// File:    Clip.h
//
// Author:  Julien Gilli
//****************************************************************************************

#ifndef Clip_h
#define Clip_h

#include <vector>

#include "audioformats.h"
#include "peakdetector.h"

#define BPM_CACHE_INVALID -1

class BPMCounter;

//========================================================================================

class WarpMarker
{
public:
    WarpMarker() : m_SampleTime(0.0), m_BeatTime(0.0) {}
    WarpMarker(double sampleTime, double beatTime) : m_SampleTime(sampleTime), m_BeatTime(beatTime) {}

	bool operator==(const WarpMarker& rhs) const;
	bool operator!=(const WarpMarker& rhs) const { return !operator==(rhs); }

    double GetBeatTime() const { return m_BeatTime; }
    double GetSampleTime() const { return m_SampleTime; }
	
	typedef double (*TimeSelector)(const WarpMarker& warpMarker);

	static double SampleTimeSelector(const WarpMarker& warpMarker);
	static double BeatTimeSelector(const WarpMarker& warpMarker);

private:
    double m_SampleTime;
    double m_BeatTime;
};

class AClip
{
private:
    AudioInfo               m_AudioInfo;   	
    
	std::vector<double>     m_Samples;

	std::vector<WarpMarker> m_WarpMarkers;	
	WarpMarker				m_CurrentCachedLowBoundWarpMarker;
	WarpMarker				m_CurrentCachedHighBoundWarpMarker;
	bool					m_LowAndHighBoundWarpMarkersCacheIsValid;

	PeakDetector*           m_PeakDetector;
    std::vector<Peak>       m_Peaks;
	
	short                   m_BPMCached;
    	
    bool FindBoundingWarpMarkersForTime(double beatTime, WarpMarker::TimeSelector timeSelector, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const;
	
	bool FindBoundingWarpMarkersForBeatTime(double beatTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const;
    bool FindBoundingWarpMarkersForSampleTime(double sampleTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const;

	bool ValidateWarpMarkerForAdd(const WarpMarker& warpMarkerToAdd);

    // Add default warp markers for beginning and end of clip
    bool AddDefaultWarpMarkers();

	bool GetFirstWarpMarker(WarpMarker& outFirstWarpMarker) const;
	bool GetLastWarpMarker(WarpMarker& outLastWarpMarker) const;

public:

    // ...
    AClip::AClip() 
        :   m_PeakDetector(0),
            m_BPMCached(BPM_CACHE_INVALID),
			m_LowAndHighBoundWarpMarkersCacheIsValid(false)
    {        
    }

    // Fill internal data structures with content from file
    bool LoadDataFromFile(const std::string& filePath);
    
    // Convert a position in the sample that is given
    // in beat time to sample time (in seconds).
    double BeatToSampleTime(double BeatTime);

    // Convert a position in the sample that is given
    // in sample time (in seconds) to beat time.
    double SampleToBeatTime(double SampleTime);

    // ...
	bool AddWarpMarker(double sampleTime, double beatTime);

	void SetPeakDetector(PeakDetector* peakDetector) { m_PeakDetector = peakDetector; }
    bool GetBPM(unsigned int& bpmCount) const;
    
    void Analyze();

	double GetDuration() const;
};


#endif


//****************************************************************************************
//                                       E O F
//****************************************************************************************




