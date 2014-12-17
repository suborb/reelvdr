#include <boost/test/unit_test.hpp>
#include "t/common_functions.h"

#include "scanner.h"

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite(int, char*[])
{
	test_suite* test = BOOST_TEST_SUITE("Burn Plugin Unit Tests");
	test->add(BOOST_TEST_CASE(&test_shell_escape));
	return test;
}
