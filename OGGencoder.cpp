#include <time.h>
#include "OGGencoder.h"


OGGencoder::OGGencoder(pPARAMS inputParams,char *szOutputFile){
	params = *inputParams;
	myFile.open(szOutputFile, ios::binary | ios::out);
	if(params.iSampleRate!=11024)
		params.iSampleRate=1;
}

OGGencoder::~OGGencoder(void){
	cleanup();
}
 
/*----------------------------------------------------------------------------
| Libvorbisenc is an encoding convenience library intended to encapsulate the
| elaborate setup that libvorbis requires for encoding. Libvorbisenc gives easy 
| access to all high-level adjustments an application may require when encoding 
| and also exposes some low-level tuning parameters to allow applications to make 
| detailed adjustments to the encoding process.
|	
|   Here the encoder setup is initialized
----------------------------------------------------------------------------*/
bool OGGencoder :: initLibvorbisenc(){
	int ret;
	
	/*The vorbis_info struct should be initialized by using vorbis_info_init()
	 *from the libvorbis API. After encoding, vorbis_info_clear should be called.
	 */
	vorbis_info_init(&vi);
	ret=vorbis_encode_init_vbr(&vi, params.iStereo,params.iSampleRate,params.qualityOfOggCompression); 
	if(ret){
		MessageBox(params.hwnd, TEXT("Error when trying to call vorbis_encode_init_vbr()"),TEXT("Error!"), MB_OK || MB_ICONEXCLAMATION);
		return 0;
	}

	/* add a comment */
	vorbis_comment_init(&vc);
	vorbis_comment_add_tag(&vc,"Contest recorder","Contest recorder");
	
	/* set up the analysis state and auxiliary encoding storage */
	vorbis_analysis_init(&vd,&vi);
	vorbis_block_init(&vd,&vb);

	return 1;
}

/*---------------------------------------------------------------------------
| When encoding, the encoding engine will output raw packets which must 
| be placed into an Ogg bitstream.
| when enough packets have been written to create a full page. 
| The pages output are pointers to buffered packet segments, and can then be
| written out and saved as an ogg stream. 
|
| Here the bitstream is initialized. And the header of the file is written.
--------------------------------------------------------------------------*/
bool OGGencoder :: initBitstream(){
	
	/* set up our packet->stream encoder */
	/* pick a random serial number; that way we can more likely build
	   chained streams just by concatenation */
	srand(time(NULL));
	ogg_stream_init(&os,rand());

	 
	/* Vorbis streams begin with three headers; the initial header (with
     most of the codec setup parameters) which is mandated by the Ogg
     bitstream spec.  The second header holds any comment fields.  The
     third header holds the bitstream codebook.  We merely need to
     make the headers, then pass them to libvorbis one at a time;
     libvorbis handles the additional Ogg bitstream constraints */
	ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

    vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
    ogg_stream_packetin(&os,&header); /* automatically placed in its own
					 page */
    ogg_stream_packetin(&os,&header_comm);
    ogg_stream_packetin(&os,&header_code);

	/* This ensures the actual
	 * audio data will start on a new page, as per spec
	 */
	
	while(1){
		int result=ogg_stream_flush(&os,&og);
		if(result==0)break;
		myFile.write((char*)og.header,og.header_len);
		myFile.write((char*)og.body,og.body_len);		
	}
	
	return 1;
}

/*--------------------------------------------------------
|  -Encodes the PCM data(pReadBuffer)
|  -Writes the compressed data into a file.
|
----------------------------------------------------------*/
bool OGGencoder :: encodeChunk(signed char *pReadBuffer){
	
	int eos=0;
	long i;
	
	//if(pReadBuffer==NULL){
		/* end of file.  this can be done implicitly in the mainline,
		   but it's easier to see here in non-clever fashion.
		   Tell the library we're at end of stream so that it can handle
		   the last frame and mark end of stream in the output properly */
	//	vorbis_analysis_wrote(&vd,0);
	//	return 1;
	//}


	/* expose the buffer to submit data */
	float **inputBuffer = vorbis_analysis_buffer(&vd,params.iSampleRate/2);
  
	// uninterleave and copy the samples
	if (params.iBitsPerSample == 8) 
	{
		for(i=0;i<params.iSampleRate/2;i++) {
			for(int j=0;j<params.iStereo;j++) {
				inputBuffer[j][i]=((int)(pReadBuffer[i*params.iStereo + j])-128)/128.0f;
            }
        }
     } 
	 else 
	 {
		 for(i=0;i<params.iSampleRate/2;i++) {
			for(int j=0;j<params.iStereo;j++) {
				inputBuffer[j][i]=((int)(pReadBuffer[i*2*params.iStereo +2*j +1]<<8) |
					(pReadBuffer[i*2*params.iStereo+2*j] & 0xff))/32768.0f;
             }
         }
     }
	
	
	/* tell the library how much we actually submitted */
	vorbis_analysis_wrote(&vd,i);	
	/* vorbis does some data preanalysis, then divvies up blocks for
	   more involved (potentially parallel) processing.  Get a single
	   block for encoding now */
	while(vorbis_analysis_blockout(&vd,&vb)==1){
		
		/* analysis, assume we want to use bitrate management */
		vorbis_analysis(&vb,NULL);
		vorbis_bitrate_addblock(&vb);

		while(vorbis_bitrate_flushpacket(&vd,&op)){
	
			/* weld the packet into the bitstream */
			ogg_stream_packetin(&os,&op);
	
			/* write out pages (if any) */
			while(1){
				//int result=ogg_stream_pageout(&os,&og);
				int result=ogg_stream_flush(&os,&og);
				/*0 means that all packet data has already been flushed into
				    pages, and there are no packets to put into the page.*/
				if(result==0)
				{
					break; 
				}

				/*writes the pages into a file*/
				myFile.write((char*)og.header,og.header_len);
				myFile.write((char*)og.body,og.body_len);

			 }//while(1)
		}//while(vorbis_bitrate_flushpacket(&vd,&op))
	}//while(vorbis_analysis_blockout(&vd,&vb)==1)
	//vorbis_analysis_wrote(&vd,0);
	return 1;
}

/*--------------------------------------------------------------------------
|
|Cleaning up some stuff...
|
|--------------------------------------------------------------------------*/
bool OGGencoder :: cleanup(){
	/* clean up and exit.  vorbis_info_clear() must be called last */
  
	 ogg_stream_clear(&os);
	 vorbis_block_clear(&vb);
	 vorbis_dsp_clear(&vd);
	 vorbis_comment_clear(&vc);
	 vorbis_info_clear(&vi);
	 myFile.close();
	/* ogg_page and ogg_packet structs always point to storage in
	   libvorbis.  They're never freed or manipulated directly */

	return 1;
}