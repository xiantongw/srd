#!/bin/bash
# 在终端中执行此脚本完成迁移

cd /workspaces/srd

# 1. 备份当前状态
git add -A
git commit -m "Backup before CMake migration" || true

# 2. 如果有嵌套的srd目录，移动到根目录
if [ -d "srd/common" ]; then
    echo "Moving directories from srd/ to root..."
    mv srd/common ./
    mv srd/record ./
    mv srd/storage ./
    rmdir srd
fi

# 3. 调整include目录结构（如果需要）
# 确保是 include/模块名/ 而不是 include/srd/模块名/
for module in common record storage; do
    if [ -d "${module}/include/srd/${module}" ]; then
        echo "Restructuring ${module}/include..."
        mv "${module}/include/srd/${module}" "${module}/include/${module}.tmp"
        rm -rf "${module}/include/srd"
        mv "${module}/include/${module}.tmp" "${module}/include/${module}"
    fi
done

# 4. 创建根CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(srd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译器警告
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 依赖
find_package(spdlog REQUIRED)

# 测试
enable_testing()
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 子目录
add_subdirectory(common)
add_subdirectory(record)
add_subdirectory(storage)
EOF

# 5. 创建common/CMakeLists.txt
cat > common/CMakeLists.txt << 'EOF'
add_library(common INTERFACE)

target_include_directories(common INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
EOF

# 6. 创建record/CMakeLists.txt
cat > record/CMakeLists.txt << 'EOF'
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
EOF

# 7. 创建storage/CMakeLists.txt
cat > storage/CMakeLists.txt << 'EOF'
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
EOF

# 8. 更新.gitignore
cat >> .gitignore << 'EOF'

# CMake build outputs
build/
cmake-build-*/
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
Makefile
*.cmake
!CMakeLists.txt
CTestTestfile.cmake
Testing/
compile_commands.json

# Build artifacts
bin/
lib/
*.o
*.a
*.so
*.exe
EOF

# 9. 删除Bazel文件
echo "Removing Bazel files..."
rm -f MODULE.bazel .bazelrc .bazelversion
find . -name "BUILD.bazel" -type f -delete
rm -rf bazel-*

echo "Migration completed!"
echo ""
echo "Next steps:"
echo "1. Install spdlog: sudo apt-get update && sudo apt-get install -y libspdlog-dev"
echo "2. Build: mkdir -p build && cd build && cmake .. && make -j"
echo "3. Test: ctest --output-on-failure"