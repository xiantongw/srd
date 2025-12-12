add_library(record
    src/field.cpp
    src/tuple.cpp
)

target_include_directories(record
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(record PUBLIC common)

if(BUILD_TESTING)
    add_executable(field_test test/field_test.cpp)
    target_link_libraries(field_test PRIVATE record GTest::gtest_main)
    add_test(NAME field_test COMMAND field_test)

    add_executable(tuple_test test/tuple_test.cpp)
    target_link_libraries(tuple_test PRIVATE record GTest::gtest_main)
    add_test(NAME tuple_test COMMAND tuple_test)
endif()
