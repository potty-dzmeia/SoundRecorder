#include <dsound.h>
#include "params.h"

#pragma once

class MyRecorder{
private:
	//for capturing
	LPDIRECTSOUNDCAPTURE		lpDSC;			//pointer to DirectSoundCapture object
	LPDIRECTSOUNDCAPTUREBUFFER	lpDSCB;			//pointer to DirectSoundCaptureBuffer
	LPDIRECTSOUNDCAPTUREBUFFER8 lpDSCB8;		//interface used to manipulate sound capture buffer
	WAVEFORMATEX				wfxC;			//WFX structure for using with the capture buffer
	DSCBUFFERDESC				dscbdesc;		//Description of the DSCapture buffer /Must be the same format as the play buffer/
	signed char					*pbDataToWrite;	//data from the soundcard
	PARAMS						params;			//parameters

public:
	MyRecorder();
	MyRecorder(pPARAMS);
	~MyRecorder();
	
	HANDLE						rghEvent[2];	//handle for Events(for CaptureBuffer)
	
	
	//----------------Creates DS Capture-------------	1
	BOOL InitDSCaptureCreate();
	//-------------Creates Capture Buffer------------	2
	BOOL CreateMyCaptureBuffer();
	//-------------Add Notification Events-----------	3
	BOOL AddNotificationEvents();
	//-----------Start the CApure Buffer-------------   4
	BOOL StartCaptureBuffer();
	//-----------Reading from sound card-------------	5
	signed char* ReadFromSoundCard(DWORD);
	//--Information for the sound card capabilities--
	DSCCAPS* GetDevCaps();
	//------------Shutting down DirectSound----------last
	void ShutDown();


};