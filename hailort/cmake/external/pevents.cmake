cmake_minimum_required(VERSION 3.11.0)

include(FetchContent)

FetchContent_Declare(
    pevents
    GIT_REPOSITORY https://github.com/neosmart/pevents.git
    GIT_TAG 1209b1fd1bd2e75daab4380cf43d280b90b45366 # Master
    #GIT_SHALLOW TRUE
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/pevents"
    BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/pevents"
)

if(NOT HAILO_OFFLINE_COMPILATION)
    # https://stackoverflow.com/questions/65527126/disable-install-for-fetchcontent
    FetchContent_GetProperties(pevents)
    if(NOT pevents_POPULATED)
        FetchContent_Populate(pevents)
    endif()
endif()

if(NOT TARGET pevents)
    add_library(pevents STATIC EXCLUDE_FROM_ALL ${pevents_SOURCE_DIR}/src/pevents.cpp)
    target_include_directories(pevents PUBLIC ${pevents_SOURCE_DIR}/src)
    target_compile_definitions(pevents PRIVATE -DWFMO)
endif()