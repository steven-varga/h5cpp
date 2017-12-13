/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this  software  and associated documentation files (the "Software"), to deal in
 * the Software  without   restriction, including without limitation the rights to
 * use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 * of the Software, and to  permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 * IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "gtest/gtest.h"

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KNRM  "\x1B[0m"

namespace testing { namespace internal {
	/* 
	 */
	template<> std::string GetTypeName<std::string>() {
		return std::string("std::string");
	}
}}

class MinimalistPrinter : public ::testing::EmptyTestEventListener {
	public:
	MinimalistPrinter( const std::string& file_name ) : file_name(file_name){};
     virtual void OnTestEnd(const ::testing::TestInfo& ti) {
		// TimeInMillis elapsed_time() const { return elapsed_time_; }
		auto status =  ti.result()->Passed() ? KGRN "[  OK  ]" KNRM :  KRED "[FAILED]" KNRM;

		printf("%-40s %35s %-6s\n",
				 				  ti.name(), ti.type_param(), status );
	}

	virtual void OnTestProgramStart(const testing::UnitTest& ut){
		printf("\n\n--------------------------------------------------------------------------------------\n");
		printf(" RUNNING TEST SUITE:    %60s \n",file_name.data() );
		printf("--------------------------------------------------------------------------------------\n");
	}
	virtual void OnTestProgramEnd(const testing::UnitTest& ut) {

		int npass = ut.successful_test_case_count();
		int nfail = ut.failed_test_case_count();
		auto color = !nfail ?  KGRN : KRED;

		printf("%s-------------------------------------------------------------------------------------- \n", color);
		printf("PASSED: %i  FAILED: %i TIME: %lli ms \n", npass, nfail, ut.elapsed_time()  );
		printf("-------------------------------------------------------------------------------------- %s\n", KNRM);
	}

	std::string file_name;
};
#undef KRED
#undef KGRN
#undef KNRM


