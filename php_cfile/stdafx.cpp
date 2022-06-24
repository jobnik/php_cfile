// stdafx.cpp : source file that includes just the standard includes
// php_filesize.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// from: https://stackoverflow.com/questions/8991192/check-filesize-without-opening-file-in-c

#ifdef WIN32

// get file size using CreateFile
__int64 FileSize_CreateFile(const wchar_t* name)
{
    HANDLE hFile = CreateFile(name, GENERIC_READ, 
								FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
								FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return -1; // error condition, could call GetLastError to find out more

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size))
    {
        CloseHandle(hFile);
        return -1; // error condition, could call GetLastError to find out more
    }

    CloseHandle(hFile);

	return size.QuadPart;
}

// get file size using GetFileAttributesEx
__int64 FileSize_GetFileAttributesEx(const wchar_t* name)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;

	if (!GetFileAttributesEx(name, GetFileExInfoStandard, &fad))
        return -1; // error condition, could call GetLastError to find out more

    LARGE_INTEGER size;

    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;

    return size.QuadPart;
}

// get file size using FindFirstFile
__int64 FileSize_FindFirstFile(const wchar_t* name)
{
	WIN32_FIND_DATA find_data;
	HANDLE find_file = FindFirstFile(name, &find_data);

	LARGE_INTEGER size;

	if(find_file && find_file != INVALID_HANDLE_VALUE){
		size.LowPart = find_data.nFileSizeLow;
		size.HighPart = find_data.nFileSizeHigh;
		FindClose(find_file);

		return size.QuadPart;
	}

	return -1;
}
#endif

// get file size using "wstat"
__int64 FileSize_Stat(const wchar_t* name)
{
	struct __stat64 buffer;

    if (_wstat64(name, &buffer) != 0)
        return -1; // error, could use errno to find out more

	return buffer.st_size;
}

// handle UTF-8 file path (from WFIO sources https://github.com/kenjiuno/php-wfio)
void char2wchar(char *s, wchar_t *sw)
{
	char fp_path[CFILE_MAX_PATH] = {0};
	BOOL fUsedDefaultChar = FALSE;
	//wchar_t fpw[CFILE_MAX_PATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, s, strlen(s), sw, CFILE_MAX_PATH);
	WideCharToMultiByte(CP_ACP, 0, sw, -1, fp_path, MAXPATHLEN, NULL, &fUsedDefaultChar);
}
