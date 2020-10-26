set(SOURCES
    Deflate.cpp
    Zlib.cpp
    Gzip.cpp
)

serenity_lib(LibCompress compression)
target_link_libraries(LibCompress LibC LibCrypto)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
#serenity_shared_lib(LibCompressShared core)
#target_link_libraries(LibCompressShared LibCShared LibCryptoShared)
