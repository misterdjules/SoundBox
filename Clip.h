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

//========================================================================================

/**
 *	A WarpMarker instance matches a sample found at sample time seconds in the clip it belongs to with
 *	the time at beatTime seconds in the set.
 */
class WarpMarker
{
public:
    WarpMarker() : m_SampleTime(0.0), m_BeatTime(0.0) {}
    WarpMarker(double sampleTime, double beatTime) : m_SampleTime(sampleTime), m_BeatTime(beatTime) {}

	bool operator==(const WarpMarker& rhs) const;
	bool operator!=(const WarpMarker& rhs) const { return !operator==(rhs); }

    double GetBeatTime()	const { return m_BeatTime;		}
    double GetSampleTime()	const { return m_SampleTime;	}
	
	typedef double (*TimeSelector)(const WarpMarker& warpMarker);

	// These two static methods of type TimeSelector provide function pointers to be used as shortcuts by 
	// algorithm working either with beat time or sample time
	static double SampleTimeSelector(const WarpMarker& warpMarker);
	static double BeatTimeSelector(const WarpMarker& warpMarker);

private:
    double m_SampleTime;
    double m_BeatTime;
};

/**
 *	An instance of AClip is an abstraction of 
 */
class AClip
{
private:
    AudioInfo               m_AudioInfo;   	    	

	// We use warp markers to match a sample time with a beat time, and conversely
	std::vector<WarpMarker> m_WarpMarkers;	
	
	// When calling SampleToBeatTime or BeatToSampleTime repeatedly over lots of subsequent samples,
	// we try to cache the last found warp marker so that we don't iterate over
	// the whole array of warp markes (m_WarpMarkers) every time.
	WarpMarker				m_CurrentCachedLowBoundWarpMarker;
	WarpMarker				m_CurrentCachedHighBoundWarpMarker;
	bool					m_LowAndHighBoundWarpMarkersCacheIsValid;
	
	PeakDetector*           m_PeakDetector;
    std::vector<Peak>       m_Peaks;
	
	// When getting the BPM value, we first try to use a cached value
	// If none is present, then we use our peak detector to approximate it
	bool					m_BPMCached;
	double					m_BPMCachedValue;
    	
	// SampleToBeatTime and BeatToSampleTime both try to find bounding warp markers to 
	// perform a linear interpolation, using FindBoundingWarpMarkersForSampleTime and FindBoundingWarpMarkersForBeatTime respectively.
	// FindBoundingWarpMarkersForTime is used by both of them.
	// They all return true if a pair of bounding warp markers could be found, storing them in lowBoundMarker and highBoundMarker
    bool FindBoundingWarpMarkersForTime(double beatTime, WarpMarker::TimeSelector timeSelector, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const;
	
	bool FindBoundingWarpMarkersForBeatTime(double beatTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const;
    bool FindBoundingWarpMarkersForSampleTime(double sampleTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker) const;

	// Returns true if data in warpMarkerToAdd is consistent, false otherwise
	bool ValidateWarpMarkerForAdd(const WarpMarker& warpMarkerToAdd);
    
	// Gets the first warp marker of the clip in outFirstWarpMarker.
	// Returns true if it could find it, false otherwise
	bool GetFirstWarpMarker(WarpMarker& outFirstWarpMarker) const;
	
	// Gets the last warp marker of the clip in outLastWarpMarker.
	// Returns true if it could find it, false otherwise
	bool GetLastWarpMarker(WarpMarker& outLastWarpMarker) const;
	
	bool ComputeBPM(const std::vector<Peak>& peaks, double& outBpmCount) const;
		
public:

    // ...
    AClip::AClip() 
        :   m_PeakDetector(0),
            m_BPMCached(false),
			m_BPMCachedValue(0.0),
			m_LowAndHighBoundWarpMarkersCacheIsValid(false)
    {        
    }	

	// Fill internal data structures with content from file
	// Feeds the peak detector, too. 
	// Returns true if the file could successfully be read, false otherwise
	// Limitation: filePath must be an absolutePath
    bool LoadDataFromFile(const std::string& filePath);
    
    // Convert a position in the sample that is given
    // in beat time to sample time (in seconds).
    double BeatToSampleTime(double BeatTime);

    // Convert a position in the sample that is given
    // in sample time (in seconds) to beat time.
    double SampleToBeatTime(double SampleTime);
    
	// Add default warp markers for beginning and end of clip
    bool AddDefaultWarpMarkers();

	// Add a warp marker that matches the sample found at sample time seconds in the clip with
	// the time at beatTime seconds in the set
	bool AddWarpMarker(double sampleTime, double beatTime);

	// Set the peak detector instance used to detect onsets in the instance's associated
	// signal
	void SetPeakDetector(PeakDetector* peakDetector) { m_PeakDetector = peakDetector; }

	// Get the number of bets per minute in bpmCount, returns true if it managed 
	// to actually figure it out, false otherwise
	bool GetBPM(double& bpmCount);       
	
	// Get duration of a clip in second
	double GetDuration() const;
};


#endif


//****************************************************************************************
//                                       E O F
//****************************************************************************************




