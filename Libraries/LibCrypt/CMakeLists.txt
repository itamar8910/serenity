set(SOURCES
    crypt.cpp
)

serenity_libc(LibCrypt crypt)
target_link_libraries(LibCrypt LibC LibCrypto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
serenity_libc_shared(LibCryptShared crypt)
target_link_libraries(LibCryptShared LibCShared LibCryptoShared)
