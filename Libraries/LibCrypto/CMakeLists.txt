set(SOURCES
    BigInt/SignedBigInteger.cpp
    BigInt/UnsignedBigInteger.cpp
    Checksum/Adler32.cpp
    Checksum/CRC32.cpp
    Cipher/AES.cpp
    Hash/MD5.cpp
    Hash/SHA1.cpp
    Hash/SHA2.cpp
    NumberTheory/ModularFunctions.cpp
    PK/RSA.cpp
)

serenity_lib(LibCrypto crypto)
target_link_libraries(LibCrypto LibC)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_WITHOUT_STATIC} -fpic")
#serenity_shared_lib(LibCryptoShared crypto)
#target_link_libraries(LibCryptoShared LibCShared)

