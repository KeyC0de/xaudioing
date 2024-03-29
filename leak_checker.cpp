#if defined(_DEBUG) || defined(DEBUG) || defined(debug) || defined(_debug)

#	include "leak_checker.h"
#	include <iostream>

namespace debugLeak
{

class LeakChecker
{
public:
	LeakChecker()
	{
		std::cerr << "Memory leak checker setup" << '\n';
		setupLeakChecker();
	}
	~LeakChecker()
	{
		if ( anyMemoryLeaks() )
		{
			std::cerr << "Leaking.." << '\n';
		}
		else
		{
			OutputDebugStringW( L"No leaks. : )\n" );
			std::cerr << "No leaks. : )\n";
		}
	}
	static inline void setupLeakChecker()
	{
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_REPORT_FLAG
			| _CRTDBG_LEAK_CHECK_DF );
		return;
	}
};

LeakChecker leakChecker{};

};// namespace debugLeak

#endif	// _DEBUG