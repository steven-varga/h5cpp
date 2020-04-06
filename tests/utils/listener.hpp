/*
 * Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#include "gtest/gtest.h"


#ifndef H5CPP_GTEST_EVENT_LISTENER
#define H5CPP_GTEST_EVENT_LISTENER

#ifdef H5CPP_TEST_NOCOLOR
    #define KRED  ""
    #define KGRN  ""
    #define KNRM  ""
#else
    #define KRED  "\x1B[31m"
    #define KGRN  "\x1B[32m"
    #define KNRM  "\x1B[0m"
#endif
namespace testing { namespace internal {
	/* 
	 */
	template<> inline std::string GetTypeName<std::string>() {
		return std::string("std::string");
	}
}}

class MinimalistPrinter : public ::testing::EmptyTestEventListener {
	public:
	MinimalistPrinter( const std::string& file_name ) : file_name(file_name){};
     virtual void OnTestEnd(const ::testing::TestInfo& ti) {
		auto status =  ti.result()->Passed() ? KGRN "[  OK  ]" KNRM :  KRED "[FAILED]" KNRM;

		printf("%-40s %35s %-6s\n",
				 				  ti.name(), ti.test_case_name(), status );
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
		printf("PASSED: %i  FAILED: %i TIME: %li ms \n", npass, nfail, ut.elapsed_time()  );
		printf("-------------------------------------------------------------------------------------- %s\n", KNRM);
	}

	std::string file_name;
};

    #undef KRED
    #undef KGRN
    #undef KNRM
#endif
