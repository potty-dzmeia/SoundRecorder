#include <windows.h>
#pragma once 


enum output_files
{
  WAV_ONLY,   // write only to .wav files
  OGG_ONLY,   // write only to .ogg files
  WAV_AND_OGG // write both
};

//----------------------------------------------------------------
				/**Parameters with sound specifications**/
typedef struct{						
		 HWND	  hwnd;				
		 BOOL 	bKill;			  // If the thread should continue running
		 LPGUID deviceLPGUID;     // Pointer to the GUID structure identifying the capture device
		 int	  iSampleRate;
		 int	  iFileSizeInSeconds;
		 int	  iOverlapInSeconds; // If we should start recodring new file before the old file has been closed
		 int	  iNumberOfChannels;
		 int	  iBitsPerSample;
		 int    iFilesWritten;
		 float  qualityOfOggCompression;
		 output_files iCompressionOnOFFBoth;            
		 char   szDirectoryName[MAX_PATH]; 
} PARAMS,*pPARAMS;
//------------------------------------