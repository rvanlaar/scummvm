#include <cxxtest/TestSuite.h>

#include "common/str.h"
#include "common/platform.h"
#include "engines/director/path.h"

class FilePathTestSuite : public CxxTest::TestSuite {
	public:
	void test_dirseparator_win() {
		struct Director::PathVar vars = {400, Common::kPlatformWindows};
		TS_ASSERT_EQUALS(
				vars.getDirSeparator(),
				'\\'
		);
	}
	void test_dirseparator_mac() {
		struct Director::PathVar vars = {400, Common::kPlatformMacintosh};
		TS_ASSERT_EQUALS(
				vars.getDirSeparator(),
				':'
		);
	}

	void test_testnewpath() {
		struct Director::PathVar vars = {400, Common::kPlatformMacintosh};
		Common::String path = "string:string";
		TS_ASSERT_EQUALS(
			Director::testNewPath(path, vars),
			true
		);

	}
};