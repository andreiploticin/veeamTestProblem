cmake_minimum_required(VERSION 3.0.0)
project(veeam-test-problem VERSION 0.1.0)

include(CTest)
enable_testing()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(Boost_INCLUDE_DIRS "C:\\Program Files\\boost_1_79_0")
find_package(Boost 1.79.0)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(veeam-test-problem
  src/main.cpp
  src/chunk.hpp
  src/file_functions.hpp
  src/ts_queue.hpp
  src/thread_pool.hpp
  src/thread_pool.cpp
)

set_target_properties(veeam-test-problem PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )


target_link_libraries(veeam-test-problem PRIVATE Threads::Threads)

target_include_directories(veeam-test-problem PRIVATE src)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)



