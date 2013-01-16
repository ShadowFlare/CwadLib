/*
	Copyright (c) 2002-2013, ShadowFlare <blakflare@hotmail.com>
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGE.
*/

#ifndef CWADLIB_INCLUDED
#define CWADLIB_INCLUDED

#include <tchar.h>

#define CWAD_INFO_NUM_FILES       0x03 //Number of files in CWAD
#define CWAD_INFO_SIZE            0x05 //Size of CWAD or uncompressed file
#define CWAD_INFO_COMPRESSED_SIZE 0x06 //Size of compressed file
#define CWAD_INFO_FLAGS           0x07 //File flags (compressed, etc.)
#define CWAD_INFO_PARENT          0x08 //Handle of CWAD that file is in
#define CWAD_INFO_POSITION        0x09 //Position of file pointer in files

#define CWAD_FILE_BEGIN   0
#define CWAD_FILE_CURRENT 1
#define CWAD_FILE_END     2

void *        CWadOpenArchive(const TCHAR *pszFileName);
bool          CWadCloseArchive(void *hCWAD);
unsigned long CWadGetArchiveInfo(void *hCWAD, int nInfoType);
unsigned long CWadListFiles(void *hCWAD, char *pmszBuffer, unsigned long nBufferLength); // Returns required buffer size.  Strings are in multi string form. (null-terminated strings with an extra null after the last string)
void *        CWadOpenFile(void *hCWAD, const char *pszFileName);
bool          CWadCloseFile(void *hFile);
unsigned long CWadGetFileSize(void *hFile);
unsigned long CWadGetFileInfo(void *hFile, int nInfoType);
unsigned long CWadSetFilePointer(void *hFile, long nDistanceToMove, int nMoveMethod);
unsigned long CWadReadFile(void *hFile, void *pBuffer, unsigned long nNumberOfBytesToRead);
unsigned long CWadFindHeader(FILE *pFile);

#endif
