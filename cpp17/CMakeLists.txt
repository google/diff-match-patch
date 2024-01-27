cmake_minimum_required(VERSION 3.22)

find_package(IncludeProjectSettings REQUIRED)
include( ${CMAKE_CURRENT_LIST_DIR}/include.cmake )
project( ${_PROJECT_NAME} )
IncludeProjectSettings(QT ${USE_QT})
add_library(${_PROJECT_NAME} STATIC
    ${_PROJECT_DEPENDENCIES} 
)
set_target_properties( ${_PROJECT_NAME} PROPERTIES FOLDER ${FOLDER_NAME} )
target_include_directories( ${_PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR} )
target_link_libraries( ${_PROJECT_NAME}
    PUBLIC
        ${project_pub_DEPS}
    PRIVATE 
        ${project_pri_DEPS}
)

set( testProjectName "" )
SET( TEST_SOURCE_FILES
    diff_match_patch_test.cpp
    diff_match_patch_test.h
    )
    
SAB_UNIT_TEST(diff_match_patch_cpp17
    diff_match_patch_test.cpp
    "gtest;gmock;diff_match_patch_cpp17"
    testProjectName
    ${TEST_SOURCE_FILES}
    )
    
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set_target_properties( ${testProjectName} PROPERTIES 
                                    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${testProjectName}>" 
                                    VS_DEBUGGER_COMMAND "$<TARGET_FILE:${testProjectName}>" 
                                    VS_DEBUGGER_ENVIRONMENT "PATH=${DEBUG_PATH}" 
)
if( WIN32 )
    set_target_properties( ${testProjectName} PROPERTIES 
        CXX_STANDARD 17
    )
target_include_directories( ${testProjectName} PUBLIC ${CMAKE_SOURCE_DIR})
endif()
