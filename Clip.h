//****************************************************************************************
// File:    Clip.h
//
// Author:  Julien Gilli
//****************************************************************************************

#ifndef Clip_h
#define Clip_h

#include <vector>
#include <map>

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
    // It's alright to use 0.0 as a default value since it is defined as all bits set to 
	// zero by the IEEE-754 standard.
	WarpMarker() : m_SampleIndex(0), m_SampleTime(0.0), m_BeatTime(0.0) {}
    WarpMarker(double sampleTime, double beatTime);
	WarpMarker(unsigned int sampleIndex, double beatTime);

	bool operator==(const WarpMarker& rhs) const;
	bool operator!=(const WarpMarker& rhs) const { return !operator==(rhs); }

    double GetBeatTime() const { return m_BeatTime; }
    
	double			GetSampleTime()		const		{ return m_SampleTime;							}
	unsigned int	GetSampleIndex()	const		{ return m_SampleIndex;							}	

private:
    unsigned int	m_SampleIndex;
	double			m_SampleTime;
    double			m_BeatTime;
};

/**
 *	An instance of AClip is an abstraction of 
 */
class AClip
{
private:
    AudioInfo               m_AudioInfo;   	    	

	// We use warp markers to match a sample time with a beat time, and conversely			
	std::map<unsigned int,	WarpMarker*>	m_SampleIndexToWarpMarker;
	std::map<double,		WarpMarker*>	m_BeatTimeToWarpMarker;

	// When calling SampleToBeatTime or BeatToSampleTime repeatedly over lots of subsequent samples,
	// we try to cache the last found warp marker so that we don't iterate over
	// the whole array of warp markes (m_WarpMarkers) every time.
	WarpMarker				m_CurrentCachedLowBoundWarpMarker;
	WarpMarker				m_CurrentCachedHighBoundWarpMarker;
	bool					m_LowAndHighBoundWarpMarkersCacheIsValid;
	
	// this points to memory allocated by the user, do not handle its deallocation
	PeakDetector*           m_PeakDetector;
    std::vector<Peak>       m_Peaks;
	
	// When getting the BPM value, we first try to use a cached value
	// If none is present, then we use our peak detector to approximate it
	bool					m_BPMCached;
	double					m_BPMCachedValue;
    	
	// Returns true if a warp marker which similar sample index already 
	// exists in this AClip instance
	bool SampleIndexKeyAlreadyExists(unsigned int sampleIndex) const;
	
	// Returns true if a warp marker which similar beat time already 
	// exists in this AClip instance	
	bool BeatTimeKeyAlreadyExists(double sampleIndex) const;

	// SampleToBeatTime and BeatToSampleTime both try to find bounding warp markers to 
	// perform a linear interpolation
	// They return true if a pair of bounding warp markers could be found, storing them in lowBoundMarker and highBoundMarker    
	bool FindBoundingWarpMarkersForBeatTime(double beatTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker);
    
	bool FindBoundingWarpMarkersForSampleIndex(unsigned int sampleIndex, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker);
	bool FindBoundingWarpMarkersForSampleTime(double sampleTime, WarpMarker& lowBoundMarker, WarpMarker& highBoundMarker);

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

	~AClip();

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




