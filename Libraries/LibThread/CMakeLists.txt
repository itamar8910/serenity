set(SOURCES
    BackgroundAction.cpp
    Thread.cpp
)

serenity_lib(LibThread thread)
target_link_libraries(LibThread LibC LibCore LibPthread)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
serenity_lib(LibThreadShared thread)
target_link_libraries(LibThreadShared LibCShared LibCoreShared LibPthreadShared)
