set(SOURCES
    pthread.cpp
)

serenity_libc(LibPthread pthread)
target_link_libraries(LibPthread LibC)
target_include_directories(LibPthread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
serenity_libc_shared(LibPthreadShared pthread)
target_link_libraries(LibPthreadShared LibCShared)
