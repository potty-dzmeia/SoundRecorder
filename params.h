#include <windows.h>
#pragma once 


enum output_files
{
  WAV_ONLY,
  OGG_ONLY,
  WAV_AND_OGG
};

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
		 output_files    iCompressionOnOFFBoth;
		 /*-if iCompressionOnOFFBoth is 1-> write only to .wav files(off)
		   -if iCompressionOnOFFBoth is 2-> write only to .ogg files(on)
		   -if iCompressionOnOFFBoth is 3-> write both	                 */
		 char  szDirectoryName[MAX_PATH]; 
} PARAMS,*pPARAMS;
//------------------------------------