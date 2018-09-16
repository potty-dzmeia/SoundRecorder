#include <windows.h>
#include <process.h>
#include <shlobj.h>
#include <strsafe.h>

#include "resource.h" 
#include "MyRecorder.h"
#include "MyMMFile.h"
#include "OGGencoder.h"
#include "params.h"

#define WIN32_LEAN_AND_MEAN // no mfc

#define POTTY_RECORDER                "PottyRecorder"
#define POTTY_RECORDER_VERSION        1.3
#define POTTY_RECORDER_VERSION_YEAR		"2018"

#define MIN_LENGTH_IN_SECS            1
#define DEFAULT_OVERLAPPING_IN_SECS		0
#define DEFAULT_LENGTH_IN_SECS			  60 


 //-----------Procedures-------------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDisplay1(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDisplay(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutAbout(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutIndex(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDevCaps(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutCaptureDevice(HWND, UINT, WPARAM, LPARAM);
//-------------------------------------------------

BOOL CALLBACK DSEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc,LPCTSTR lpszDrvName, LPVOID lpContext);

float translateToFloat(int); // Translates quality from -1to10 to -0.1to10

PARAMS		  params;//params for the thread
HWND		    hwndDisplay, hwndDisplay1;//The display windows
TCHAR				szBuffer[MAX_PATH];// char buffer used for different things
 

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{ 
  HWND         hwnd ;
  MSG          msg ;
  WNDCLASSEX   wndclass;
  HMENU		     hMenu;

  wndclass.cbSize		     = sizeof(wndclass);
  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON1)) ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)CreateSolidBrush (RGB(184,200,242)) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = POTTY_RECORDER ;
  wndclass.hIconSm		   = NULL;

  if (!RegisterClassEx (&wndclass))
  {
    MessageBox (NULL, TEXT ("This program requires Windows NT!"), 
      szBuffer, MB_ICONERROR) ;
    return 0 ;
  }

  hMenu=LoadMenu(hInstance, MAKEINTRESOURCE (IDR_MENU1));

  sprintf_s(szBuffer, "%s v%.1f         Ham Radio software", POTTY_RECORDER, POTTY_RECORDER_VERSION);
  hwnd = CreateWindow (POTTY_RECORDER,             // window class name
                       szBuffer,					  // window caption
                       WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX,// window style
                       CW_USEDEFAULT,              // initial x position
                       CW_USEDEFAULT,              // initial y position
                       500,						  // initial x size
                       360,						  // initial y size
                       NULL,                       // parent window handle
                       hMenu,                       // window menu handle
                       hInstance,                  // program instance handle
                       NULL) ;                     // creation parameters

  ShowWindow (hwnd, iCmdShow) ;
  UpdateWindow (hwnd) ;

  while (GetMessage (&msg, NULL, 0, 0))
  {
    TranslateMessage (&msg) ;
    DispatchMessage (&msg) ;
  }
  return (int)msg.wParam ;
}


static MyMMFile* createWavFile(pPARAMS pParams)
{
  SYSTEMTIME		tS; 
  TCHAR				  szFile[MAX_PATH];

  //Reads the local time and creates file with the specific name.....
	GetLocalTime(&tS);
  StringCchPrintf(szFile, MAX_PATH, TEXT("%s%u-%.2u-%.2u %.2u%.2u %.2u %.3u.wav"), pParams->szDirectoryName, tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute,tS.wSecond, tS.wMilliseconds);

  return new MyMMFile(pParams, szFile);
}


static BOOL writeDataToWav(MyMMFile *file, signed char	*data)
{
  //writes the data to .ogg and .wav files
  if(file)
  {
    if(!file->AppendDataToFile(data)) 
    { 
      MessageBox(NULL, "Error writing data to WAV file", "Error", MB_OK);
      return false;
    }	
  }
  return true;
}


static OGGencoder* createOggEncoder(pPARAMS pParams)
{
  SYSTEMTIME		tS;
  TCHAR				  szFile[MAX_PATH];

  GetLocalTime(&tS);
  StringCchPrintf(szFile, MAX_PATH, TEXT("%s%u-%.2u-%.2u %.2u%.2u %.2u %.3u.ogg"), pParams->szDirectoryName, tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute,tS.wSecond, tS.wMilliseconds);
  
  OGGencoder *oggEncoder_A = new OGGencoder(pParams, szFile);

  if(!oggEncoder_A)
  {
    MessageBox(NULL, "Error creating OGGencoder", "Error", MB_OK);
    return false;
  }

  oggEncoder_A->initLibvorbisenc();
  oggEncoder_A->initBitstream();

  return oggEncoder_A;
}


static BOOL writeDataToOgg(OGGencoder *ogg_encoder, signed char	*data)
{
   //writes the data to .ogg and .wav files
  if(ogg_encoder)
  {
    if(!ogg_encoder->encodeChunk(data)) 
    { 
      MessageBox(NULL, "Error writing data to OGG file", "Error", MB_OK);
      return false;
    }	
  }
  return true;
}


// If file to be closed is not NULL it will close it and will try to open a new file (in case not already opened)
static BOOL closeAndOpen
(
  MyMMFile    **ppWaveToBeClosed,   // [IN/OUT] if !=NULL file will be closed
  MyMMFile    **ppWaveToBeOpened,   // [IN/OUT] if ppWaveToBeClosed was succesfully closed, ppWaveToBeOpened will be opened but only if it is NULL
  OGGencoder  **ppOggToBeClosed,    // [IN/OUT] if !=NULL file will be closed
  OGGencoder  **ppOggToBeOpened,    // [IN/OUT] if ppOggToBeClosed was succesfully closed, ppOggToBeOpened will be opened but only if it is NULL
  pPARAMS     pParams               // [IN]
)
{
  if(*ppWaveToBeClosed)
  {
    delete(*ppWaveToBeClosed); 
    *ppWaveToBeClosed = NULL;
    if(*ppWaveToBeOpened == NULL)
    {
      *ppWaveToBeOpened = createWavFile(pParams);
      if(!(*ppWaveToBeOpened)) 
        return FALSE; 
    }
  }

  if(*ppOggToBeClosed)
  {
    delete(*ppOggToBeClosed); 
    *ppOggToBeClosed = NULL;
    if(*ppOggToBeOpened == NULL)
    {
      *ppOggToBeOpened = createOggEncoder(pParams);
      if(!(*ppOggToBeOpened)) 
        return FALSE;
    }
  }

  return TRUE;
}


// If file to be checked is not NULL it will try to open a new file (in case not already opened)
static BOOL openWithoutClosing
(
  MyMMFile    *pWaveToBeChecked,   // [IN] ppWaveToBeOpened will be open only if this is currently open
  MyMMFile    **ppWaveToBeOpened,  // [IN/OUT] new file to open if the supplied one is NULL
  OGGencoder  *pOggToBeChecked,    // [IN] ppOggToBeOpened will be open only if this is currently open
  OGGencoder  **ppOggToBeOpened,   // [IN/OUT] new file to open if the supplied one is NULL
  pPARAMS     pParams              // [IN]
)
{
  if(pWaveToBeChecked)
  {
    if(*ppWaveToBeOpened == NULL)
    {
      *ppWaveToBeOpened = createWavFile(pParams);
      if(!(*ppWaveToBeOpened)) 
        return FALSE; 
    }
  }

  if(pOggToBeChecked)
  {
    if(*ppOggToBeOpened == NULL)
    {
      *ppOggToBeOpened = createOggEncoder(pParams);
      if(!(*ppOggToBeOpened)) 
        return FALSE;
    }
  }

  return TRUE;
}


void Thread(PVOID input)
{                  
  signed char	  *pCaptureBuffer;	    
  volatile      pPARAMS	pParams = (pPARAMS)input;	
  int					  iCaptureBufferSize = (pParams->iSampleRate*pParams->iNumberOfChannels*pParams->iBitsPerSample) / 8; //Buffer size for one second of sound data
  OGGencoder    *oggEncoder_A = NULL;
  OGGencoder    *oggEncoder_B = NULL;
  MyMMFile      *myMMFile_A   = NULL; 
  MyMMFile      *myMMFile_B   = NULL;
  int					  seconds_counter_A = 0;  // Keeps info for the current length of the files A
  int					  seconds_counter_B = 0;  // Keeps info for the current length of the files B

 
  MyRecorder *pMyRecorder = new MyRecorder(pParams);
	if(!pMyRecorder) {MessageBox(NULL, "Error MyRecorder", "Error",MB_OK); _endthread();}

	//Creates directory for sound files:
	if(!CreateDirectory(params.szDirectoryName,NULL))
  {
		if(GetLastError() != ERROR_ALREADY_EXISTS) 
    { 
      MessageBox(params.hwnd, "Couldn't create directory for audio files", "error", MB_OK);
      _endthread();
    }
	}

  if(params.iCompressionOnOFFBoth == WAV_ONLY || params.iCompressionOnOFFBoth == WAV_AND_OGG)
  {
    myMMFile_A = createWavFile(pParams);
    if(!myMMFile_A) {_endthread();}
  }

  if(params.iCompressionOnOFFBoth == OGG_ONLY || params.iCompressionOnOFFBoth == WAV_AND_OGG)
  {
    oggEncoder_A = createOggEncoder(pParams);
    if(!oggEncoder_A) {_endthread();}
  }

  //Starting to read from the DirectSound Buffer
  while(!pParams->bKill)
  {
    //**------W8 for 500ms of data -----**/
    WaitForSingleObject(pMyRecorder->rghEvent[0], INFINITE); 
    ResetEvent(pMyRecorder->rghEvent[0]);
    pCaptureBuffer = pMyRecorder->ReadFromSoundCard(0);
    // Write data to files which are open
    if(!writeDataToWav(myMMFile_A, pCaptureBuffer)) 
      break;
    if(!writeDataToWav(myMMFile_B, pCaptureBuffer)) 
      break;
    if(!writeDataToOgg(oggEncoder_A, pCaptureBuffer))
      break;
    if(!writeDataToOgg(oggEncoder_B, pCaptureBuffer))
      break;
    
    //**------W8 for 500ms of data -----**/
    WaitForSingleObject(pMyRecorder->rghEvent[1], INFINITE); //w8 for the second event
    ResetEvent(pMyRecorder->rghEvent[1]);
    pCaptureBuffer= pMyRecorder->ReadFromSoundCard(iCaptureBufferSize/2);
     // Write data to files which are open
    if(!writeDataToWav(myMMFile_A, pCaptureBuffer)) 
      break;
    if(!writeDataToWav(myMMFile_B, pCaptureBuffer)) 
      break;
    if(!writeDataToOgg(oggEncoder_A, pCaptureBuffer))
      break;
    if(!writeDataToOgg(oggEncoder_B, pCaptureBuffer))
      break;

    // Increment values for active files
    if(myMMFile_A || oggEncoder_A)
      seconds_counter_A++;
    if(myMMFile_B || oggEncoder_B)
      seconds_counter_B++;
    
    // If the file(s) reaches the needed size, close current and create next one(s) 
    if(seconds_counter_A >= pParams->iFileSizeInSeconds)
    {
      pParams->iFilesWritten++;
      seconds_counter_A = 0;

      SendMessage(hwndDisplay,WM_INITDIALOG,0,0); // Send message to the display dialog box to update how many files have been written

      // Close A(is possible) and open B(if possible)
      if(!closeAndOpen(&myMMFile_A, &myMMFile_B, &oggEncoder_A, &oggEncoder_B, pParams))
        _endthread();
		}

    // If the file(s) reaches the needed size, close current and create next one(s) 
    if(seconds_counter_B >= pParams->iFileSizeInSeconds)
    {
      pParams->iFilesWritten++;
      seconds_counter_B = 0;

      SendMessage(hwndDisplay,WM_INITDIALOG,0,0); // Send message to the display dialog box to update how many files have been written

      // Close B(is possible) and open A(if possible)
      if(!closeAndOpen(&myMMFile_B, &myMMFile_A, &oggEncoder_B, &oggEncoder_A, pParams))
        _endthread();
		}

    // Overlap is enabled - time to open B file(s) if not already open
    if( seconds_counter_A >= (pParams->iFileSizeInSeconds - pParams->iOverlapInSeconds) )
    {
      if(!openWithoutClosing(myMMFile_A, &myMMFile_B, oggEncoder_A, &oggEncoder_B, pParams))
        _endthread();
    }

    // Overlap is enabled - time to open A file(s) if not already open
    if( seconds_counter_B >= (pParams->iFileSizeInSeconds - pParams->iOverlapInSeconds) )
    {
      if(!openWithoutClosing(myMMFile_B, &myMMFile_A, oggEncoder_B, &oggEncoder_A, pParams))
        _endthread();
    }

	} 
	
  if(myMMFile_A)
    myMMFile_A->CloseMMFile();
  if(myMMFile_B)
    myMMFile_B->CloseMMFile();
  if(oggEncoder_A)
    delete(oggEncoder_A);
  if(oggEncoder_B)
    delete(oggEncoder_B);
		
	pMyRecorder->ShutDown();
	//MessageBox(pParams->hwnd, "thread: You stopped the recorder!", "Note", MB_OK);
	_endthread();
}

enum WINDOW_IDENTIFIER 
{ 
  START_BUTTON = 1,
  STOP_BUTTON,
  FILE_LENGTH_EDIT_BUTTON,
  FILE_OVERLAP_EDIT_BUTTON,
  SAMPLE_RATE_COMBOBOX,
  MONO_BUTTON,
  STEREO_BUTTON,
  BITS_PER_SAMPLE_COMBOBOX,
  CAPTURE_DEVICE_BUTTON,
  DEVICE_CAPS_BUTTON,
  OGG_COMPRESSION_BUTTON,
  NEED_WAV_BUTTON,
  COMPRESSION_QUALITY_EDIT_BUTTON
};


LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	 static         HINSTANCE	hInstance;
	 HDC				    hdc;
   PAINTSTRUCT		ps;	
	 static HWND		hStart, hStop, hSampleRate, hFileSize, hOverlapping;
	 static HWND		hMono, hStereo, hBits, hRecordingControl,hDevCaps;
	 static HWND    hOgg,hOggQuality,hOggNeedWav;
	 static HBRUSH  hBrushStatic;
	 
	 static bool		bOgg, bStillNeedWav; // used for the two checkboxes 
	 static BOOL		bRecording; // if its TRUE there is a thread running

	 //for directory select dialog box
	 static BROWSEINFO	browseInfo;
	 LPITEMIDLIST		lpItemId;
     
	 //temps
	 int				    iTemp;
	 BOOL				    bTemp;
	 TCHAR				  szBuffer[MAX_PATH];
	 
	 switch (message)
   {
     case WM_CREATE:
		  hInstance=((LPCREATESTRUCT) lParam)->hInstance;
		  //---------------------Adding buttons------------------------------------------
		  //Start
		  hStart=CreateWindow("button", "Start", WS_CHILD | WS_VISIBLE | WS_BORDER,
						20,270,70,30,hwnd,(HMENU)START_BUTTON,hInstance ,NULL);
		  //Stop
		  hStop=CreateWindow("button", "Stop", WS_CHILD | WS_VISIBLE | WS_BORDER,
						150,270,70,30,hwnd,(HMENU)STOP_BUTTON, hInstance,NULL);
		  //File size
		  sprintf(szBuffer, "%d", DEFAULT_LENGTH_IN_SECS);
		  hFileSize=CreateWindow("edit", TEXT(szBuffer), WS_CHILD | WS_VISIBLE | WS_BORDER  | ES_RIGHT,
						20,6,70,20,hwnd,(HMENU)FILE_LENGTH_EDIT_BUTTON, ((LPCREATESTRUCT) lParam)->hInstance,NULL);
      //File Overlapping
      sprintf(szBuffer, "%d", DEFAULT_OVERLAPPING_IN_SECS);
		  hOverlapping = CreateWindow("edit", 
                                  TEXT(szBuffer), 
                                  WS_CHILD | WS_VISIBLE | WS_BORDER  | ES_RIGHT,
						                      20,32,50,20,
                                  hwnd,
                                  (HMENU)FILE_OVERLAP_EDIT_BUTTON, 
                                  ((LPCREATESTRUCT) lParam)->hInstance,NULL);
		  //Sample rate
		  hSampleRate=CreateWindow("combobox", "sample rate", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
						20,58,88,300,hwnd,(HMENU)SAMPLE_RATE_COMBOBOX, hInstance,NULL);
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("8000"));
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("11024"));
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("22050"));
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("44100"));
      SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("48000"));
      SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("96000"));
		  SendMessage(hSampleRate, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)TEXT("11024"));
		  
		  //Mono
		  hMono=CreateWindow("button", "Mono", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_BORDER,
        20,89,70,22,hwnd,(HMENU)MONO_BUTTON, hInstance,NULL);
		  SendMessage(hMono, BM_SETCHECK, 1,0);//sets the Mono button checked
		  //Stereo
		  hStereo=CreateWindow("button", "Stereo", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_BORDER,
						100,89,70,22,hwnd,(HMENU)STEREO_BUTTON, hInstance,NULL);
		  //Bits per sample
		  hBits=CreateWindow("combobox", "bits per sample", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
						20,119,50,60,hwnd,(HMENU)BITS_PER_SAMPLE_COMBOBOX, hInstance,NULL);
		  SendMessage(hBits,CB_ADDSTRING,0,(LPARAM)TEXT("8"));
		  SendMessage(hBits,CB_ADDSTRING,0,(LPARAM)TEXT("16"));
		  SendMessage(hBits, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)TEXT("16"));
		  
		 
		 
		  //Recording control
		  hRecordingControl=CreateWindow("button", "Select capture device", WS_CHILD | WS_VISIBLE | WS_BORDER,
						275,5,170,22,hwnd,(HMENU)CAPTURE_DEVICE_BUTTON, hInstance,NULL);
		 
		  //Device Capabilities Dialog
		  hDevCaps=CreateWindow("button", "Device Capabilities", WS_CHILD | WS_VISIBLE | WS_BORDER,
						275,30,170,22,hwnd,(HMENU)DEVICE_CAPS_BUTTON, hInstance,NULL);

		  //Do you want ogg compression? button
		  hOgg=CreateWindow("button", "Ogg Vorbis compression", BS_CHECKBOX | WS_CHILD | WS_VISIBLE | WS_BORDER,
						260,75,198,22,hwnd,(HMENU)OGG_COMPRESSION_BUTTON, hInstance,NULL);
		  //Still need the .wav files
		  hOggNeedWav=CreateWindow("button", "", BS_CHECKBOX | WS_CHILD | WS_VISIBLE | WS_BORDER,
						261,190,15,15,hwnd,(HMENU)NEED_WAV_BUTTON, hInstance,NULL);

		  //for quality of the compression	
		  hOggQuality=CreateWindow("edit", "1", WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_BORDER, 342, 110 , 30, 15, hwnd,
			  (HMENU) COMPRESSION_QUALITY_EDIT_BUTTON, hInstance, NULL);
		  

		  /*---------------------Initializing  stuff-------------------------*/
		  
		  //Initializing browse information for Folder Dialog box
		  browseInfo.hwndOwner=hwnd;
		  browseInfo.pidlRoot=NULL;
		  browseInfo.lpszTitle="Choose the output directory";
		  browseInfo.lpfn=NULL;

		  //Initializing the configuration PARAMS
      params.deviceLPGUID = NULL;
		  params.iBitsPerSample=16;
		  params.iFileSizeInSeconds=DEFAULT_LENGTH_IN_SECS;
		  params.iSampleRate=11024;
		  params.iNumberOfChannels = 1;
		  bRecording=0;
		  params.hwnd=hwnd;
		  params.bKill=0;
		  params.qualityOfOggCompression = 0.1f;  
      params.iCompressionOnOFFBoth = WAV_ONLY;
		  
		  if(!GetCurrentDirectory(256,szBuffer)){
			//if its not possible to determine the current dir, choosing output dir is mandatory//
			  MessageBox(params.hwnd, "Couldn't read the current directory. Choose output dir now!", "Error", MB_OK);
			 
			  lpItemId=SHBrowseForFolder(&browseInfo);
			  SHGetPathFromIDList(lpItemId,szBuffer);
			  StringCchCat(szBuffer,MAX_PATH,TEXT("\\"));
			  StringCchCopy(params.szDirectoryName,MAX_PATH,szBuffer);
		  }
		  else{//set the output dir to  ...\current folder\Audio Files Here
			  StringCchCat(szBuffer,MAX_PATH,TEXT("\\Audio Files Here\\"));	
			  StringCchCopy(params.szDirectoryName,MAX_PATH,szBuffer);
		  }
		  
		  //------
		  bOgg=false;			//no compression at startup			
		  bStillNeedWav=false;	//StillneedWav option is off 

		  //Disabling the uneeded compression buttons
		  EnableWindow(hOggQuality,0);
		  EnableWindow(hOggNeedWav,0);
		  	  
		  //opening the display dialog box
		  hwndDisplay1=CreateDialog(hInstance,(LPCSTR)IDD_DIALOGDISPLAY1,hwnd, AboutDisplay1);
		  SetFocus(hwnd);

		  return 0 ;

     case WM_PAINT:
      hdc = BeginPaint (hwnd, &ps) ;
		  
		  SetBkColor(hdc, RGB(184,200,242));
		  TextOut(hdc,100, 7, "File size in seconds", (int)strlen("File size in seconds"));
      TextOut(hdc,80, 34, "File overlap in seconds", (int)strlen("File overlap in seconds"));
		  TextOut(hdc,115, 60, "Sample rate in Hz", (int)strlen("Sample rate in Hz"));
		  TextOut(hdc, 75, 122, "Bits per sample", (int)strlen("Bits per sample"));
		  

		  MoveToEx (hdc,255, 70, NULL) ;
		  LineTo (hdc, 462, 70) ;
		  LineTo (hdc, 462, 250) ;
		  LineTo (hdc, 255, 250) ;
		  LineTo (hdc, 255, 70) ;
		  TextOut(hdc, 270, 128, "Choose the needed quality", (int) strlen("Choose the needed quality"));	  
		  TextOut(hdc, 315, 145, "(from -1 to 10)", (int)  strlen("(from -1 to 10)"));	  
		  TextOut(hdc, 278, 190, "But I still need the .wav files", (int) strlen("But i still need the .wav files"));	  
		  
		  EndPaint (hwnd, &ps) ;
      return 0 ;
	 
	 case WM_COMMAND:
		  switch (LOWORD (wParam))
		  { 
      case START_BUTTON:

				if(bRecording) //if its recording already break;
					break;

				if(GetDlgItemInt(hwnd,3,&bTemp,0) < MIN_LENGTH_IN_SECS)//if the user has chosed a file smaller then MIN seconds, warn and break;
				{
					sprintf(szBuffer, "Files can not be smaller then %d seconds.", MIN_LENGTH_IN_SECS);
					MessageBox(hwnd, TEXT(szBuffer), "Warning",  MB_OK | MB_ICONWARNING);
					break;
				}
				
				
				params.iFilesWritten=1; // It is the first file written
        bRecording=1;//recording flag on;
				params.bKill=0;//do not close the thread;

				SendMessage(hwndDisplay1,WM_INITDIALOG,0,0);
				ShowWindow(hwndDisplay1,SW_HIDE);
				
				if(!_beginthread(Thread,0,&params))
        {//if error
					MessageBox(hwnd, "Could not start the recorder","Error", MB_OK);
					bRecording=0;
					break;
				}
				//Disable the unneeded buttons
				EnableWindow(hStart,0);
				EnableWindow(hSampleRate,0);
				EnableWindow(hFileSize,0);
        EnableWindow(hOverlapping,0);
				EnableWindow(hMono,0);
				EnableWindow(hStereo,0);
				EnableWindow(hBits,0);
				EnableWindow(hOgg,0);
				EnableWindow(hOggQuality,0);
				EnableWindow(hOggNeedWav,0);
        EnableWindow(hRecordingControl,0);
        EnableWindow(hDevCaps,0);
				EnableWindow(hStop,1);
                
				
				//create display dialog
				hwndDisplay=CreateDialog(hInstance, (LPSTR)IDD_DIALOGDISPLAY, hwnd, AboutDisplay);
				SetFocus(hwnd);
				break;


      case STOP_BUTTON:
				if(!bRecording) break;//if not recording break;
				params.bKill=1;//close the thread
				SendMessage(hwndDisplay,WM_CLOSE,0,0);//close the display dialog
				ShowWindow(hwndDisplay1,SW_SHOWNORMAL);//open the first display
				SetFocus(hwnd);
				
				
				//Enable the buttons
				EnableWindow(hStart,1);
				EnableWindow(hSampleRate,1);
				EnableWindow(hFileSize,1);
        EnableWindow(hOverlapping,1);
				EnableWindow(hMono,1);
				EnableWindow(hStereo,1);
				EnableWindow(hBits,1);
        EnableWindow(hRecordingControl,1);
        EnableWindow(hDevCaps,1);
				EnableWindow(hOgg,1);
				EnableWindow(hStop,0);
				if(bOgg)
        {
					EnableWindow(hOggQuality,1);
					EnableWindow(hOggNeedWav,1);
				}

				Sleep(1001); //sleep for one second so the thread is closed for sure.
        bRecording=0;//not recording flag
				break;
				

      case MONO_BUTTON:
				if(params.iNumberOfChannels == 2)
        {
					SendMessage(hStereo, BM_SETCHECK, 0, 0);
					SendMessage(hMono, BM_SETCHECK, 1, 0);
					params.iNumberOfChannels=1;
				}
				SendMessage(hwndDisplay1,WM_COMMAND,33,0);//send message to Display1 so it can update the filesize MB/min
				break;


      case STEREO_BUTTON:
				if(params.iNumberOfChannels == 1)
        {
					SendMessage(hMono, BM_SETCHECK, 0,0);
					SendMessage(hStereo, BM_SETCHECK, 1,0);	
					params.iNumberOfChannels=2;
				}
				SendMessage(hwndDisplay1,WM_COMMAND,33,0);//send message to Display1 so it can update the filesize MB/min
				break;


      case FILE_LENGTH_EDIT_BUTTON:
				if(HIWORD(wParam)==EN_UPDATE)
				{
					params.iFileSizeInSeconds=GetDlgItemInt(hwnd, FILE_LENGTH_EDIT_BUTTON, &bTemp, 0);
					if(!bTemp) //if it can't translate the value into number
					{
						params.iFileSizeInSeconds = DEFAULT_LENGTH_IN_SECS;
						sprintf(szBuffer, "%d", DEFAULT_LENGTH_IN_SECS);
						SetWindowText(hFileSize, TEXT(szBuffer));
            
					}
          else if(params.iFileSizeInSeconds < MIN_LENGTH_IN_SECS)
          {
            sprintf(szBuffer, "%d", MIN_LENGTH_IN_SECS);
						SetWindowText(hFileSize, TEXT(szBuffer));
          }

          // Send message to overlap button - in case the overlap value is not
          SendMessage(hwnd, 
                      WM_COMMAND, 
                      MAKEWPARAM(FILE_OVERLAP_EDIT_BUTTON, EN_UPDATE), 
                      NULL);
				}
				break;


      case FILE_OVERLAP_EDIT_BUTTON:
        if(HIWORD(wParam)==EN_UPDATE)
				{
					params.iOverlapInSeconds = GetDlgItemInt(hwnd, FILE_OVERLAP_EDIT_BUTTON, &bTemp, 0);
					if(!bTemp) //if it can't translate the value into number
					{
						params.iOverlapInSeconds = DEFAULT_OVERLAPPING_IN_SECS;
						sprintf(szBuffer, "%d", DEFAULT_OVERLAPPING_IN_SECS);
						SetWindowText(hOverlapping, TEXT(szBuffer));
            break;
					}
          if(params.iOverlapInSeconds > params.iFileSizeInSeconds/2)
          {
            params.iOverlapInSeconds = params.iFileSizeInSeconds/2;
            sprintf(szBuffer, "%d", params.iOverlapInSeconds);
						SetWindowText(hOverlapping, TEXT(szBuffer));
          }
				}
				break;


			case SAMPLE_RATE_COMBOBOX:
				if(HIWORD (wParam) == LBN_SELCHANGE)
					params.iSampleRate=GetDlgItemInt(hwnd, SAMPLE_RATE_COMBOBOX, &bTemp, 0);
				SendMessage(hwndDisplay1,WM_COMMAND,33,0);//send message to Display1 so it can update the filesize MB/min
				break;

      case BITS_PER_SAMPLE_COMBOBOX:
				if(HIWORD (wParam) == LBN_SELCHANGE)
					params.iBitsPerSample=GetDlgItemInt(hwnd, BITS_PER_SAMPLE_COMBOBOX, &bTemp,0);	
				SendMessage(hwndDisplay1,WM_COMMAND,33,0);
				break;

			case ID_HELP_ABOUT: //About dialogbox
				DialogBox(hInstance,(LPCSTR)IDD_DIALOGABOUT,hwnd,AboutAbout);
				break;

			case ID_FILE_QUIT: //Quit program
				SendMessage(hwnd,WM_DESTROY,0,0);
				break;

			case ID_HELP_INDEX40002:
				DialogBox(hInstance,(LPCSTR)IDD_DIALOGINDEX,hwnd,AboutIndex);
				break;

      case CAPTURE_DEVICE_BUTTON: //Select capture device (e.g sound card)from a dialog box
				DialogBox(hInstance,(LPCSTR)IDD_DIALOG_SELECT_DEVICE, hwnd, AboutCaptureDevice);
				break;

      case DEVICE_CAPS_BUTTON://Opens the dialog box with the device capabilities
				DialogBox(hInstance,(LPCSTR)IDD_DIALOGDEVCAPS, hwnd, AboutDevCaps);
				break;

      case OGG_COMPRESSION_BUTTON:
				if(bOgg == false) //compression ON
        {
					bOgg = true;
					if(bStillNeedWav)
            params.iCompressionOnOFFBoth = WAV_AND_OGG; 
					else 
            params.iCompressionOnOFFBoth = OGG_ONLY; 
					
					SendMessage(hOgg,BM_SETCHECK,1,0);
					/*enabling the buttons from the ogg compression part*/
					EnableWindow(hOggQuality,1);
					EnableWindow(hOggNeedWav,1);
				}
				else //compression OFF
        {		    
					bOgg=false;
					SendMessage(hOgg,BM_SETCHECK,0,0);
					
					/*Sets the quality to 0, so we know that we should
					  not compress the sound*/
          params.iCompressionOnOFFBoth = WAV_ONLY;  
					
					/*disabling the buttons from the ogg compression part*/
					EnableWindow(hOggQuality,0);
					EnableWindow(hOggNeedWav,0);
				}
				break;

			case NEED_WAV_BUTTON:
				if(bStillNeedWav == false)
        {
					bStillNeedWav=true;
					params.iCompressionOnOFFBoth = WAV_AND_OGG;
					SendMessage(hOggNeedWav,BM_SETCHECK,1,0);
				}
				else
        {
					bStillNeedWav=false;
          params.iCompressionOnOFFBoth = OGG_ONLY;
					SendMessage(hOggNeedWav,BM_SETCHECK,0,0);
				}
				break;

      case COMPRESSION_QUALITY_EDIT_BUTTON:
				if(HIWORD(wParam)==EN_KILLFOCUS)
				{
					iTemp=GetDlgItemInt(hwnd,12,&bTemp,true);
					if(!bTemp)
					{//if it can't translate the value into number
						params.qualityOfOggCompression=0.1f;
						SetWindowText(hOggQuality,"1");
						return 0;
					}
					if(translateToFloat(iTemp)==11){//the integer is not between -1 and 10, set the default value of 1
						params.qualityOfOggCompression=0.1f;
						SetWindowText(hOggQuality,"1");
						return 0;
					}
					params.qualityOfOggCompression=translateToFloat(iTemp);
				}
				break;
			case ID_FILE_SELECTOUTPUTDIRECTORY40005:
			
				lpItemId=SHBrowseForFolder(&browseInfo);
				SHGetPathFromIDList(lpItemId,szBuffer);
				StringCchCat(szBuffer,MAX_PATH,TEXT("\\"));
				StringCchCopy(params.szDirectoryName,MAX_PATH,szBuffer);
				break;
		  }
	 	  return 0; 

	 case WM_DESTROY:
          
		  params.bKill=0;
		  PostQuitMessage (0) ;
          return 0 ;
     }
     return DefWindowProc (hwnd, message, wParam, lParam) ;
}


BOOL CALLBACK AboutDisplay(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR szBuffer[MAX_PATH];

     switch (message)
     {
     case WM_INITDIALOG :
		  //fills the edit controls with information
		  
		  StringCchPrintf(szBuffer,MAX_PATH, TEXT("at %dHz"), params.iSampleRate);
		  SetWindowText(GetDlgItem(hDlg, IDC_EDIT1),szBuffer);
		 
      if(params.iNumberOfChannels==2)	
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT2),"Stereo");
      else 
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT2),"Mono");

		  StringCchPrintf(szBuffer,MAX_PATH, TEXT("%d Bits"), params.iBitsPerSample);
		  SetWindowText(GetDlgItem(hDlg, IDC_EDIT3),szBuffer);
		  
		  StringCchPrintf(szBuffer,MAX_PATH, TEXT("files written:%d"), params.iFilesWritten);
		  SetWindowText(GetDlgItem(hDlg, IDC_EDIT4),szBuffer);
		  if(params.iCompressionOnOFFBoth==2 || params.iCompressionOnOFFBoth==3){
			  SetWindowText(GetDlgItem(hDlg, IDC_EDIT5),"Vorbis: ON");
			  StringCchPrintf(szBuffer,MAX_PATH, TEXT("At quality:%.1f"), params.qualityOfOggCompression);
			  SetWindowText(GetDlgItem(hDlg, IDC_EDIT6),szBuffer);
		  }
		  return TRUE ;

	 case WM_CLOSE : 
		  DestroyWindow(hDlg);
		  return TRUE;
     }
     return FALSE ;
}


BOOL CALLBACK AboutAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    SetDlgItemText(hDlg, IDC_EDITABOUT, "Open source recording software github.com/potty-dzmeia/SoundRecorder");
    sprintf_s(szBuffer, "version:%.1f\n LZ1ABC %s", POTTY_RECORDER_VERSION, POTTY_RECORDER_VERSION_YEAR); 
    SetDlgItemText(hDlg, IDC_STATIC1, szBuffer); 
    return TRUE;
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK:
      EndDialog(hDlg,0);
      return TRUE;
    }
    break;
  }
  return FALSE ;

}


BOOL CALLBACK AboutIndex(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    SetWindowText(GetDlgItem(hDlg,IDC_EDIT1),"github.com/potty-dzmeia/SoundRecorder");
    return TRUE;
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDCLOSE:
      EndDialog(hDlg,0);
      return TRUE;
    }
    break;
  }
  return FALSE ;

}


BOOL CALLBACK AboutDisplay1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
  TCHAR	szBuffer[MAX_PATH];

  switch (message)
  {
  case WM_INITDIALOG:
    sprintf_s(szBuffer,"Wav file size: %.2f MB/sec",(((double)params.iBitsPerSample*params.iSampleRate*params.iNumberOfChannels)/8388608)*60);
    SetWindowText(GetDlgItem(hDlg,IDC_EDIT1),szBuffer);
    return TRUE;
  case WM_COMMAND:
    switch(wParam)
    {
    case 33:
      sprintf_s(szBuffer,"Wav file size: %.2f MB/min",(((double)params.iBitsPerSample*params.iSampleRate*params.iNumberOfChannels)/8388608)*60);
      SetWindowText(GetDlgItem(hDlg,IDC_EDIT1),szBuffer);
      return FALSE;
    }

    break;
  }
  return FALSE ;

}

BOOL CALLBACK AboutDevCaps(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){

  DSCCAPS dsccaps;
  MyRecorder *myRecorder = NULL;

  switch (message)
  {
  case WM_INITDIALOG:
    myRecorder = new MyRecorder(&params);
    if(!myRecorder)
    {
      MessageBox(hDlg, "Stop recording and try again", "Error", MB_OK);
      EndDialog(hDlg,0);
    }
    dsccaps=*myRecorder->GetDevCaps();


    SendDlgItemMessage(hDlg,IDC_EDIT1, EM_SETREADONLY,1,0);
    sprintf_s(szBuffer, "%u", dsccaps.dwChannels);
    SetWindowText(GetDlgItem(hDlg,IDC_EDIT1),szBuffer);

    if( dsccaps.dwFlags & DSCCAPS_CERTIFIED)
      SendMessage(GetDlgItem(hDlg,IDC_RADIO1),BM_SETCHECK,1,0);
    if( dsccaps.dwFlags & DSCCAPS_EMULDRIVER)
      SendMessage(GetDlgItem(hDlg,IDC_RADIO2),BM_SETCHECK,1,0);
    if ( dsccaps.dwFlags & DSCCAPS_MULTIPLECAPTURE)
      SendMessage(GetDlgItem(hDlg,IDC_RADIO3),BM_SETCHECK,1,0);



    if(dsccaps.dwFormats&WAVE_FORMAT_96S16)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"96 kHz, stereo, 16-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_96M16)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"96 kHz, mono, 16-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_96S08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"96 kHz, stereo, 8-bit"); 
    else if(dsccaps.dwFormats&WAVE_FORMAT_96M08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"96 kHz, mono, 8-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_4S16)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"44.1 kHz, stereo, 16-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_4S08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"44.1 kHz, stereo, 8-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_4M08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"44.1 kHz, mono, 8-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_2S16)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"22.05 kHz, stereo, 16-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_2S08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"22.05 kHz, stereo, 8-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_2M16)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"22.05 kHz, mono, 16-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_2M08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"22.05 kHz, mono, 8-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_1S16)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"11.025 kHz, stereo, 16-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_1S08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"11.025 kHz, stereo, 8-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_1M16)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"11.025 kHz, mono, 16-bit");
    else if(dsccaps.dwFormats&WAVE_FORMAT_1M08)
      SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)"11.025 kHz, mono, 8-bit");
    return TRUE;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK:
      if(!myRecorder)
        delete(myRecorder);
      EndDialog(hDlg,0);
      return TRUE;
    }
    break;
  }
  return FALSE ;

}

// Procedure for selecting capture device
BOOL CALLBACK AboutCaptureDevice(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  HWND        hComboBox;
  LRESULT     lResult, lResult1 ;

  hComboBox = GetDlgItem(hDlg, IDC_COMBO_CAPTUREDEVICE);

  switch (message)
  {
  case WM_INITDIALOG:         
    // Init the ComboBox
    //------------------------------------------------------------------------------------
    // Enumerate all sound capture devices and add them to the combobox
    DirectSoundCaptureEnumerate(&DSEnumProc,(VOID*)hComboBox);

    // Read the number of entries
    lResult = SendMessage(hComboBox, CB_GETCOUNT, 0, 0); 
    // Check wich capture device was selected previously and set it as an active selection
    for(int i = 0; i<lResult; i++)
    {
      // Check if the GUID is the same with the current selection
      lResult1 = SendMessage(hComboBox, CB_GETITEMDATA, i, 0);
      switch(lResult1)
      {
      case NULL: // Current selection is default          
        if (params.deviceLPGUID == NULL) // and the previous select was the default
          SendMessage(hComboBox, CB_SETCURSEL , i, 0); 
        break;
      default: // Current selection is notdefault
        if ( params.deviceLPGUID == NULL  ) //if the last selection was the default - break
          break;
        if( params.deviceLPGUID->Data1 == ((LPGUID)lResult1)->Data1 &&
          params.deviceLPGUID->Data2 == ((LPGUID)lResult1)->Data2   )
          SendMessage(hComboBox, CB_SETCURSEL , i, 0); 
        break;
      }//switch
    }   
    //End initing ComboBdox
    //------------------------------------------------------------------------------------
    return TRUE;


  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK:
      EndDialog(hDlg,0);
      return TRUE;
    case IDC_COMBO_CAPTUREDEVICE:
      // The selection in the combobox has changed
      if(HIWORD(wParam)== CBN_SELCHANGE)
      { 
        // Read the selected string ID
        lResult = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
        // Read the item attached to it
        params.deviceLPGUID = (LPGUID) SendMessage(hComboBox, CB_GETITEMDATA, lResult, 0);           
      }
      return TRUE;
    }
    break;
  case WM_CLOSE:
    EndDialog(hDlg,0);
    return TRUE;
  }
  return FALSE ;
}


// Enumerates all sound capture devices
BOOL CALLBACK DSEnumProc(LPGUID  lpGUID,      // Address of the GUID that identifies the device being enumerated, or NULL for the primary device. 
                         LPCTSTR lpszDesc,    // Address of a null-terminated string that provides a textual description of the DirectSound device.
                         LPCTSTR lpszDrvName, // Address of a null-terminated string that specifies the module name of the DirectSound driver corresponding to this device. 
                         LPVOID  lpContext)   // Address of application-defined data. This is the pointer passed to DirectSoundEnumerate or DirectSoundCaptureEnumerate as the lpContext parameter. 
{
  HWND    hCombo = (HWND)lpContext;
  LPGUID  lpTemp = NULL;
  LRESULT tempLResult;

  // If not the primary capture device copy the data from the structure
  if(lpGUID !=NULL) 
  {    
    lpTemp = (LPGUID)malloc(sizeof(GUID));
    if (lpTemp == NULL)
      return(TRUE);
    memcpy(lpTemp, lpGUID, sizeof(GUID));
  }

  // Add the capture device name to the combo box
  tempLResult = SendMessage((HWND)hCombo, (UINT)CB_ADDSTRING, 0, (LPARAM)lpszDesc);
  if(tempLResult == LB_ERR)
    MessageBox(NULL, TEXT("Couldn't add text to the combo box 1"), TEXT("Error"), MB_OK);

  // Add the associated value (the address of the first field of the structure)
  SendMessage(hCombo, 
    CB_SETITEMDATA, 
    tempLResult,
    (LPARAM) lpTemp);

  return(TRUE);
}


/*-----------------------Translates int values to float----------------*/
float translateToFloat(int i){
  if(i==-1)
    return -0.1f;
  else if(i==0)
    return 0;
  else if(i==1)
    return 0.1f;
  else if(i==2)
    return 0.2f;
  else if(i==3)
    return 0.3f;
  else if(i==4)
    return 0.4f;
  else if(i==5)
    return 0.5f;
  else if(i==6)
    return 0.6f;
  else if(i==7)
    return 0.7f;
  else if(i==8)
    return 0.8f;
  else if(i==9)
    return 0.9f;
  else if(i==10)
    return 1.0f;

  return 11;
}
