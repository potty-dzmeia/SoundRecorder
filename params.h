#include <windows.h>
#pragma once 

#define WAV_ONLY	(1)
#define OGG_ONLY	(2)
#define WAV_AND_OGG (3)

//----------------------------------------------------------------
				/**Parameters with sound specifications**/
typedef struct{						
		 HWND	hwnd;				
		 BOOL	bKill;			  // If the thread should continue running
		 LPGUID deviceLPGUID;     // Pointer to the GUID structure identifying the capture device
		 int	iSampleRate;
		 int	iFileSizeInSeconds;
		 int	iOverlapInSeconds; // If we should start recodring new file before the old file has been closed
		 int	iStereo;
		 int	iBitsPerSample;
		 int    iFilesWritten;
		 float  qualityOfOggCompression;
		 int    iCompressionOnOFFBoth;
		 /*-if iCompressionOnOFFBoth is 1-> write only to .wav files(off)
		   -if iCompressionOnOFFBoth is 2-> write only to .ogg files(on)
		   -if iCompressionOnOFFBoth is 3-> write both	                 */
		 char  szDirectoryName[MAX_PATH]; 
} PARAMS,*pPARAMS;
//------------------------------------