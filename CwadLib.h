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
