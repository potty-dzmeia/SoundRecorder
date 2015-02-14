#include <windows.h>
#include "params.h"
#pragma once

class MyMMFile{
private:
	HMMIO				 hMMFileW;//handle to MultiMedia files
	PARAMS				 params;
	WAVEFORMATEX		 wfxC;

public:
	MyMMFile(pPARAMS,TCHAR*);
	~MyMMFile();

	BOOL OpenMMFileWrite(TCHAR*);
	BOOL WriteHeader();
	BOOL AppendDataToFile(signed char*);
	BOOL CloseMMFile();


};