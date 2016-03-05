#pragma once
#include <vector>

struct t_TestResult
{
	std::vector<const char*> failed_expressions;
};

typedef t_TestResult (t_TestFunc)();

typedef unsigned int t_TestId;

struct t_TestFuncData
{
	t_TestFunc* func;
	const char* name;
};

t_TestId t_AddTestFuncPrivate( const t_TestFuncData& func_data );


/*
Create test. Usage:

H_TEST(TestName)
{
// test body
}
 */
#define H_TEST(NAME) \
static void NAME##Func( t_TestResult& );\
\
static const t_TestId NAME##variable=\
	t_AddTestFuncPrivate(\
		t_TestFuncData{\
		[]() -> t_TestResult\
		{\
			t_TestResult result;\
			NAME##Func(result);\
			return result;\
		},\
		#NAME} );\
\
static void NAME##Func( t_TestResult& test_private_result )

#define H_TEST_EXPECT_PRIVATE(x, returner)\
	if(!(x))\
	{\
		test_private_result.failed_expressions.push_back(#x);\
		returner\
	}

/*
Usage: H_TEST_EXPECT(expressin)
if expression if false, test failed, but continuing.
*/
#define H_TEST_EXPECT(x)\
	H_TEST_EXPECT_PRIVATE(x, )

/*
Usage: H_TEST_EXPECT(expressin)
if expression if false, test failed and aborted
*/
#define H_TEST_ASSERT(x)\
	H_TEST_EXPECT_PRIVATE(x, return;)
