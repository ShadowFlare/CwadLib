#include "CwadLib.h"
#include "SFTypes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef __SYS_ZLIB
#include "zlib/zlib.h"
#else
#include <zlib.h>
#endif

const UInt32 ID_CWAD = *(UInt32 *)"CWAD";
const UInt32 ID_SEP = *(UInt32 *)"SEP\0";

typedef struct _CWADFILEHEADER {
	UInt32 dwPackedSize;
	UInt32 dwNameLength;
	UInt32 dwFullSize;
	UInt32 dwFlags;
//	char   szFileName[dwNameLength];
//	UInt8  byData[dwPackedSize];
} CWADFILEHEADER;

/* struct CWAD {
	char     IDTag[4] = "CWAD";
	CWADFILEHEADER Files[];
}; */

typedef struct _CWADFILE CWADFILE;

typedef struct _CWADARCHIVE {
	TCHAR *pszFileName;
	FILE *pFile;
	unsigned long nStart;
	unsigned long nEnd;
	unsigned long nFiles;
	CWADFILE *pFiles;
	char *pmszFileList;
	unsigned long nFileListSize;
	bool bHasFileNameBlank;
	long nReferences;
	bool bOpen;
} CWADARCHIVE;

typedef struct _CWADFILE {
	char *pszFileName;
	CWADARCHIVE *pParentArc;
	UInt32 dwPackedSize;
	UInt32 dwFullSize;
	UInt32 dwFlags;
	unsigned long nOffset;
	UInt8 *pBuffer;
	long nReferences;
} CWADFILE;

typedef struct _CWADFILEHANDLE {
	CWADFILE *f;
	UInt32 dwFilePointer;
} CWADFILEHANDLE;

void CWadListFilesInternal(void *hCWAD);
unsigned long CWadFindHeaderAndSize(FILE *hFile, unsigned long *pnCwadEnd);
void CWadDecryptData(UInt8 *pBuffer, unsigned long nBufferLength);

void * CWadOpenArchive(const TCHAR *pszFileName)
{
	if (!pszFileName)
		return 0;

	unsigned long flen;
	FILE *pFile;
	pFile = _tfopen(pszFileName, _T("rb"));
	if (pFile)
	{
		unsigned long nCwadStart = CWadFindHeaderAndSize(pFile, &flen);
		if (nCwadStart != -1) {
			CWADARCHIVE *cOpenArc = (CWADARCHIVE *)malloc(sizeof(CWADARCHIVE));
			if (cOpenArc) {
				cOpenArc->pFile = pFile;
				cOpenArc->nStart = nCwadStart;
				cOpenArc->nEnd = flen;
				cOpenArc->pmszFileList = 0;
				cOpenArc->bHasFileNameBlank = false;
				cOpenArc->pFiles = 0;
				CWadListFilesInternal(cOpenArc);
				if (cOpenArc->pmszFileList) {
					cOpenArc->pszFileName = _tcsdup(pszFileName);
					cOpenArc->nReferences = 0;
					cOpenArc->bOpen = true;
					return cOpenArc;
				}

				free(cOpenArc);
			}
		}

		fclose(pFile);
	}

	return 0;
}

bool CWadCloseArchive(void *hCWAD)
{
	if (!hCWAD)
		return false;

	CWADARCHIVE *cOpenArc = (CWADARCHIVE *)hCWAD;
	if (cOpenArc->nReferences > 0) {
		cOpenArc->bOpen = false;
		return false;
	}
	fclose(cOpenArc->pFile);
	if (cOpenArc->pFiles) free(cOpenArc->pFiles);
	if (cOpenArc->pmszFileList) free(cOpenArc->pmszFileList);
	if (cOpenArc->pszFileName) free(cOpenArc->pszFileName);
	free(cOpenArc);
	return true;
}

unsigned long CWadGetArchiveInfo(void *hCWAD, int nInfoType)
{
	if (!hCWAD)
		return -1;

	CWADARCHIVE *cOpenArc = (CWADARCHIVE *)hCWAD;
	switch (nInfoType) {
	case CWAD_INFO_NUM_FILES:
		return cOpenArc->nFiles;
	case CWAD_INFO_SIZE:
		return cOpenArc->nEnd - cOpenArc->nStart;
	default:
		return -1;
	}
}

unsigned long CWadListFiles(void *hCWAD, char *pmszBuffer, unsigned long nBufferLength)
{
	if (!hCWAD)
		return 0;

	CWADARCHIVE *cOpenArc = (CWADARCHIVE *)hCWAD;
	if (pmszBuffer && nBufferLength >= cOpenArc->nFileListSize)
		memcpy(pmszBuffer, cOpenArc->pmszFileList, cOpenArc->nFileListSize);

	return cOpenArc->nFileListSize;
}

void CWadListFilesInternal(void *hCWAD)
{
	if (!hCWAD)
		return;

	CWADARCHIVE *cOpenArc = (CWADARCHIVE *)hCWAD;
	CWADFILEHEADER CwadFile;
	unsigned long nReqSize = 0;
	cOpenArc->nFiles = 0;
	if (cOpenArc->bHasFileNameBlank && cOpenArc->pmszFileList) {
		*cOpenArc->pmszFileList = 0;
		nReqSize++;
	}
	fseek(cOpenArc->pFile, cOpenArc->nStart + 4, SEEK_SET);
	for (long fpos = ftell(cOpenArc->pFile); fpos != -1 && fpos + sizeof(CWADFILEHEADER) <= cOpenArc->nEnd; fpos = ftell(cOpenArc->pFile)) {
		if (fread(&CwadFile, sizeof(CWADFILEHEADER), 1, cOpenArc->pFile) == 1) {
			if (cOpenArc->pmszFileList)
			{
				if (CwadFile.dwNameLength > 0) {
					if (fread(cOpenArc->pmszFileList + nReqSize, CwadFile.dwNameLength, 1, cOpenArc->pFile) == 1)
						CWadDecryptData((UInt8 *)cOpenArc->pmszFileList + nReqSize, CwadFile.dwNameLength);
					cOpenArc->pFiles[cOpenArc->nFiles].pszFileName = cOpenArc->pmszFileList + nReqSize;
				}
				else
					cOpenArc->pFiles[cOpenArc->nFiles].pszFileName = cOpenArc->pmszFileList;
				cOpenArc->pFiles[cOpenArc->nFiles].pParentArc = cOpenArc;
				cOpenArc->pFiles[cOpenArc->nFiles].dwPackedSize = CwadFile.dwPackedSize;
				cOpenArc->pFiles[cOpenArc->nFiles].dwFullSize = CwadFile.dwFullSize;
				cOpenArc->pFiles[cOpenArc->nFiles].dwFlags = CwadFile.dwFlags;
				cOpenArc->pFiles[cOpenArc->nFiles].nOffset = fpos + sizeof(CWADFILEHEADER) + CwadFile.dwNameLength;
				cOpenArc->pFiles[cOpenArc->nFiles].pBuffer = 0;
				cOpenArc->pFiles[cOpenArc->nFiles].nReferences = 0;
			}
			else {
				fseek(cOpenArc->pFile, CwadFile.dwNameLength, SEEK_CUR);
			}
			nReqSize += CwadFile.dwNameLength;
			if (CwadFile.dwNameLength > 0) {
				if (cOpenArc->pmszFileList)
					cOpenArc->pmszFileList[nReqSize] = 0;
				nReqSize++;
			}
			else if (!cOpenArc->bHasFileNameBlank) {
				cOpenArc->bHasFileNameBlank = true;
				nReqSize++;
			}
			fseek(cOpenArc->pFile, CwadFile.dwPackedSize, SEEK_CUR);
			cOpenArc->nFiles++;
		}
		else break;
	}
	if (cOpenArc->pmszFileList)
		cOpenArc->pmszFileList[nReqSize] = 0;
	nReqSize++;
	if (nReqSize == 1)
	{
		if (cOpenArc->pmszFileList)
			cOpenArc->pmszFileList[nReqSize] = 0;
		nReqSize++;
	}
	if (!cOpenArc->pmszFileList) {
		cOpenArc->pmszFileList = (char *)malloc(nReqSize);
		cOpenArc->nFileListSize = nReqSize;
		if (cOpenArc->nFiles > 0)
			cOpenArc->pFiles = (CWADFILE *)calloc(cOpenArc->nFiles, sizeof(CWADFILE));
		if (cOpenArc->pmszFileList && (cOpenArc->pFiles || cOpenArc->nFiles == 0))
			CWadListFilesInternal(hCWAD);
		else {
			if (cOpenArc->pmszFileList) {
				free(cOpenArc->pmszFileList);
				cOpenArc->pmszFileList = 0;
			}
			if (cOpenArc->pFiles)
				free(cOpenArc->pFiles);
		}
	}
}

void * CWadOpenFile(void *hCWAD, const char *pszFileName)
{
	if (!hCWAD || !pszFileName)
		return 0;

	CWADARCHIVE *cOpenArc = (CWADARCHIVE *)hCWAD;
	unsigned long nFile;
	for (nFile = 0; nFile < cOpenArc->nFiles; nFile++) {
		if (strcmp(pszFileName, cOpenArc->pFiles[nFile].pszFileName) == 0) {
			CWADFILEHANDLE *cOpenFile = (CWADFILEHANDLE *)malloc(sizeof(CWADFILEHANDLE));
			if (cOpenFile) {
				cOpenFile->f = &cOpenArc->pFiles[nFile];
				cOpenFile->dwFilePointer = 0;
				cOpenFile->f->nReferences++;
				cOpenArc->nReferences++;
				return cOpenFile;
			}
		}
	}

	return 0;
}

bool CWadCloseFile(void *hFile)
{
	if (!hFile)
		return false;

	CWADFILEHANDLE *cOpenFile = (CWADFILEHANDLE *)hFile;
	cOpenFile->f->nReferences--;
	cOpenFile->f->pParentArc->nReferences--;
	if (cOpenFile->f->nReferences < 1 && cOpenFile->f->pBuffer) {
		free(cOpenFile->f->pBuffer);
		cOpenFile->f->pBuffer = 0;
	}
	if (cOpenFile->f->pParentArc->nReferences < 1 && !cOpenFile->f->pParentArc->bOpen)
		CWadCloseArchive(cOpenFile->f->pParentArc);
	free(cOpenFile);

	return true;
}

unsigned long CWadGetFileSize(void *hFile)
{
	if (!hFile)
		return -1;

	CWADFILEHANDLE *cOpenFile = (CWADFILEHANDLE *)hFile;
	return cOpenFile->f->dwFullSize;
}

unsigned long CWadGetFileInfo(void *hFile, int nInfoType)
{
	if (!hFile)
		return -1;

	CWADFILEHANDLE *cOpenFile = (CWADFILEHANDLE *)hFile;
	switch (nInfoType) {
	case CWAD_INFO_NUM_FILES:
		return cOpenFile->f->pParentArc->nFiles;
	case CWAD_INFO_SIZE:
		return cOpenFile->f->dwFullSize;
	case CWAD_INFO_COMPRESSED_SIZE:
		return cOpenFile->f->dwPackedSize;
	case CWAD_INFO_FLAGS:
		return cOpenFile->f->dwFlags;
	case CWAD_INFO_PARENT:
		return (unsigned long)cOpenFile->f->pParentArc;
	case CWAD_INFO_POSITION:
		return cOpenFile->dwFilePointer;
	default:
		return -1;
	}
}

unsigned long CWadSetFilePointer(void *hFile, long nDistanceToMove, int nMoveMethod)
{
	if (!hFile)
		return -1;

	CWADFILEHANDLE *cOpenFile = (CWADFILEHANDLE *)hFile;
	long fsz = cOpenFile->f->dwFullSize;
	long cpos = cOpenFile->dwFilePointer;
	switch (nMoveMethod) {
	case CWAD_FILE_CURRENT:
		if (cpos + nDistanceToMove < 0 || cpos + nDistanceToMove > fsz) return -1;
		cOpenFile->dwFilePointer += nDistanceToMove;
		break;
	case CWAD_FILE_END:
		if (fsz + nDistanceToMove < 0 || nDistanceToMove > 0) return -1;
		cOpenFile->dwFilePointer = fsz + nDistanceToMove;
		break;
	case CWAD_FILE_BEGIN:
	default:
		if (nDistanceToMove < 0 || nDistanceToMove > fsz) return -1;
		cOpenFile->dwFilePointer = nDistanceToMove;
	}

	return cOpenFile->dwFilePointer;
}

unsigned long CWadReadFile(void *hFile, void *pBuffer, unsigned long nNumberOfBytesToRead)
{
	if (!hFile || !pBuffer || nNumberOfBytesToRead == 0)
		return 0;

	CWADFILEHANDLE *cOpenFile = (CWADFILEHANDLE *)hFile;
	CWADARCHIVE *cOpenArc = (CWADARCHIVE *)cOpenFile->f->pParentArc;
	if (cOpenFile->dwFilePointer >= cOpenFile->f->dwFullSize)
		return 0;
	if (cOpenFile->dwFilePointer + nNumberOfBytesToRead > cOpenFile->f->dwFullSize)
		nNumberOfBytesToRead = cOpenFile->f->dwFullSize - cOpenFile->dwFilePointer;
	unsigned long nBytesRead = nNumberOfBytesToRead;
	if (cOpenFile->f->dwFlags & 1) {
		if (!cOpenFile->f->pBuffer) {
			cOpenFile->f->pBuffer = (UInt8 *)malloc(cOpenFile->f->dwFullSize);
			UInt8 *compbuffer = (UInt8 *)malloc(cOpenFile->f->dwPackedSize);
			bool bReadOK = false;
			if (cOpenFile->f->pBuffer && compbuffer) {
				fseek(cOpenArc->pFile, cOpenFile->f->nOffset, SEEK_SET);
				if (fread(compbuffer, 1, cOpenFile->f->dwPackedSize, cOpenArc->pFile) == cOpenFile->f->dwPackedSize)
					if (uncompress(cOpenFile->f->pBuffer, &nBytesRead, compbuffer, cOpenFile->f->dwPackedSize) == Z_OK)
						bReadOK = true;
			}
			if (!bReadOK && cOpenFile->f->pBuffer) {
				free(cOpenFile->f->pBuffer);
				cOpenFile->f->pBuffer = 0;
				nBytesRead = 0;
			}
			if (compbuffer)
				free(compbuffer);
		}
		if (cOpenFile->f->pBuffer)
			memcpy(pBuffer, cOpenFile->f->pBuffer + cOpenFile->dwFilePointer, nNumberOfBytesToRead);
	}
	else {
		fseek(cOpenArc->pFile, cOpenFile->f->nOffset + cOpenFile->dwFilePointer, SEEK_SET);
		nBytesRead = fread(pBuffer, 1, nNumberOfBytesToRead, cOpenArc->pFile);
	}

	cOpenFile->dwFilePointer += nBytesRead;
	return nBytesRead;
}

unsigned long CWadFindHeader(FILE *hFile)
{
	return CWadFindHeaderAndSize(hFile, 0);
}

unsigned long CWadFindHeaderAndSize(FILE *pFile, unsigned long *pnCwadEnd)
{
	if (!pFile) return -1;

	if (fseek(pFile, 0, SEEK_END)) return -1;
	long fsz = ftell(pFile);
	UInt32 buffer[2];
	long sep, i, offset;

	if (pnCwadEnd) *pnCwadEnd = fsz;

	fseek(pFile, 0, SEEK_SET);
	if (fread(buffer, sizeof(UInt32), 1, pFile) < 1) return -1;
	if (buffer[0] == ID_CWAD)
		return 0;

	if (fsz < 12)
		return -1;

	if (pnCwadEnd) *pnCwadEnd = fsz - 8;

	fseek(pFile, -8, SEEK_END);
	if (fread(buffer, sizeof(UInt32), 2, pFile) < 2) return -1;
	if (buffer[0] == ID_CWAD) {
		fseek(pFile, buffer[1], SEEK_SET);
		if (fread(buffer, sizeof(UInt32), 1, pFile) < 1) return -1;
		if (buffer[0] == ID_CWAD)
			return buffer[1];
	}

	if (fsz < 132)
		return -1;

	for (sep = fsz - 12; sep >= fsz - 132; sep -= 8) {
		fseek(pFile, sep, SEEK_SET);
		if (fread(buffer, sizeof(UInt32), 1, pFile) < 1) return -1;

		if (buffer[0] == ID_SEP) {
			for (i = sep + 4; i < fsz; i += 8) {
				fseek(pFile, i, SEEK_SET);
				if (fread(buffer, sizeof(UInt32), 2, pFile) < 2) return -1;

				offset = buffer[0];
				if (pnCwadEnd) *pnCwadEnd = offset + buffer[1] - 8;
				fseek(pFile, offset + buffer[1] - 8, SEEK_SET);
				if (fread(buffer, sizeof(UInt32), 2, pFile) < 2) return -1;
				if (buffer[0] == ID_CWAD) {
					fseek(pFile, offset + buffer[1], SEEK_SET);
					if (fread(buffer, sizeof(UInt32), 1, pFile) < 1) return -1;
					if (buffer[0] == ID_CWAD)
						return offset + buffer[1];
				}
			}

			break;
		}
	}

	return -1;
}

void CWadDecryptData(UInt8 *pBuffer, unsigned long nBufferLength)
{
	if (!pBuffer || nBufferLength == 0) return;
	pBuffer += nBufferLength - 1;
	UInt8 byCWadKey;
	byCWadKey = (UInt8)(66 - (nBufferLength << 1));
	for (unsigned long i = 0; i < nBufferLength; i++) {
		pBuffer[0] ^= (UInt8)(byCWadKey + (i << 1));
		pBuffer--;
	}
}
