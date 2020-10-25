#include "com_initializer.h"
#include <objbase.h>


COMInitializer::COMInitializer()
{
	m_hres = CoInitializeEx( nullptr, COINIT_MULTITHREADED );// COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
}

COMInitializer::~COMInitializer()
{
	if ( m_hres == S_OK )
	{
		CoUninitialize();
	}
}