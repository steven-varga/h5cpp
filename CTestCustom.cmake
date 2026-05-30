set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 1048576)
set(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE 1048576)
set(CTEST_CUSTOM_MAXIMUM_NUMBER_OF_ERRORS 100)
set(CTEST_CUSTOM_MAXIMUM_NUMBER_OF_WARNINGS 100)

# Coverage scope — restrict the CDash Coverage step to the h5cpp library
# headers only.  Without these excludes CTest folds every instrumented
# .gcda it can find into the dashboard, so vendored thirdparty libraries,
# the test harness, and the example translation units inflate the LOC
# denominator and bury the real library figure (observed: 9276 LOC counted
# vs. 2453 library lines).  These patterns are POSIX regexes matched against
# the absolute source path and mirror the lcov --remove list in the CI
# coverage job (ci.yml) so CDash and Codecov report the same number.
set(CTEST_CUSTOM_COVERAGE_EXCLUDE
    ${CTEST_CUSTOM_COVERAGE_EXCLUDE}
    "/thirdparty/"
    "/test/"
    "/examples/"
    "/usr/"
    "/CMakeFiles/"
)
