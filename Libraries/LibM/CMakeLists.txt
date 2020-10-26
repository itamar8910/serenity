set(SOURCES
    math.cpp
)

serenity_libc(LibM m)
target_include_directories(LibM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(LibM LibC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
serenity_libc_shared(LibMShared m)
target_link_libraries(LibMShared LibCShared)
