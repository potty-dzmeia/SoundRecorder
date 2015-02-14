#include "MyMMFile.h"

//---------------------------------------------------------------------------
//							Constructor + Destructor
//---------------------------------------------------------------------------
MyMMFile :: MyMMFile(pPARAMS pParams,TCHAR* szFileName){
	
	hMMFileW=NULL;
	 
	params=*pParams;
	
	wfxC.cbSize=			sizeof(WAVEFORMATEX);
	wfxC.wFormatTag=		WAVE_FORMAT_PCM;
	wfxC.nChannels=			params.iStereo;
	wfxC.nSamplesPerSec=	params.iSampleRate;
	wfxC.wBitsPerSample=	params.iBitsPerSample;
	wfxC.nBlockAlign=		(wfxC.nChannels*wfxC.wBitsPerSample)/8;
	wfxC.nAvgBytesPerSec=	wfxC.nBlockAlign*wfxC.nSamplesPerSec;
	
	OpenMMFileWrite(szFileName);
	WriteHeader();

}
MyMMFile :: ~MyMMFile(){
	
	CloseMMFile();
}
//---------------------------------------------------------------------------
//							Opens file for Writing
//---------------------------------------------------------------------------
BOOL MyMMFile :: OpenMMFileWrite(TCHAR *szFileName){
	
	if(mmioOpen(szFileName, NULL, MMIO_EXIST))		//if the file exists
		mmioOpen(szFileName, NULL, MMIO_DELETE);	//delete it

	if(NULL==(hMMFileW=mmioOpen(szFileName, NULL, MMIO_CREATE | MMIO_WRITE))){
		MessageBox(params.hwnd, TEXT("Error opening/creating file for writing"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		CloseMMFile();
		return 0;
	}
	
	return 1;
}
//---------------------------------------------------------------------------
//						 Writes the Header to the MMFile
//---------------------------------------------------------------------------
BOOL MyMMFile :: WriteHeader(){
	
	MMCKINFO		chunkInf;//chunk information
	HRESULT			hr;
	
	chunkInf.fccType=mmioStringToFOURCC("WAVE",0);		//creating the Main Chunk
	chunkInf.cksize=36+wfxC.nAvgBytesPerSec*params.iFileSize;
	if(MMSYSERR_NOERROR!=mmioCreateChunk(hMMFileW, &chunkInf, MMIO_CREATERIFF)){
		MessageBox(params.hwnd, TEXT("Error creating RIFF chunk"), TEXT("Error"), MB_OK);
		CloseMMFile();
		return 0;
	}

	chunkInf.ckid=mmioStringToFOURCC("fmt ",0);	
	chunkInf.cksize=sizeof(wfxC);
												//Creating the first subchunk-"fmt"
	hr=mmioCreateChunk(hMMFileW, &chunkInf,0);
	if(hr==MMIOERR_CANNOTSEEK){
		MessageBox(params.hwnd, TEXT("Error creating fmt chunk: MMIOERR_CANNOTSEEK"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		CloseMMFile();
		return 0;
	}
	if(hr==MMIOERR_CANNOTWRITE){
		MessageBox(params.hwnd, TEXT("Error creating fmt chunk: MMIOERR_CANNOTWRITE"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		CloseMMFile();
		return 0;
	}

	if(mmioWrite(hMMFileW, (HPSTR)&wfxC, sizeof(WAVEFORMATEX))==-1){
		MessageBox(params.hwnd, TEXT("Error writing wfxC structure to fmt chunk."), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		 CloseMMFile();
		return 0;
	}

	if(MMSYSERR_NOERROR!=mmioAscend(hMMFileW, &chunkInf, 0)){//Ascending from fmt chunk
		MessageBox(params.hwnd, TEXT("Error when Ascending from fmt chunk"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		CloseMMFile();
		return 0;
	}
	
	chunkInf.ckid=mmioStringToFOURCC("data",0);			//Creating the second subchunk-"data"
	chunkInf.cksize=wfxC.nAvgBytesPerSec*params.iFileSize;
												
	hr=mmioCreateChunk(hMMFileW, &chunkInf,0);
	if(hr==MMIOERR_CANNOTSEEK){
		MessageBox(params.hwnd, TEXT("Error creating fmt chunk: MMIOERR_CANNOTSEEK"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		CloseMMFile();
		return 0;
	}
	if(hr==MMIOERR_CANNOTWRITE){
		MessageBox(params.hwnd, TEXT("Error creating fmt chunk: MMIOERR_CANNOTWRITE"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		CloseMMFile();
		return 0;
	}
	
	return 1;
}
//---------------------------------------------------------------------------
//							Appends data to MMfile(used in ReadFrom SoundCard)
//---------------------------------------------------------------------------
BOOL MyMMFile :: AppendDataToFile(signed char *lpDataWrite){
	
	//mmioSeek(hMMFileW, 0, SEEK_END);//Appends any data to the end of the file
	if(mmioWrite(hMMFileW, (HPSTR)lpDataWrite, wfxC.nAvgBytesPerSec/2)==-1){
		MessageBox(params.hwnd, TEXT("Error when writing sound data to file"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		CloseMMFile();
		return 0;
	}
	
	return 1;
}
//---------------------------------------------------------------------------
//							Close MultiMEdia FIle
//---------------------------------------------------------------------------
BOOL MyMMFile :: CloseMMFile(){
	if(mmioClose(hMMFileW,NULL)!=0){//close the old file
		MessageBox(params.hwnd, TEXT("Error closing file"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
		return 0;
		}
	return 1;

}