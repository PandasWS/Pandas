// Copyright (c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include <config/pandas.hpp>
#include <common/utf8.hpp>

#ifdef Pandas_Support_UTF8BOM_Files
	#define fopen(FNAME, MODE) PandasUtf8::fopen(FNAME, MODE)
	#define fgets(BUFFER, MAXCOUNT, STREAM) PandasUtf8::fgets(BUFFER, MAXCOUNT, STREAM)
	#define fread(BUFFER, ELESIZE, ELECOUNT, STREAM) PandasUtf8::fread(BUFFER, ELESIZE, ELECOUNT, STREAM)
	#define fclose(FPOINTER) PandasUtf8::fclose(FPOINTER)

	#define _fgets(BUFFER, MAXCOUNT, STREAM, FLAG) PandasUtf8::_fgets(BUFFER, MAXCOUNT, STREAM, FLAG)
	#define _fread(BUFFER, ELESIZE, ELECOUNT, STREAM, FLAG) PandasUtf8::_fread(BUFFER, ELESIZE, ELECOUNT, STREAM, FLAG)
#endif // Pandas_Support_UTF8BOM_Files
