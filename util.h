#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include "com_initializer.h"
#include <comdef.h>

#define pass (void)0;

// error printing
std::wstring printHresultErrorDescription( HRESULT hres )
{
	_com_error error( hres );
	return error.ErrorMessage();
}

#define ASSERT_HRES_IF_FAILED_( hres ) if ( FAILED ( hres ) )\
	{\
		std::wcout << __LINE__\
			<< L" "\
			<< printHresultErrorDescription( hres )\
			<< std::endl;\
		std::exit( hres );\
	}

#define ASSERT_HRES_IF_FAILED if ( FAILED ( hres ) )\
	{\
		std::wcout << __LINE__\
			<< L" "\
			<< printHresultErrorDescription( hres )\
			<< std::endl;\
		std::exit( hres );\
	}

template<typename T>
void removeByBackSwap( std::vector<typename T>& v, std::size_t index )
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

