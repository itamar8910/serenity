set(SOURCES
    Decoder.cpp
    Encoder.cpp
    Endpoint.cpp
    Message.cpp
)

serenity_lib(LibIPC ipc)
target_link_libraries(LibIPC LibC LibCore)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
serenity_shared_lib(LibIPCShared ipc)
target_link_libraries(LibIPCShared LibCShared LibCoreShared)
