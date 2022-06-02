cmake_minimum_required(VERSION 3.0.0)
project(veeam-test-problem VERSION 0.1.0)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(BOOST_ROOT_USER "C:\\Program Files\\boost_1_79_0" "/usr/local/boost_1_79_0")
find_package(Boost 1.79.0 PATHS ${BOOST_ROOT_USER})
find_package(Boost 1.79.0)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  message("Boost found in ${Boost_INCLUDE_DIRS} \n")
else()
  message("!!! Boost not found, please check BOOST_ROOT or set BOOST_ROOT_USER !!!\n")
endif()

add_executable(veeam-test-problem
  src/main.cpp
  src/chunk.hpp
  src/file_functions.hpp
  src/ts_queue.hpp
  src/thread_pool.hpp
  src/hasher.hpp
)

set_target_properties(veeam-test-problem PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )


target_link_libraries(veeam-test-problem PRIVATE Threads::Threads)

target_include_directories(veeam-test-problem PRIVATE src)



