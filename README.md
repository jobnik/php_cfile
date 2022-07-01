## php_cfile
PHP extension, that wraps Windows 64bit FILE functions to support in older PHP versions

cfile = C/C++ File

## Why?
Got tired of PHP 5.5 (that I'm using), that doesn't support filesize > 2GB and a lack of UTF-8 filename support in built in file functions.

## Included file functions
Prefix "cfile_" and then a function name as standard built in functions

* cfile_filesize
* cfile_fopen
* cfile_fseek
* cfile_ftell
* cfile_fgets
* cfile_fread
* cfile_fwrite
* cfile_fstat
* cfile_feof
* cfile_fclose
* cfile_popen
* cfile_pclose

## Usage
Same parameters as in standard built in functions, parameters are mandatory (in PHP there're some parameters that can be left optional. It could be added here as well using built in PHP functions for number of argument check...)

Get filesize
```php
/*
		Find file size for any file.

		"cfile_filesize" function usage:

		IN:
			fp		: file path (UTF-8 compatible)
			method	: optional
				-1	- finds file size using wstat64 (default)
				0 	- finds file size using GetFileAttributesEx
				1 	- finds file size using CreateFile
				2 	- finds file size using FindFirstFile

		OUT:
			File size as string, to return large file sizes (> 2GB and up to 9 PetaBytes)
*/
	echo cfile_filesize("filesize.php");	// change for your file path
```

fopen and fread
```php
<?php
	if (!($fh = cfile_fopen("test.txt", "r")))
		echo "Couldn't open file stream";
	else {
		while (!cfile_feof($fh)) {
			echo cfile_fread($fh, 1024);
		}

		cfile_fclose($fh);
	}
?>
```

## License
BSD 3-Clause License

[LICENSE](https://github.com/jobnik/php_cfile/blob/main/LICENSE)
