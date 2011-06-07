#include <iostream>
#include <cstdlib>

#include "simplepeakdetector.h"
#include "Clip.h"

int main(int argc, char* argv[])
{
    AClip myTestClip;
    if (!argv[1] || !myTestClip.LoadDataFromFile(argv[1]))
    {
        return EXIT_FAILURE;
    }   	
    
	SimplePeakDetector simplePeakDetector;        
	myTestClip.SetPeakDetector(&simplePeakDetector);

	if (!myTestClip.AddWarpMarker(1.0, 2.0))
	{
		std::cerr << "Couldn't add warp marker, exiting..." << std::endl;
	}

	if (!myTestClip.AddWarpMarker(2.0, 4.0))
	{
		std::cerr << "Couldn't add warp marker, exiting..." << std::endl;
	}

	if (!myTestClip.AddWarpMarker(3.0, 6.0))
	{
		std::cerr << "Couldn't add warp marker, exiting..." << std::endl;
	}
	
	for (double currentSampleTime = 0.0; currentSampleTime < myTestClip.GetDuration(); currentSampleTime += 0.1)
	{
		std::cout << "Sample time: " << currentSampleTime << ", beat time: " << myTestClip.SampleToBeatTime(currentSampleTime) << std::endl;
	}

	for (double currentBeatTime = 0.0; currentBeatTime < 4.0; currentBeatTime += 0.1)
	{
		std::cout << "Beat time: " << currentBeatTime << ", sample time: " << myTestClip.BeatToSampleTime(currentBeatTime) << std::endl;
	}

    double bpmCount = 0.0;
    if (myTestClip.GetBPM(bpmCount))
    {
        std::cout << "BPM: " << bpmCount << std::endl;
    }
    else
    {
        std::cerr << "Could not get BPM value, exiting." << std::endl;
    }

	std::cout << "press a key to continue...";
	
	char c;
	std::cin >> c;

    return EXIT_SUCCESS;
}