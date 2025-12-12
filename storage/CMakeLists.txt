add_library(storage
    src/slotted_page.cpp
    src/storage_manager.cpp
)

target_include_directories(storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(storage
    PUBLIC
        common
        record
        spdlog::spdlog
)

if(BUILD_TESTING)
    add_executable(slotted_page_test test/slotted_page_test.cpp)
    target_link_libraries(slotted_page_test PRIVATE storage GTest::gtest_main)
    add_test(NAME slotted_page_test COMMAND slotted_page_test)

    add_executable(storage_manager_test test/storage_manager_test.cpp)
    target_link_libraries(storage_manager_test PRIVATE storage GTest::gtest_main)
    add_test(NAME storage_manager_test COMMAND storage_manager_test)
endif()
