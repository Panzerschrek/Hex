#include "test.h"

H_TEST(EinzUndEinzTest)
{
	H_TEST_EXPECT(1 + 1 == 2);
}

H_TEST(ZweimalZweiTest)
{
	H_TEST_EXPECT(2 * 2 == 4);
}
