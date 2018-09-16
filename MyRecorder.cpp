#include "MyRecorder.h"

//------------------------------------------------------------------
//							Constructor/Destructor
//------------------------------------------------------------------
MyRecorder :: MyRecorder()
{
  lpDSC=NULL;	
  lpDSCB=NULL;
  lpDSCB8=NULL;
  pbDataToWrite=NULL;

  if(!InitDSCaptureCreate())
  {
    MessageBox(params.hwnd, TEXT("Error in InitDSCaptureCreate function call"), TEXT("Error"),MB_OK); 
    ShutDown();
  }
}


MyRecorder :: MyRecorder(pPARAMS pParams)
{	
  lpDSC=NULL;	
  lpDSCB=NULL;
  lpDSCB8=NULL;
  pbDataToWrite=NULL;

  params=*pParams;


  if(!InitDSCaptureCreate()){
    MessageBox(params.hwnd, TEXT("Error in InitDSCaptureCreate function call"), TEXT("Error"),MB_OK); 
    ShutDown();
  }
  if(!CreateMyCaptureBuffer()){
    MessageBox(params.hwnd, TEXT("Error in CreateMyCaptureBuffer function call"), TEXT("Error"),MB_OK); 
    ShutDown();
  }
  if(!AddNotificationEvents()){
    MessageBox(params.hwnd, TEXT("Error in AddNotificationEvents function call"), TEXT("Error"),MB_OK); 
    ShutDown();
  }
  if(!StartCaptureBuffer()){
    MessageBox(params.hwnd, TEXT("Error when trying to start capture buffer"), TEXT("Error"),MB_OK); 
    ShutDown();
  }

}

MyRecorder::~MyRecorder()
{
  ShutDown();
}
//-----------------------------------------------------------------------------
//						Shutting down DirectSound
//-----------------------------------------------------------------------------
void MyRecorder :: ShutDown(){

  if(lpDSCB8!=NULL)
    lpDSCB->Stop();
  if(lpDSCB8!=NULL)
    lpDSCB->Release();

  if(lpDSCB!=NULL)
    lpDSCB->Release();
  if(lpDSC!=NULL)
    lpDSC->Release();

  if(!lpDSCB8!=NULL)
    delete(lpDSCB8);
  if(!lpDSCB!=NULL)
    delete(lpDSCB);
  if(!lpDSC!=NULL)
    delete(lpDSC);
  if(pbDataToWrite)
    delete(pbDataToWrite);
}

//-----------------------------------------------------------------------------
//						Create  DSCapture object
//-----------------------------------------------------------------------------
BOOL MyRecorder :: InitDSCaptureCreate(){

  if(DS_OK!=DirectSoundCaptureCreate8(params.deviceLPGUID, &lpDSC, NULL)){//creating the DSCapture object
    MessageBox(params.hwnd, TEXT("Error when creating DirectSoundCaptureCreate object"), TEXT("ERROR"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  }

  return 1;
}  

//------------------------------------------------------------------------------
//					    Creates the Capture Buffer
//------------------------------------------------------------------------------
BOOL MyRecorder :: CreateMyCaptureBuffer(){

  HRESULT hr;

  wfxC.cbSize=			sizeof(WAVEFORMATEX);		//configuring the waveformat
  wfxC.wFormatTag=		WAVE_FORMAT_PCM;
  wfxC.nChannels=			params.iNumberOfChannels;// 1 or 2
  wfxC.nSamplesPerSec=	params.iSampleRate;
  wfxC.wBitsPerSample=	params.iBitsPerSample;
  wfxC.nBlockAlign=		(wfxC.nChannels*wfxC.wBitsPerSample)/8; // 
  wfxC.nAvgBytesPerSec=	wfxC.nSamplesPerSec*wfxC.nBlockAlign;	// 


  dscbdesc.dwSize=sizeof(DSCBUFFERDESC);		//configuring the BUffer description
  dscbdesc.dwFlags=0;
  dscbdesc.dwBufferBytes=wfxC.nAvgBytesPerSec; //Size of capture buffer to create in bytes
  dscbdesc.dwReserved=0;
  dscbdesc.lpwfxFormat=&wfxC;
  dscbdesc.dwFXCount=0;
  dscbdesc.lpDSCFXDesc=NULL;

  pbDataToWrite=new signed char[wfxC.nAvgBytesPerSec/2]; // We need space for data for half a second - but lets us

  if(lpDSC==NULL){//if there is no DSCapture object
    MessageBox(params.hwnd, TEXT("Create DSCapture object before creating Buffer"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  }

  hr=lpDSC->CreateCaptureBuffer(&dscbdesc,&lpDSCB, NULL);//Creating DS Capture Buffer
  switch(hr){ 
  case DSERR_BADFORMAT:
    MessageBox(params.hwnd, TEXT("Error code:  DSERR_BADFORMAT"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  case DSERR_GENERIC:
    MessageBox(params.hwnd, TEXT("Error code:  DSERR_GENERIC"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  case DSERR_NODRIVER:
    MessageBox(params.hwnd, TEXT("Error code:  DSERR_NODRIVER"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  case DSERR_OUTOFMEMORY:
    MessageBox(params.hwnd, TEXT("Error code:  DSERR_OUTOFMEMORY"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  case DSERR_UNINITIALIZED:
    MessageBox(params.hwnd, TEXT("Error code:  DSERR_UNINITIALIZED"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  case DSERR_INVALIDPARAM:
    MessageBox(params.hwnd, TEXT("Error code:  DSERR_INVALIDPARAM"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  }

  if(S_OK!=lpDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&lpDSCB8))
  {
    MessageBox(params.hwnd, TEXT("Error code:  DSERR_INVALIDPARAM"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  }


  return 1;
}


//----------------------------------------------------------------------------
//						 Adds the notification points
//----------------------------------------------------------------------------
BOOL MyRecorder :: AddNotificationEvents()
{
  LPDIRECTSOUNDNOTIFY8	lpDSNotify;			//for the notification events
  DSBPOSITIONNOTIFY		rgdsbpn[2];			//structures for the Events
  HRESULT					hr;


  if (S_OK!=lpDSCB8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&lpDSNotify)){
    MessageBox(params.hwnd, TEXT("Error when getting interface pointer of type IID_IDirectSoundNotify"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  }

  //Creating the Events
  for (int i = 0; i <2; i++)
  {			
    rghEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == rghEvent[i]){
      MessageBox(params.hwnd, TEXT("Error when creating Events"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
      ShutDown();
      return 0;
    }//if
  }//for


  // Describe notifications. 
  rgdsbpn[0].dwOffset = (wfxC.nAvgBytesPerSec/2) -1;
  rgdsbpn[0].hEventNotify = rghEvent[0];

  rgdsbpn[1].dwOffset = wfxC.nAvgBytesPerSec - 1;
  rgdsbpn[1].hEventNotify = rghEvent[1];

  // Create notifications.
  if(DSERR_INVALIDPARAM==(hr=lpDSNotify->SetNotificationPositions(2, rgdsbpn))){
    MessageBox(params.hwnd, TEXT("Error when setting notification positions: DSERR_INVALIDPARAM"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  }
  else if(hr==DSERR_OUTOFMEMORY){
    MessageBox(params.hwnd, TEXT("Error when setting notification positions: DSERR_OUTOFMEMORY"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;
  }

  return 1;
}
//---------------------------------------------------------------------------
//						 ReadFromSoundCard
//---------------------------------------------------------------------------
signed char* MyRecorder :: ReadFromSoundCard(DWORD dwBufferOffset)
{

  LPVOID  lpvPtr1;	//Address of a pointer to contain the first block of the sound buffer to be locked 
  DWORD	  dwBytes1;	//Address of a variable to contain the number of bytes pointed to by the lplpvAudioPtr1 parameter.
  LPVOID  lpvPtr2;	//second pointer
  DWORD	  dwBytes2;	//second variable 
  DWORD	  dwReader;	//we  must not read data after the ReadCursor

  if(DS_OK!=lpDSCB8->GetCurrentPosition(NULL, &dwReader)){	//Gets the position of the read cursor
    MessageBox(params.hwnd, TEXT("Error when trying to call lpDSCB->GetCurrentPosition "), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    return 0;
  }
  if(dwBufferOffset==0){//if the Read Cursor is smaller the the marker, w8 till it gets bigger so we can lock safely
    while(dwReader<wfxC.nAvgBytesPerSec/2){
      lpDSCB8->GetCurrentPosition(NULL, &dwReader);
    }
  }
  else{
    while(dwReader>wfxC.nAvgBytesPerSec/2){//if the Read Cursor is smaller the the marker, w8 till it gets bigger(passes the zero) so we can lock safely
      lpDSCB8->GetCurrentPosition(NULL, &dwReader);
    }
  }

  if( DS_OK!=lpDSCB8->Lock(dwBufferOffset,// Lock the portion of the buffer we need
    (wfxC.nAvgBytesPerSec/2), &lpvPtr1,
    &dwBytes1,&lpvPtr2,
    &dwBytes2,NULL)
    ){
      MessageBox(params.hwnd, TEXT("Error when trying to lock the Capture buffer"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
      return 0;
  }

  CopyMemory(pbDataToWrite, lpvPtr1, dwBytes1);
  if(lpvPtr2!=NULL)
  {		//if there is overlapping and there is information in lpvPtr2- read it
    CopyMemory(pbDataToWrite+dwBytes1, lpvPtr2,dwBytes2);
  }

  if(DS_OK!=lpDSCB8->Unlock(lpvPtr1, dwBytes1, lpvPtr2, dwBytes2))// Unlock the portion of the buffer we just used							
  {						
    MessageBox(params.hwnd, TEXT("Error when trying to unlock the Capture buffer"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    return 0;
  }

  return pbDataToWrite; 
}

//---------------------------------------------------------------------------
//							 Starts the capture buffer
//---------------------------------------------------------------------------
BOOL MyRecorder ::  StartCaptureBuffer(){

  if(DS_OK!=lpDSCB8->Start(DSCBSTART_LOOPING)){// Starting the Loop
    MessageBox(params.hwnd, TEXT("Error when trying to start the Capture Buffer"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
    ShutDown();
    return 0;	
  }

  return 1;
}

DSCCAPS* MyRecorder :: GetDevCaps(){

  static  DSCCAPS	dsccaps;

  dsccaps.dwSize=sizeof (DSCCAPS);

  lpDSC->GetCaps(&dsccaps);

  return &dsccaps;
}