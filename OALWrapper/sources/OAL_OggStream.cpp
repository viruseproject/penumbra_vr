/*
 * Copyright 2007-2010 (C) - Frictional Games
 *
 * This file is part of OALWrapper
 *
 * For conditions of distribution and use, see copyright notice in LICENSE
 */
/**
	@file OAL_OggStream.cpp
	@author Luis Rodero
	@date 2006-10-06
	@version 0.1
	Definition of Base class for containing Ogg Vorbis streams
*/


#include "OALWrapper/OAL_OggStream.h"
#include "OALWrapper/OAL_Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>


extern ov_callbacks gCallbacks;

//---------------------------------------------------------------------

cOAL_OggStream::cOAL_OggStream()
{
}

//---------------------------------------------------------------------

cOAL_OggStream::~cOAL_OggStream()
{
	Destroy();
}

//---------------------------------------------------------------------

///////////////////////////////////////////////////////////
//	void Stream ( const ALuint alDestBuffer )
//	-	Streams data from the Ogg file to the buffer
///////////////////////////////////////////////////////////

//---------------------------------------------------------------------

bool cOAL_OggStream::Stream(cOAL_Buffer* apDestBuffer)
{
	DEF_FUNC_NAME("cOAL_OggStream::Stream()");
	FUNC_USES_AL;

	long lDataSize = 0;

	//hpl::Log("Streaming...\n");
	double fStartTime = GetTime();

	// Loop which loads chunks of decoded data into a buffer
	while(lDataSize < (int)mlBufferSize)
	{
		long lChunkSize = ov_read(&movStreamHandle,
								   mpPCMBuffer+lDataSize,
								   mlBufferSize-lDataSize,
								   SYS_ENDIANNESS,
								   2, 1, &mlCurrent_section);

		// If we get a 0, then we are at the end of the file
		if(lChunkSize == 0)
		{
			break;
		}
		// If we get a negative value, then something went wrong. Clean up and set error status.
		else if(lChunkSize == OV_HOLE)										
			;
		else if(lChunkSize == OV_EINVAL)
			mbStatus = false;
		else if(lChunkSize == OV_EBADLINK)
			mbStatus = false;
		else if(lChunkSize<0)
			mbStatus = false;
		else
			lDataSize += lChunkSize;
	}
	// Bind the data to the Buffer Object
	if(lDataSize) 
		apDestBuffer->Feed(mpPCMBuffer, lDataSize, fStartTime);
	else 
		mbEOF = true;

	if(AL_ERROR_OCCURED)
	{
		//hpl::Log("%s\n", OAL_GetALErrorString().c_str());
		mbStatus = false;
	}

	return (lDataSize != 0);
}

//---------------------------------------------------------------------

///////////////////////////////////////////////////////////
// void Seek(float afWhere)
//
///////////////////////////////////////////////////////////

//---------------------------------------------------------------------

void cOAL_OggStream::Seek(float afWhere, bool abForceRebuffer)
{
	mbEOF = false;
	
	if(afWhere < 0)
		afWhere = 0;
	if(afWhere > 1)
		afWhere = 1;

	afWhere = afWhere*mlSamples;

    // Move the pointer to the desired offset
	ov_pcm_seek_page_lap(&movStreamHandle, (long int) afWhere);
	if(abForceRebuffer)
		mbNeedsRebuffering = true;
}

//---------------------------------------------------------------------

double cOAL_OggStream::GetTime()
{
	return ov_time_tell(&movStreamHandle);
}

//---------------------------------------------------------------------

///////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////

//---------------------------------------------------------------------

bool cOAL_OggStream::CreateFromFile(const wstring &asFilename)
{
	DEF_FUNC_NAME("cOAL_OggStream::Load()");
	
	if(mbStatus==false)
		return false;
	
	std::string sTemp;
	size_t needed = wcstombs(NULL,&asFilename[0],asFilename.length());
	sTemp.resize(needed);
	wcstombs(&sTemp[0],&asFilename[0],asFilename.length());

	int lOpenResult;
	FILE *pStreamFile = fopen(sTemp.c_str(),"rb");

	if (pStreamFile == NULL)
		return false;

	msFilename = asFilename;

	// If not an Ogg file, set status and exit

	lOpenResult = ov_open_callbacks(pStreamFile, &movStreamHandle,
									NULL, 0, gCallbacks);
	if(lOpenResult<0)	
	{
		fclose(pStreamFile);
		mbIsValidHandle = false;
		mbStatus = false;
		return false;
	}
	mbIsValidHandle = true;

	// Get file info
	vorbis_info *viFileInfo = ov_info ( &movStreamHandle, -1 );

	mlChannels = viFileInfo->channels;
	mlFrequency = viFileInfo->rate;
	mlSamples = (long) ov_pcm_total ( &movStreamHandle, -1 );
	mFormat = (mlChannels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

	mfTotalTime = ov_time_total( &movStreamHandle, -1 );

	return true;
}

//---------------------------------------------------------------------

///////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////

//---------------------------------------------------------------------

void cOAL_OggStream::Destroy()
{
	DEF_FUNC_NAME("cOAL_OggStream::Unload()");
	
	// If we loaded a stream, clear the handle to the Ogg Vorbis file
	if(mbIsValidHandle)
		ov_clear(&movStreamHandle);
}

//---------------------------------------------------------------------

