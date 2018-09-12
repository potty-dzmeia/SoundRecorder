#include <fstream>
#include <vorbis/vorbisenc.h>
#include "params.h"

using namespace std;

#pragma once

class OGGencoder{
private:
	
	PARAMS 	        params;			 //Audio parameters
	ofstream		myFile;          //File where the data will be written

	ogg_stream_state os;   /* take physical pages, weld into a logical
						      stream of packets */
	ogg_page         og;   /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       op;   /* one raw packet of data for decode */
	vorbis_info      vi;   /* struct that stores all the static vorbis bitstream
							  settings */
	vorbis_comment   vc;   /* struct that stores all the user comments */
	vorbis_dsp_state vd;   /* central working state for the packet->PCM decoder */
	vorbis_block     vb;   /* local working space for packet->PCM decode */
						

public:	
	
	OGGencoder(pPARAMS,char* szOutputFilename);
	~OGGencoder();
	
		/*initializing encoder setup*/
	bool initLibvorbisenc();
		/*Here the bitstream is initialized. And the header of the file is written*/
	bool initBitstream();
		/*Encodes a chunk of PCM data and appends it to a file*/
	bool encodeChunk(signed char *readbuffer);
		/*Cleans up stuff*/
	bool cleanup();
 
};