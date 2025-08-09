find_package(OpenSSL REQUIRED)

add_library(encryption STATIC
    src/crypto_wrap.cpp
)

target_include_directories(encryption PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(encryption PRIVATE OpenSSL::Crypto)
