#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <codecvt>
#include <comdef.h>
#include "com_initializer.h"


// or: wide_string( s.begin(),s.end() );
// codecvt_utf16/8 is deprecated from C++17 although it seems to work fine
std::wstring s2ws( const std::string& s )
{
	try
	{
		return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{}
			.from_bytes( s );
	}
	catch( std::range_error& e )
	{
		(void)e;
		size_t length = s.length();
		std::wstring result;
		result.reserve( length );
		for ( size_t i = 0; i < length; i++ )
		{
			result.push_back( s[i] & 0xFF );
		}
		return result;
	}
}

std::string ws2s( const std::wstring& ws )
{
	try
	{
		return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{}
			.to_bytes( ws );
	}
	catch( std::range_error& e )
	{
		(void)e;
		size_t length = ws.length();
		std::string result;
		result.reserve( length );
		for ( size_t i = 0; i < length; i++ )
		{
			result.push_back( ws[i] & 0xFF );
		}
		return result;
	}
}


// error printing
std::string printHresultErrorDescription( HRESULT hres )
{
	_com_error error( hres );
	return ws2s( error.ErrorMessage() );
}

#define ASSERT_HRES_IF_FAILED_( hres ) if ( FAILED ( hres ) )\
	{\
		std::cout << __LINE__\
			<< " "\
			<< printHresultErrorDescription( hres )\
			<< std::endl;\
		std::exit( hres );\
	}

#define ASSERT_HRES_IF_FAILED if ( FAILED ( hres ) )\
	{\
		std::cout << __LINE__\
			<< " "\
			<< printHresultErrorDescription( hres )\
			<< std::endl;\
		std::exit( hres );\
	}

template<typename T>
void removeByBackSwap( std::vector<typename T>& v,
	std::size_t index )
{
	auto& pback = v.back();
	std::swap( v[index], pback );
	v.pop_back();
}

template<class Container>
inline std::size_t getIndexOfBack( Container& t )
{
	return t.size() - 1;
}
