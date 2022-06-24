/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: jobnik <arthur@jobnik.net>                                   |
  +----------------------------------------------------------------------+
  | PHP_CFILE                                                            |
  |                                                                      |
  | Copyright (c) 2019 (: JOBnik! :) Arthur Aminov, Israel               |
  | https://www.jobnik.net                                               |
  |                                                                      |
  | Tested on PHP x86/x64 5.5.38 using VC11 (Visual Studio 2012)         |
  |                                                                      |
  | Note: For compiling in other VC, please define below in              |
  |       "php_src/main/config.w32.h"                                    |
  |                                                                      |
  |	      #define PHP_COMPILER_ID "VC11"                                 |
  +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "stdafx.h"

// declaration of functions to be exported
ZEND_FUNCTION(cfile_filesize);
ZEND_FUNCTION(cfile_fopen);
ZEND_FUNCTION(cfile_fseek);
ZEND_FUNCTION(cfile_ftell);
ZEND_FUNCTION(cfile_fgets);
ZEND_FUNCTION(cfile_fread);
ZEND_FUNCTION(cfile_fwrite);
ZEND_FUNCTION(cfile_fstat);
ZEND_FUNCTION(cfile_feof);
ZEND_FUNCTION(cfile_fclose);
ZEND_FUNCTION(cfile_popen);
ZEND_FUNCTION(cfile_pclose);

// compiled function list so Zend knows what's in this module
zend_function_entry phpExtModule_functions[] = {
    ZEND_FE(cfile_filesize, NULL)
	ZEND_FE(cfile_fopen, NULL)
    ZEND_FE(cfile_fseek, NULL)
	ZEND_FE(cfile_ftell, NULL)
	ZEND_FE(cfile_fgets, NULL)
	ZEND_FE(cfile_fread, NULL)
	ZEND_FE(cfile_fwrite, NULL)
	ZEND_FE(cfile_fstat, NULL)
	ZEND_FE(cfile_feof, NULL)
	ZEND_FE(cfile_fclose, NULL)
	ZEND_FE(cfile_popen, NULL)
	ZEND_FE(cfile_pclose, NULL)
	PHP_FE_END
};

// compiled module information
zend_module_entry phpExtModule_module_entry = {
    STANDARD_MODULE_HEADER,
    "php_cfile",
    phpExtModule_functions,
	PHP_MINIT(cfile),
	PHP_MSHUTDOWN(cfile),
	NULL,
	NULL,
	NULL,
    NO_VERSION_YET, STANDARD_MODULE_PROPERTIES
};

// implement standard "stub" routine to introduce ourselves to Zend
ZEND_GET_MODULE(phpExtModule)

PHP_MINIT_FUNCTION(cfile)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/

	le_cfile = zend_register_list_destructors_ex(cfile_dtor, NULL, RES_TYPE_CFILE, module_number);
	le_cfile_proc = zend_register_list_destructors_ex(cfile_proc_dtor, NULL, RES_TYPE_CFILE_PROC, module_number);
	le_cfile_stat = zend_register_list_destructors_ex(cfile_stat_dtor, NULL, RES_TYPE_CFILE_STAT, module_number);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(cfile)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}

void error_resource(const char *func, const char *call_func)
{
	char c[100] = {0};
	func += 4;	// remove zif_ prefix from __FUNCTION__
	sprintf(c, "%s(): supplied resource is not a valid \"%s\" resource", func, call_func);
	zend_error(E_WARNING, c);
}

//
// cfile_filesize - Gets the size for the given file
// --------------
//
// IN:
//		*fpa	: File path (UTF-8 compatible)
//		method	: one of the FileSize_XXX functions (optional)
//
// OUT:
//		File size bytes as double, to return large file sizes (> 2GB as in php)
//
ZEND_FUNCTION(cfile_filesize)
{
	char *fpa = NULL;
	int fp_len = 0;
	int method = -1;

	wchar_t fpw[CFILE_MAX_PATH] = {0};
	char buf_cwd[MAXPATHLEN] = {0};
	__int64 fsize = -1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &fpa, &fp_len, &method) == FAILURE)
        RETURN_FALSE;

	char2wchar(fpa, fpw);

	// file doesn't exists? try to get current dir and concat to file
	if ((fsize = FileSize_Stat(fpw)) == -1) {
		if (VCWD_GETCWD(buf_cwd, MAXPATHLEN)) {
			strcat(buf_cwd, "\\");
			strcat(buf_cwd, fpa);

			memset(fpw, 0, CFILE_MAX_PATH);
			char2wchar(buf_cwd, fpw);
		}
	}

	switch (method) {
		#ifdef WIN32
			case 0: fsize = FileSize_GetFileAttributesEx(fpw); break;
			case 1: fsize = FileSize_CreateFile(fpw); break;
			case 2: fsize = FileSize_FindFirstFile(fpw); break;
		#endif
		default:
			fsize = fsize != -1 ? fsize : FileSize_Stat(fpw);
	}

#ifdef STR_OUT
	char fsizes[20];	// more than enough
	sprintf(fsizes, "%I64d", fsize);
	RETURN_STRING(fsizes, 1);
#else
	RETURN_DOUBLE(fsize);
#endif
}

//
// cfile_fopen - Opens file and binds a named resource, specified by filename, to a stream
// -----------
//
// IN:
//		*fpa	: File path (UTF-8 compatible)
//		mode	: Kind of access that's enabled (e.g: "r", "w", "a", "r+", "w+", "a+", "rt", "rb" ...)
//
// OUT:
//		Returns a file pointer resource on success, or FALSE on error
//
// INFO:
//		https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=vs-2017
//
ZEND_FUNCTION(cfile_fopen)
{
	char *fpa = NULL;
	int fpa_len = 0;
	char *mode = NULL;
	int mode_len = 0;

	wchar_t fpw[CFILE_MAX_PATH] = {0};
	char buf_cwd[MAXPATHLEN] = {0};
	wchar_t modew[CFILE_MAX_PATH] = {0};
	FILE *fp = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &fpa, &fpa_len, &mode, &mode_len) == FAILURE)
        RETURN_FALSE;

	char2wchar(fpa, fpw);

	// file doesn't exists? try to get current dir and concat to file
	if (FileSize_Stat(fpw) == -1) {
		if (VCWD_GETCWD(buf_cwd, MAXPATHLEN)) {
			strcat(buf_cwd, "\\");
			strcat(buf_cwd, fpa);

			memset(fpw, 0, CFILE_MAX_PATH);
			char2wchar(buf_cwd, fpw);
		}
	}

	char2wchar(mode, modew);

	if (!(fp = _wfopen(fpw, modew))) {
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, fp, le_cfile);
}

//
// cfile_fseek - Seeks on a file pointer
// -----------
//
// IN:
//		*res	: A file system pointer resource that is typically created using cfile_fopen(), fopen() or wfio_fopen8()
//		offset	: The offset as double to support large seek positions
//		whence	: SEEK_SET/SEEK_CUR/SEEK_END (optional, SEEK_SET default)
//
// OUT:
//		Upon success, returns 0; otherwise, returns -1
//
ZEND_FUNCTION(cfile_fseek)
{
	zval *res = NULL;		// resource
	int whence = SEEK_SET;
	FILE *fp = NULL;
	php_stream *stream = NULL;

#ifdef STR_OUT
	char *offset = NULL;	// offset as string to support large seek pos in 32 bit PHP
	int offset_length;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &res, &offset, &offset_length, &whence) == FAILURE)
#else
	double offset = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rd|l", &res, &offset, &whence) == FAILURE)
#endif
		RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (!fp) {
		php_stream_from_zval(stream, &res);

		if (php_stream_cast(stream, PHP_STREAM_AS_STDIO, (void**)&fp, 0) == FAILURE) {
			// check if stream created using WFIO extension
			struct php_wfio_stream_data_t *self = (struct php_wfio_stream_data_t *) stream->abstract;

			if (stream->abstract)
				fp = self->pf;
			else {
				error_resource(__FUNCTION__, "cfile_fopen or fopen");
				RETURN_FALSE;
			}
		}
	}

#ifdef STR_OUT
	RETURN_LONG(_fseeki64(fp, _atoi64(offset), whence));
#else
	RETURN_LONG(_fseeki64(fp, offset, whence));
#endif
}

//
// cfile_ftell - Returns the current position of the file read/write pointer
// -----------
//
// IN:
//		*res : Target FILE structure (The file pointer must be valid, and must point to a file successfully opened by cfile_fopen())
//
// OUT:
//		Returns the position of the file pointer referenced by handle as double; i.e., its offset into the file stream or false on error.
//
ZEND_FUNCTION(cfile_ftell)
{
	zval *res = NULL;
	FILE *fp = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &res) == FAILURE)
        RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (!fp) {
		// check if created using "cfile_popen"
		ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile_proc);

		if (!fp) {
			error_resource(__FUNCTION__, "cfile_fopen/popen");
			RETURN_FALSE;
		}
	}

#ifdef STR_OUT
	char fposs[20];
	sprintf(fposs, "%I64d", _ftelli64(fp));
	RETURN_STRING(fposs, 1);
#else
	RETURN_DOUBLE(_ftelli64(fp));
#endif
}

//
// cfile_fgets - Gets line from file pointer
// -----------
//
// IN:
//		*res	: A file system pointer resource that is typically created using cfile_fopen()
//		length	: Reading ends when length - 1 bytes have been read, or a newline (which is included in the return value), or an EOF (whichever comes first).
//
// OUT:
//		Returns a string of up to length - 1 bytes read from the file pointed to by stream. If there is no more data to read in the file pointer, then false is returned.
//
//		If an error occurs, false is returned.
//
ZEND_FUNCTION(cfile_fgets)
{
	zval *res = NULL;
	FILE *fp = NULL;

#ifdef STR_OUT
	char *length = NULL;
	int length_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &res, &length, &length_len) == FAILURE)
#else
	double length = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rd", &res, &length) == FAILURE)
#endif
        RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (!fp) {
		// check if created using "cfile_popen"
		ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile_proc);

		if (!fp) {
			error_resource(__FUNCTION__, "cfile_fopen/popen");
			RETURN_FALSE;
		}
	}

#ifdef STR_OUT
	size_t bsize = _atoi64(length);
#else
	size_t bsize = length;
#endif

	if (bsize <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Length parameter must be greater than 0");
		RETURN_FALSE;
	}

	char *buf = (char*)ecalloc(bsize + 1, sizeof(char));

	if (!buf || !fgets(buf, bsize, fp)) {
		if (buf) efree(buf);
		RETURN_FALSE;
	}

	// needed because recv/read/gzread doesn't put a null at the end
	buf[bsize] = 0;

	RETURN_STRINGL(buf, strlen(buf), 0);
}

//
// cfile_fread - File read
// -----------
//
// IN:
//		*res	: A file system pointer resource that is typically created using cfile_fopen()
//		size	: Up to length number of bytes read as double to support over 2GB PHP limitation
//
// OUT:
//		Returns the read string or FALSE on failure
//
ZEND_FUNCTION(cfile_fread)
{
	zval *res = NULL;
	FILE *fp = NULL;

#ifdef STR_OUT
	char *size = NULL;
	int size_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &res, &size, &size_len) == FAILURE)
#else
	double size = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rd", &res, &size) == FAILURE)
#endif
        RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (!fp) {
		// check if created using "cfile_popen"
		ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile_proc);

		if (!fp) {
			error_resource(__FUNCTION__, "cfile_fopen/popen");
			RETURN_FALSE;
		}
	}

#ifdef STR_OUT
	size_t bsize = _atoi64(size);
#else
	size_t bsize = size;
#endif

	if (bsize <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Size parameter must be greater than 0");
		RETURN_FALSE;
	}

	char *buf = (char*)emalloc(bsize + 1);

	size_t bytes = fread(buf, sizeof(char), bsize, fp);

	// needed because recv/read/gzread doesn't put a null at the end
	buf[bytes] = 0;

	RETURN_STRINGL(buf, bytes, 0);
}

//
// cfile_fwrite - File write
// ------------
//
// IN:
//		*res	: A file system pointer resource that is typically created using cfile_fopen()
//		*buf	: The string that is to be written
//		size	: If the length argument is given, writing will stop after length bytes have been written or the end of string is reached, whichever comes first
//
// OUT:
//		Returns the number of bytes written, or FALSE on error
//
ZEND_FUNCTION(cfile_fwrite)
{
	zval *res = NULL;
	char *buf = NULL;
	long buf_len = 0;
	FILE *fp = NULL;

#ifdef STR_OUT
	char *size = NULL;
	int size_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|s", &res, &buf, &buf_len, &size, &size_len) == FAILURE)
#else
	double size = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|d", &res, &buf, &buf_len, &size) == FAILURE)
#endif
        RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (!fp) {
		// check if created using "cfile_popen"
		ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile_proc);

		if (!fp) {
			error_resource(__FUNCTION__, "cfile_fopen/popen");
			RETURN_FALSE;
		}
	}

	size_t wsize = 0;

#ifdef STR_OUT
	__int64 bsize = _atoi64(size);
#else
	__int64 bsize = size;
#endif

	bsize = bsize > 0 ? bsize : buf_len;

	if (!(wsize = fwrite(buf, sizeof(char), bsize, fp)))
		RETURN_FALSE;

#ifdef STR_OUT
	char fws[20];
	sprintf(fws, "%I64d", wsize);
	RETURN_STRING(fws, 1);
#else
	RETURN_DOUBLE(wsize);
#endif
}

//
// cfile_fstat - Gets information about a file using an open file pointer
// -----------
//
// IN:
//		*res	: A file system pointer resource that is typically created using cfile_fopen()
//
// OUT:
//		Returns an array with the statistics of the file or FALSE on failure.
//
ZEND_FUNCTION(cfile_fstat)
{
	zval *res = NULL;
	FILE *fp = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &res) == FAILURE)
        RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (!fp) {
		// check if created using "cfile_popen"
		ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile_proc);

		if (!fp) {
			error_resource(__FUNCTION__, "cfile_fopen/popen");
			RETURN_FALSE;
		}
	}

	struct _stati64 buf;

	if (!_fstati64(fileno(fp), &buf))
	{
		array_init(return_value);

#ifdef STR_OUT
		char st_atime[20];
		sprintf(st_atime, "%I64d", buf.st_atime);
		add_assoc_string(return_value, "atime", st_atime, 1);

		char st_ctime[20];
		sprintf(st_ctime, "%I64d", buf.st_ctime);
		add_assoc_string(return_value, "ctime", st_ctime, 1);

		char st_mtime[20];
		sprintf(st_mtime, "%I64d", buf.st_mtime);
		add_assoc_string(return_value, "mtime", st_mtime, 1);

		char st_size[20];
		sprintf(st_size, "%I64d", buf.st_size);
		add_assoc_string(return_value, "size", st_size, 1);
#else
		add_assoc_double(return_value, "atime", buf.st_atime);
		add_assoc_double(return_value, "ctime", buf.st_ctime);
		add_assoc_double(return_value, "mtime", buf.st_mtime);
		add_assoc_double(return_value, "size", buf.st_size);
#endif
		add_assoc_long(return_value, "dev", buf.st_dev);
		add_assoc_long(return_value, "gid", buf.st_gid);
		add_assoc_long(return_value, "ino", buf.st_ino);
		add_assoc_long(return_value, "mode", buf.st_mode);
		add_assoc_long(return_value, "nlink", buf.st_nlink);
		add_assoc_long(return_value, "rdev", buf.st_rdev);
		add_assoc_long(return_value, "uid", buf.st_uid);
	} else
		RETURN_FALSE;
}

//
// cfile_feof - Tests for end-of-file on a file pointer
// ----------
//
// IN:
//		*res	: The file pointer must be valid, and must point to a file successfully opened by cfile_fopen()
//
// OUT:
//		Returns TRUE if the file pointer is at EOF or an error occurs otherwise returns FALSE.
//
ZEND_FUNCTION(cfile_feof)
{
	zval *res = NULL;
	FILE *fp = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &res) == FAILURE)
        RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (!fp) {
		// check if created using "cfile_popen"
		ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile_proc);

		if (!fp) {
			error_resource(__FUNCTION__, "cfile_fopen/popen");
			RETURN_TRUE;
		}
	}

	if (feof(fp) && !ferror(fp))
		RETURN_TRUE;

	RETURN_FALSE;
}

//
// cfile_fclose - Closes an open file pointer
// ------------
//
// IN:
//		*res	: The file pointer must be valid, and must point to a file successfully opened by cfile_fopen()
//
// OUT:
//		Returns TRUE on success or FALSE on failure.
//
ZEND_FUNCTION(cfile_fclose)
{
	zval *res = NULL;
	FILE *fp = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &res) == FAILURE)
        RETURN_FALSE;

	// check if created using "cfile_fopen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile);

	if (fp)
		fclose(fp);

	if (zend_list_delete(Z_RESVAL_P(res)) != FAILURE)
		RETURN_TRUE;

	error_resource(__FUNCTION__, "cfile_fopen/popen");

	RETURN_FALSE;
}

//
// cfile_popen - Opens process file pointer
// -----------
//
// IN:
//		*command	: Command to be executed
//		*mode		: Mode of the returned stream ("r", "w", "b", "t")
//
// OUT:
//		Returns a file pointer identical to that returned by cfile_fopen(), except that it is unidirectional (may only be used for reading or writing).
//		If an error occurs, returns FALSE.
//
ZEND_FUNCTION(cfile_popen)
{
	char *command = NULL;
	int command_len = 0;
	char *mode = NULL;
	int mode_len = 0;
	FILE *ph = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &command, &command_len, &mode, &mode_len) == FAILURE)
        RETURN_FALSE;

	wchar_t commandW[CFILE_MAX_PATH] = {0};
	char2wchar(command, commandW);

	wchar_t modeW[CFILE_MAX_PATH] = {0};
	char2wchar(mode, modeW);

	if (!(ph = _wpopen(commandW, modeW)))
		RETURN_FALSE;

	ZEND_REGISTER_RESOURCE(return_value, ph, le_cfile_proc);
}

//
// cfile_pclose - Closes an open file pointer
// ------------
//
// IN:
//		*res	: The file pointer must be valid, and must point to a file successfully opened by cfile_popen()
//
// OUT:
//		Returns TRUE on success or FALSE on failure.
//
ZEND_FUNCTION(cfile_pclose)
{
	zval *res = NULL;
	FILE *fp = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &res) == FAILURE)
        RETURN_FALSE;

	// check if created using "cfile_popen"
	ZEND_FETCH_RESOURCE_NO_RETURN(fp, FILE*, &res, -1, NULL, le_cfile_proc);

	fflush(fp);
	_pclose(fp);

	if (zend_list_delete(Z_RESVAL_P(res)) != FAILURE)
		RETURN_TRUE;

	error_resource(__FUNCTION__, "cfile_popen");

	RETURN_FALSE;
}
