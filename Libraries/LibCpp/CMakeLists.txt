set(SOURCES
    Lexer.cpp
)

serenity_lib(LibCpp cpp)
target_link_libraries(LibCpp LibC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
serenity_shared_lib(LibCppShared cpp)
target_link_libraries(LibCppShared LibCShared)
