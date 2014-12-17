#include "t/common_functions.h"
#include "common.h"
#include <string>
#include <boost/test/unit_test.hpp>

using boost::unit_test::test_suite;

void test_shell_escape(void)
{
	using namespace vdr_burn;
	using namespace std;

	string path = "/video0/Claire's \"Erbe\"";
	string escaped = shell_escape(path, "'");
	BOOST_CHECK_EQUAL(escaped, "/video0/Claire\\'s \"Erbe\"");
	escaped = shell_escape(path, "\"");
	BOOST_CHECK_EQUAL(escaped, "/video0/Claire's \\\"Erbe\\\"");
}
