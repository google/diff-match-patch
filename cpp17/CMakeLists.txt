cmake_minimum_required(VERSION 3.22)

SET( _PROJECT_NAME diff_match_patch_cpp17 )
project( ${_PROJECT_NAME} )
add_library(${_PROJECT_NAME} 
    STATIC 
        diff_match_patch.cpp 
        diff_match_patch.h 
        diff_match_patch_utils.cpp 
        diff_match_patch_utils.h
)

target_include_directories( ${_PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR} )
target_link_libraries( ${_PROJECT_NAME} )

SET( TEST_NAME "${_PROJECT_NAME}_test" )
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
project( ${TEST_NAME} )
add_executable( ${TEST_NAME} diff_match_patch_test.cpp diff_match_patch_test.h diff_match_patch_test_assertEquals.cpp)
   
target_include_directories( ${TEST_NAME} PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries( diff_match_patch_cpp17_test ${_PROJECT_NAME} )
add_test( ${TEST_NAME} ${TEST_NAME} )
