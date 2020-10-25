#pragma once

#include "winner.h"

#ifdef _DEBUG

#	ifdef __cplusplus
extern "C" {
#	endif

#	ifdef UNICODE
#		define LSTR( expr ) L ## expr
#		define STRINGIFY( expr ) LSTR( #expr )
extern bool assertPrint( const wchar_t* expr,
	const wchar_t* file,
	int line,
	const wchar_t* function,
	const wchar_t* msg );

#		define ASSERT( arg, msg ) (!( arg ) \
			&& assertPrint( STRINGIFY( arg ),\
				__FILEW__,\
				__LINE__,\
				__function__,\
				msg ) )
#	else
#		define STRINGIFY( expr ) #expr
extern bool assertPrint( const char* expr,
	const char* file,
	int line,
	const char* function,
	const char* msg );

#		define ASSERT( arg, msg ) (!(arg) \
			&& assertPrint( STRINGIFY( arg ),\
				__FILE__,\
				__LINE__,\
				__function__,\
				msg ) )
#	endif // UNICODE

#	ifdef __cplusplus
}
#	endif

#else
// Release
#	define ASSERT(arg, msg) void
#endif // _DEBUG
