add_executable(test_asyncworker
    test_asyncworker.cpp
)

target_link_libraries(test_asyncworker
    PRIVATE
        cppasyncworker::cppasyncworker
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_asyncworker)
