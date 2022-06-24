// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef PHP_CFILE_H
	#define PHP_CFILE_H
#endif

#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifdef __cplusplus
	extern "C" {
#endif

	// PHP Extension headers
	#include "zend_config.w32.h"
	#include "php.h"

    /*#ifdef PHP_WIN32
        #define _ALLOW_KEYWORD_MACROS  
        #define PHP_CFILE_API __declspec(dllexport)
		#define PHP_CFILE __declspec(dllimport)
	#endif*/

	/*#ifdef ZTS
		#include "TSRM.h"
	#endif*/

#ifdef __cplusplus
	}
#endif

#define STR_OUT 1	// if not defined will output as double

#define RES_TYPE_CFILE "php_cfile"
#define RES_TYPE_CFILE_PROC "php_cfile_proc"
#define RES_TYPE_CFILE_STAT "php_cfile_stat"

static int le_cfile;
static int le_cfile_proc;
static int le_cfile_stat;

static void cfile_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	FILE *fp = (FILE *) rsrc->ptr;
	fclose(fp);
}

static void cfile_proc_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	/*HANDLE h = (HANDLE) rsrc->ptr;
	CloseHandle(h);*/
	FILE *ph = (FILE *) rsrc->ptr;
	_pclose(ph);

}

static void cfile_stat_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	struct _stati64 *s = (struct _stati64 *) rsrc->ptr;
	if (s) efree(s);

}

#ifndef CFILE_MAX_PATH
    #define CFILE_MAX_PATH 1024 * 10
#endif

struct php_wfio_stream_data_t {
	FILE *pf;
};

PHP_MINIT_FUNCTION(cfile);
PHP_MSHUTDOWN_FUNCTION(cfile);

#ifdef WIN32
__int64 FileSize_CreateFile(const wchar_t* name);
__int64 FileSize_GetFileAttributesEx(const wchar_t* name);
__int64 FileSize_FindFirstFile(const wchar_t* name);
#endif
__int64 FileSize_Stat(const wchar_t* name);
void char2wchar(char *s, wchar_t *sw);