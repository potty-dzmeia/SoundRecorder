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

float translateToFloat(int);//Translates quality from -1to10 to -0.1to10

PARAMS		params;//params for the thread

HWND		hwndDisplay, hwndDisplay1;//The display windows

TCHAR				szBuffer[MAX_PATH];// char buffer used for different things
TCHAR				szBufferWavFile[MAX_PATH],szBufferOggFile[MAX_PATH];;// 

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{ 
	 //---------------------------------
	 static TCHAR szAppName[] = TEXT ("PottyRecorder b1.0") ;
     HWND         hwnd ;
     MSG          msg ;
     WNDCLASSEX   wndclass;
	 HMENU		  hMenu;
	  
	 wndclass.cbSize		= sizeof(wndclass);
     wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
     wndclass.lpfnWndProc   = WndProc ;
     wndclass.cbClsExtra    = 0 ;
     wndclass.cbWndExtra    = 0 ;
     wndclass.hInstance     = hInstance ;
     wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON1)) ;
     wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
     wndclass.hbrBackground = (HBRUSH)CreateSolidBrush (RGB(184,200,242)) ;
     wndclass.lpszMenuName  = NULL ;
     wndclass.lpszClassName = szAppName ;
	 wndclass.hIconSm		= NULL;

     if (!RegisterClassEx (&wndclass))
     {
          MessageBox (NULL, TEXT ("This program requires Windows NT!"), 
                      szAppName, MB_ICONERROR) ;
          return 0 ;
     }

	 hMenu=LoadMenu(hInstance, MAKEINTRESOURCE (IDR_MENU1));

     hwnd = CreateWindow (szAppName,                  // window class name
                          TEXT ("PottyRecorder b1.0                     Ham Radio software"), // window caption
                          WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,// window style
                          CW_USEDEFAULT,              // initial x position
                          CW_USEDEFAULT,              // initial y position
                          500,						  // initial x size
                          300,						  // initial y size
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

void Thread(PVOID pParameter){
	
	volatile pPARAMS	pParams;	// Must be volatile
	int					i=1;		//keeps info for the length of the file, when i==60 make new file
	SYSTEMTIME			tS;			//structure for the time;
	int					iCaptureBufferSize;//the size of the CaptureBuffer
	int					iFileSizeInSeconds;
	signed char			*pByteBuffer;		
 
	pParams=(pPARAMS)pParameter;

	iCaptureBufferSize=(pParams->iSampleRate*pParams->iStereo*pParams->iBitsPerSample)/8; //calculates the buffer size for one second
	iFileSizeInSeconds=pParams->iFileSize;//how big must be the file(I am using second variable, cause the user may change value durring recording)

	
	//Creates directory for sound files:
	if(!CreateDirectory(params.szDirectoryName,NULL)){
		
		if(GetLastError()==ERROR_ALREADY_EXISTS){
			//continiue;
		}
		else {
			MessageBox(params.hwnd, "Couldn't create directory for audio files", "error", MB_OK);
			_endthread();
		}
	}

	//Reads the local time and creates file with the specific name.....
	GetLocalTime(&tS);
	StringCchPrintf(szBufferWavFile, MAX_PATH, TEXT("%s%u-%.2u-%.2u %.2u%.2u.wav"), pParams->szDirectoryName,tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute);
	StringCchPrintf(szBufferOggFile, MAX_PATH, TEXT("%s%u-%.2u-%.2u %.2u%.2u.ogg"), pParams->szDirectoryName,tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute);
	
	
	//--Create MyRecroder object--
	MyRecorder *pMyRecorder=new MyRecorder(pParams);
	if(!pMyRecorder){
			MessageBox(pParams->hwnd, "Couldn't create MySound object, possibly out of memory", "Error",MB_OK | MB_ICONEXCLAMATION);
			_endthread();
	}

	/*-----------------------------------------------------------------|
	|			Here is the case when no compression is needed.        |	 
	|			The output is only .wav files.						   |
	|-------------------------------------------------------------------*/
	if(params.iCompressionOnOFFBoth == WAV_ONLY){
	
		//--Create MMFile object--
		MyMMFile *myMMFile=new MyMMFile(pParams,szBufferWavFile);
		if(!myMMFile){
			MessageBox(pParams->hwnd, "Error when trying to create file object", "Error", MB_OK | MB_ICONEXCLAMATION);
			_endthread();	
		}

		//Starting to read from the DirectSound Buffer
		while(!pParams->bKill){
			i++;
					//**-------EVENT1-------**/
			WaitForSingleObject(pMyRecorder->rghEvent[0], INFINITE);//w8 for the first event
			ResetEvent(pMyRecorder->rghEvent[0]);
		
			if(!myMMFile->AppendDataToFile(pMyRecorder->ReadFromSoundCard(0))){
				MessageBox(pParams->hwnd, "Error with the ReadFromSoundCard function + Append to file", "Error", MB_OK | MB_ICONEXCLAMATION);
				break;	
			}
					//**-------EVENT2------**/
			WaitForSingleObject(pMyRecorder->rghEvent[1], INFINITE);//w8 for the second event
			ResetEvent(pMyRecorder->rghEvent[1]);
		
			if(!myMMFile->AppendDataToFile(pMyRecorder->ReadFromSoundCard(iCaptureBufferSize/2))){
				MessageBox(pParams->hwnd, "Error with the ReadFromSoundCard function + Append to file", "Error", MB_OK | MB_ICONEXCLAMATION);
				break;	
			}
		
			//Create New FILE
			if(i>iFileSizeInSeconds){
				pParams->iFilesWritten++;
				i=1;//reset i;
	
				//Send message to the display dialog box to update how many files have been written
				SendMessage(hwndDisplay,WM_INITDIALOG,0,0);
				
				/** new MyMMFile object**/
				if(!myMMFile->CloseMMFile()){//close the old file
					MessageBox(pParams->hwnd, "Error closing file", "Error", MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				
				GetLocalTime(&tS);				//open new file with the new name		
				StringCchPrintf(szBufferWavFile,MAX_PATH,TEXT("%s%u-%.2u-%.2u %.2u%.2u.wav"), pParams->szDirectoryName,tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute);
				if(!myMMFile->OpenMMFileWrite(szBufferWavFile)){
					MessageBox(pParams->hwnd, "Error opening file", "Error", MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				if(!myMMFile->WriteHeader()){//writes the header of teh .wav file
					MessageBox(pParams->hwnd, "Error opening file", "Error", MB_OK | MB_ICONEXCLAMATION);
					break;
				}
			}
		}//while
		myMMFile->CloseMMFile();
	}/*end of WAV_ONLY procedure*/

	/*-----------------------------------------------------------------|
	|			Here is the case when compression is needed(ON).	   |	 
	|			The output is only .ogg vorbis files.				   |
	|-------------------------------------------------------------------*/
	else if(params.iCompressionOnOFFBoth == OGG_ONLY){
		//--Create OGGencoder object--
		OGGencoder *oggEncoder=new OGGencoder(pParams, szBufferOggFile);
		if(!oggEncoder){
			 MessageBox(pParams->hwnd, "Couldn't create OGGencoder object, possibly out of memory", "Error",MB_OK | MB_ICONEXCLAMATION); 
			 _endthread();
		 }
	 
		 oggEncoder->initLibvorbisenc();
		 oggEncoder->initBitstream();
	
		//Starting to read from the DirectSound Buffer
		while(!pParams->bKill){
			i++;
					//**--------EVENT1--------**/
			WaitForSingleObject(pMyRecorder->rghEvent[0], INFINITE);//w8 for the first event
			ResetEvent(pMyRecorder->rghEvent[0]);
			oggEncoder->encodeChunk(pMyRecorder->ReadFromSoundCard(0));
					
					//**-------EVENT2---------**/
			WaitForSingleObject(pMyRecorder->rghEvent[1], INFINITE);//w8 for the second event
			ResetEvent(pMyRecorder->rghEvent[1]);
			oggEncoder->encodeChunk(pMyRecorder->ReadFromSoundCard(iCaptureBufferSize/2));
			
			//if the file reaches the needed size, create another file
			if(i>iFileSizeInSeconds){
				pParams->iFilesWritten++;
				i=1;//reset i;
			
				//Send message to the display dialog box to update so current settings are displayed
				SendMessage(hwndDisplay,WM_INITDIALOG,0,0);

				GetLocalTime(&tS);				//open new file with the new name		
				StringCchPrintf(szBufferOggFile,MAX_PATH,TEXT("%s%u-%.2u-%.2u %.2u%.2u.ogg"), pParams->szDirectoryName,tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute);
			
				//--Create OGGencoder object so that a new file is created--
				delete(oggEncoder);
				OGGencoder *oggEncoder=new OGGencoder(&params,szBufferOggFile);
				if(!oggEncoder){
					 MessageBox(pParams->hwnd, "Couldn't create OGGencoder object, possibly out of memory", "Error",MB_OK | MB_ICONEXCLAMATION); 
					 _endthread();
				 }
				oggEncoder->initLibvorbisenc();
				oggEncoder->initBitstream();
			}
		}//while
		if(oggEncoder)
			delete(oggEncoder);
	}/*end of procedure when only compressed files are needed*/
	
	/*------------------------------------------------------------------|
	|			Here is the case when compressed and not compressed	    |
	|			files are needed(Both)									|	 
	|			The output is  .ogg vorbis files and .wav files.	    |
	|-------------------------------------------------------------------*/
	else{
		//--Create OGGencoder object--
		OGGencoder *oggEncoder=new OGGencoder(&params,szBufferOggFile);
		if(!oggEncoder){
			 MessageBox(pParams->hwnd, "Couldn't create OGGencoder object, possibly out of memory", "Error",MB_OK | MB_ICONEXCLAMATION); 
			 _endthread();
		 }
		oggEncoder->initLibvorbisenc();
		oggEncoder->initBitstream();
		
		//--Create MMFile object--
		MyMMFile *myMMFile=new MyMMFile(pParams,szBufferWavFile);
		if(!myMMFile){
			MessageBox(pParams->hwnd, "Error when trying to create file object", "Error", MB_OK | MB_ICONEXCLAMATION);
			_endthread();	
		}
	 
	
		//Starting to read from the DirectSound Buffer
		while(!pParams->bKill){
			i++;
					//**------EVENT1-----**/
			WaitForSingleObject(pMyRecorder->rghEvent[0], INFINITE);//w8 for the first event
			ResetEvent(pMyRecorder->rghEvent[0]);
			pByteBuffer=pMyRecorder->ReadFromSoundCard(0);
			
			//writes the data to .ogg and .wav files
			oggEncoder->encodeChunk(pByteBuffer);
			if(!myMMFile->AppendDataToFile(pByteBuffer)){
				MessageBox(pParams->hwnd, "Error with the ReadFromSoundCard function + Append to file", "Error", MB_OK | MB_ICONEXCLAMATION);
				break;	
			}		
					//**------EVENT2------**/
			WaitForSingleObject(pMyRecorder->rghEvent[1], INFINITE);//w8 for the second event
			ResetEvent(pMyRecorder->rghEvent[1]);
			pByteBuffer=pMyRecorder->ReadFromSoundCard(iCaptureBufferSize/2);
			
			oggEncoder->encodeChunk(pByteBuffer);
			if(!myMMFile->AppendDataToFile(pByteBuffer)){
				MessageBox(pParams->hwnd, "Error with the ReadFromSoundCard function + Append to file", "Error", MB_OK | MB_ICONEXCLAMATION);
				break;	
			}		
			//if the file reaches the needed size, create another file
			if(i>iFileSizeInSeconds){
				pParams->iFilesWritten++;
				i=1;//reset i;
			
				//Send message to the display dialog box to update how many files have been written
				SendMessage(hwndDisplay,WM_INITDIALOG,0,0);

				/** New .wav file**/
				GetLocalTime(&tS);				//open new file with the new name		
				StringCchPrintf(szBufferWavFile, MAX_PATH, TEXT("%s%u-%.2u-%.2u %.2u%.2u.wav"), pParams->szDirectoryName,tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute);
				StringCchPrintf(szBufferOggFile, MAX_PATH, TEXT("%s%u-%.2u-%.2u %.2u%.2u.ogg"), pParams->szDirectoryName,tS.wYear,tS.wMonth,tS.wDay,tS.wHour,tS.wMinute);
			
				if(!myMMFile->CloseMMFile()){//close the old file
					MessageBox(pParams->hwnd, "Error closing file", "Error", MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				if(!myMMFile->OpenMMFileWrite(szBufferWavFile)){
					MessageBox(pParams->hwnd, "Error opening file", "Error", MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				if(!myMMFile->WriteHeader()){//writes the header of teh .wav file
					MessageBox(pParams->hwnd, "Error opening file", "Error", MB_OK | MB_ICONEXCLAMATION);
					break;
				}


				//--Create OGGencoder object so that a new file is created--
				delete(oggEncoder);
				OGGencoder *oggEncoder=new OGGencoder(&params,szBufferOggFile);
				if(!oggEncoder){
					 MessageBox(pParams->hwnd, "Couldn't create OGGencoder object, possibly out of memory", "Error",MB_OK | MB_ICONEXCLAMATION); 
					 _endthread();
				 }
				oggEncoder->initLibvorbisenc();
				oggEncoder->initBitstream();
			}
		}//while
		myMMFile->CloseMMFile();
		if(oggEncoder)
			delete(oggEncoder);
	}/*end of procedure when only compressed files are needed*/
	
		
	pMyRecorder->ShutDown();
	//MessageBox(pParams->hwnd, "thread: You stopped the recorder!", "Note", MB_OK);
	_endthread();
}



LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	 static HINSTANCE	hInstance;
	 HDC				hdc;
     PAINTSTRUCT		ps;	
	 static HWND		hStart, hStop, hSampleRate, hFileSize;
	 static HWND		hMono, hStereo, hBits, hRecordingControl,hDevCaps;
	 static BOOL		bStereo;//Flag used for the Mono and Stereo buttons
	 static HWND        hOgg,hOggQuality,hOggNeedWav;
	 static HBRUSH		hBrushStatic;
	 //OSVERSIONINFO		osVersionInfo;
	 
	 static bool		bOgg, bStillNeedWav;//used for the two checkboxes 

	 static BOOL		bRecording;//if its TRUE there is a thread running

	 //for directory select dialog box
	 static BROWSEINFO	browseInfo;
	 LPITEMIDLIST		lpItemId;
     
	 //temps
	 int				iTemp;
	 BOOL				bTemp;
	 TCHAR				szBuffer[MAX_PATH];
	 
	 switch (message)
     {
     case WM_CREATE:
		  hInstance=((LPCREATESTRUCT) lParam)->hInstance;
		  //---------------------Adding buttons------------------------------------------
		  //Start
		  hStart=CreateWindow("button", "Start", WS_CHILD | WS_VISIBLE | WS_BORDER,
						20,220,70,30,hwnd,(HMENU)1,hInstance ,NULL);
		  //Stop
		  hStop=CreateWindow("button", "Stop", WS_CHILD | WS_VISIBLE | WS_BORDER,
						150,220,70,30,hwnd,(HMENU)2, hInstance,NULL);
		  //File size
		  hFileSize=CreateWindow("edit", "60", WS_CHILD | WS_VISIBLE | WS_BORDER  | ES_RIGHT,
						20,6,70,20,hwnd,(HMENU)3, ((LPCREATESTRUCT) lParam)->hInstance,NULL);
		  //Sample rate
		  hSampleRate=CreateWindow("combobox", "sample rate", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
						20,32,88,300,hwnd,(HMENU)4, hInstance,NULL);
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("8000"));
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("11024"));
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("22050"));
		  SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("44100"));
          SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("48000"));
          SendMessage(hSampleRate,CB_ADDSTRING,0,(LPARAM)TEXT("96000"));
		  SendMessage(hSampleRate, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)TEXT("11024"));
		  
		  //Mono
		  hMono=CreateWindow("button", "Mono", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_BORDER,
						20,63,70,22,hwnd,(HMENU)5, hInstance,NULL);
		  SendMessage(hMono, BM_SETCHECK, 1,0);//sets the Mono button checked
		  bStereo=0;
		  //Stereo
		  hStereo=CreateWindow("button", "Stereo", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_BORDER,
						100,63,70,22,hwnd,(HMENU)6, hInstance,NULL);
		  //Bits per sample
		  hBits=CreateWindow("combobox", "bits per sample", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
						20,93,50,60,hwnd,(HMENU)7, hInstance,NULL);
		  SendMessage(hBits,CB_ADDSTRING,0,(LPARAM)TEXT("8"));
		  SendMessage(hBits,CB_ADDSTRING,0,(LPARAM)TEXT("16"));
		  SendMessage(hBits, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)TEXT("16"));
		  //hBitmap=LoadBitmap(hInstance, (LPCTSTR) IDB_BITMAP1 );
		 
		  //Recording control
		  hRecordingControl=CreateWindow("button", "Select capture device", WS_CHILD | WS_VISIBLE | WS_BORDER,
						275,5,170,22,hwnd,(HMENU)8, hInstance,NULL);
		 
		  //Device Capabilities Dialog
		  hDevCaps=CreateWindow("button", "Device Capabilities", WS_CHILD | WS_VISIBLE | WS_BORDER,
						275,30,170,22,hwnd,(HMENU)9, hInstance,NULL);

		  //Do you want ogg compression? button
		  hOgg=CreateWindow("button", "Ogg Vorbis compression", BS_CHECKBOX | WS_CHILD | WS_VISIBLE | WS_BORDER,
						260,75,198,22,hwnd,(HMENU)10, hInstance,NULL);
		  //Still need the .wav files
		  hOggNeedWav=CreateWindow("button", "", BS_CHECKBOX | WS_CHILD | WS_VISIBLE | WS_BORDER,
						261,190,15,15,hwnd,(HMENU)11, hInstance,NULL);

		  //scroll bar for quality of the compression	
		  hOggQuality=CreateWindow("edit", "1", WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_BORDER, 342, 110 , 30, 15, hwnd,
			  (HMENU) 12, hInstance, NULL);
		  

		  /*---------------------Initializing  stuff-------------------------*/
		  
		  //Initializing browse information for Folder Dialog box
		  browseInfo.hwndOwner=hwnd;
		  browseInfo.pidlRoot=NULL;
		  browseInfo.lpszTitle="Choose the output directory";
		  browseInfo.lpfn=NULL;

		  //Initializing the configuration PARAMS
          params.deviceLPGUID = NULL;
		  params.iBitsPerSample=16;
		  params.iFileSize=60;
		  params.iSampleRate=11024;
		  params.iStereo=1;
		  bRecording=0;
		  params.hwnd=hwnd;
		  params.bKill=0;
		  params.qualityOfOggCompression=0.1f;  
		  params.iCompressionOnOFFBoth=1;
		  
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
		  TextOut(hdc,115, 34, "Sample rate in Hz", (int)strlen("Sample rate in Hz"));
		  TextOut(hdc, 75, 96, "Bits per sample", (int)strlen("Bits per sample"));
		 
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
			case 1://START button

				if(bRecording) //if its recording already break;
					break;

				if(GetDlgItemInt(hwnd,3,&bTemp,0)<60)//if the user has chosed a file smaller then 60seconds, warn and break;
				{
					MessageBox(hwnd, "Files can not be smaller then 60 seconds.", "Warning",  MB_OK | MB_ICONWARNING);
					break;
				}
				
				
				params.iFilesWritten=1; // It is the first file written
                bRecording=1;//recording flag on;
				params.bKill=0;//do not close the thread;

				SendMessage(hwndDisplay1,WM_INITDIALOG,0,0);
				ShowWindow(hwndDisplay1,SW_HIDE);
				
				if(!_beginthread(Thread,0,&params)){//if error
					MessageBox(hwnd, "Could not start the recorder","Error", MB_OK);
					bRecording=0;
					break;
				}
				//Disable the unneeded buttons
				EnableWindow(hStart,0);
				EnableWindow(hSampleRate,0);
				EnableWindow(hFileSize,0);
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

			case 2://Stop button
				if(!bRecording) break;//if not recording break;
				params.bKill=1;//close the thread
				SendMessage(hwndDisplay,WM_CLOSE,0,0);//close the display dialog
				ShowWindow(hwndDisplay1,SW_SHOWNORMAL);//open the first display
				SetFocus(hwnd);
				
				
				//Enable the buttons
				EnableWindow(hStart,1);
				EnableWindow(hSampleRate,1);
				EnableWindow(hFileSize,1);
				EnableWindow(hMono,1);
				EnableWindow(hStereo,1);
				EnableWindow(hBits,1);
                EnableWindow(hRecordingControl,1);
                EnableWindow(hDevCaps,1);
				EnableWindow(hOgg,1);
				EnableWindow(hStop,0);
				if(bOgg){
					EnableWindow(hOggQuality,1);
					EnableWindow(hOggNeedWav,1);
				}

				Sleep(1001);//sleep for one second so the thread is closed for sure.

                bRecording=0;//not recording flag
				break;
				
			case 5://Mono
				if(bStereo){
					SendMessage(hStereo, BM_SETCHECK, 0, 0);
					SendMessage(hMono, BM_SETCHECK, 1, 0);
					bStereo=0;
					params.iStereo=1;
				}
				SendMessage(hwndDisplay1,WM_COMMAND,33,0);//send message to Display1 so it can update the filesize MB/min
				break;

			case 6://Stereo
				if(!bStereo){
					SendMessage(hMono, BM_SETCHECK, 0,0);
					SendMessage(hStereo, BM_SETCHECK, 1,0);	
					bStereo=1;
					params.iStereo=2;
				}
				SendMessage(hwndDisplay1,WM_COMMAND,33,0);//send message to Display1 so it can update the filesize MB/min
				break;

			case 3://File size
				if(HIWORD(wParam)==EN_UPDATE){
					params.iFileSize=GetDlgItemInt(hwnd,3,&bTemp,0);
					if(!bTemp){//if it can't translate the value into number
						params.iFileSize=60;//set default length to 60 seconds
						SetWindowText(hFileSize,"60");
					}
				}
				break;

			case 4://Sample rate
				if(HIWORD (wParam) == LBN_SELCHANGE)
					params.iSampleRate=GetDlgItemInt(hwnd,4,&bTemp,0);
				SendMessage(hwndDisplay1,WM_COMMAND,33,0);//send message to Display1 so it can update the filesize MB/min
				break;

			case 7://Bits per sample
				if(HIWORD (wParam) == LBN_SELCHANGE)
					params.iBitsPerSample=GetDlgItemInt(hwnd,7,&bTemp,0);	
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

			case 8: //Select capture device (e.g sound card)from a dialog box
				DialogBox(hInstance,(LPCSTR)IDD_DIALOG_SELECT_DEVICE, hwnd, AboutCaptureDevice);
				break;

			case 9://Opens the dialog box with the device capabilities
				DialogBox(hInstance,(LPCSTR)IDD_DIALOGDEVCAPS, hwnd, AboutDevCaps);
				break;

			case 10://Do you want Ogg Vorbis compression
				if(bOgg==false){//compression ON
					bOgg=true;
					if(bStillNeedWav)
						params.iCompressionOnOFFBoth=3; 
					else params.iCompressionOnOFFBoth=2; 
					
					SendMessage(hOgg,BM_SETCHECK,1,0);
					/*enabling the buttons from the ogg compression part*/
					EnableWindow(hOggQuality,1);
					EnableWindow(hOggNeedWav,1);
				}
				else{		    //compression OFF
					bOgg=false;
					SendMessage(hOgg,BM_SETCHECK,0,0);
					
					/*Sets the quality to 0, so we know that we should
					  not compress the sound*/
					params.iCompressionOnOFFBoth=1;  
					
					/*disabling the buttons from the ogg compression part*/
					EnableWindow(hOggQuality,0);
					EnableWindow(hOggNeedWav,0);
				}
				break;

			case 11://I still need the .wav files
				if(bStillNeedWav==false){
					bStillNeedWav=true;
					params.iCompressionOnOFFBoth=3;
					SendMessage(hOggNeedWav,BM_SETCHECK,1,0);
				}
				else{
					bStillNeedWav=false;
					params.iCompressionOnOFFBoth=2;
					SendMessage(hOggNeedWav,BM_SETCHECK,0,0);
				}
				break;

			case 12://edit control, quality of compression
				if(HIWORD(wParam)==EN_KILLFOCUS){
					iTemp=GetDlgItemInt(hwnd,12,&bTemp,true);
					if(!bTemp){//if it can't translate the value into number
						params.qualityOfOggCompression=0.1f;//set default length to 60 seconds
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
		 
		  if(params.iStereo==2)	
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT2),"Stereo");
		  else SetWindowText(GetDlgItem(hDlg, IDC_EDIT2),"Mono");
		
		  StringCchPrintf(szBuffer,MAX_PATH, TEXT("%d Bits"), params.iBitsPerSample);
		  SetWindowText(GetDlgItem(hDlg, IDC_EDIT3),szBuffer);
		  //SetTimer(hDlg,NULL,10000,NULL);
		  
		  StringCchPrintf(szBuffer,MAX_PATH, TEXT("files written:%d"), params.iFilesWritten);
		  SetWindowText(GetDlgItem(hDlg, IDC_EDIT4),szBuffer);
		  if(params.iCompressionOnOFFBoth==2 || params.iCompressionOnOFFBoth==3){
			  SetWindowText(GetDlgItem(hDlg, IDC_EDIT5),"Vorbis: ON");
			  StringCchPrintf(szBuffer,MAX_PATH, TEXT("At quality:%.1f"), params.qualityOfOggCompression);
			  SetWindowText(GetDlgItem(hDlg, IDC_EDIT6),szBuffer);
		  }
		  return TRUE ;
	 //case WM_TIMER:
		  //how many files there are created
	//	  wsprintf(szBuffer,"files written:%d", params.iFilesWritten);
	//	  SetWindowText(GetDlgItem(hDlg, IDC_EDIT4),szBuffer);
	//	  return 0;
	 case WM_CLOSE : 
		  DestroyWindow(hDlg);
		  return TRUE;
     }
     return FALSE ;
}

BOOL CALLBACK AboutAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message)
    {
	   case WM_INITDIALOG:
			//SetWindowText(GetDlgItem(hDlg,IDC_EDITABOUT),"Freeware recording software. \n http://www.qsl.net/lz1abc/software.htm");
		    SetDlgItemText(hDlg, IDC_EDITABOUT, "Freeware recording software. http://www.qsl.net/lz1abc/software.htm");
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

BOOL CALLBACK AboutIndex(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message)
    {
	   case WM_INITDIALOG:
		   SetWindowText(GetDlgItem(hDlg,IDC_EDIT1),"http://www.qsl.net/lz1abc/software.htm");
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
		   sprintf_s(szBuffer,"Wav file size: %.2f MB/sec",(((double)params.iBitsPerSample*params.iSampleRate*params.iStereo)/8388608)*60);
		   SetWindowText(GetDlgItem(hDlg,IDC_EDIT1),szBuffer);
		    return TRUE;
	   case WM_COMMAND:
			switch(wParam)
			{
				case 33:
					sprintf_s(szBuffer,"Wav file size: %.2f MB/min",(((double)params.iBitsPerSample*params.iSampleRate*params.iStereo)/8388608)*60);
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
