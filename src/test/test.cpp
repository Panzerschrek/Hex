#include <iostream>

#include "test.h"

static std::vector<t_TestFuncData>& GetFuncsContainer()
{
	static std::vector<t_TestFuncData> funcs;
	return funcs;
}

static int RunTests()
{
	unsigned int failed= 0;
	unsigned int passed= 0;

	std::vector<t_TestFuncData>& funcs= GetFuncsContainer();

	for( const t_TestFuncData& func : funcs )
	{
		std::cout << "Running \"" << func.name << "\"";

		t_TestResult test_result = func.func();
		if( !test_result.failed_expressions.empty() )
		{
			std::cout << " [FAILED]" << std::endl;
			for( const char* expr : test_result.failed_expressions )
				std::cout << "Expression \"" << expr << "\" is false\n";

			failed++;
		}
		else
		{
			std::cout << " [OK]" << std::endl;
			passed++;
		}
	}

	std::cout << funcs.size() << " test runned. Passed: " << passed << " failed: " << failed << std::endl;

	return -int(failed);
}

t_TestId t_AddTestFuncPrivate( const t_TestFuncData& func_data )
{
	std::vector<t_TestFuncData>& funcs= GetFuncsContainer();
	funcs.push_back(func_data);

	return funcs.size() - 1u;
}

int main()
{
	return RunTests();
}
