include(FetchContent)

FetchContent_Declare(
  http_parser
  GIT_REPOSITORY https://github.com/nodejs/http-parser.git
  GIT_TAG        v2.9.4
)
FetchContent_MakeAvailable(http_parser)

FetchContent_Declare(
  uriparser
  GIT_REPOSITORY https://github.com/uriparser/uriparser.git
  GIT_TAG        uriparser-0.9.8
)
set(URIPARSER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(URIPARSER_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(uriparser)

add_library(http STATIC
    ${http_parser_SOURCE_DIR}/http_parser.c
    src/http_parser_wrap.cpp
)

target_include_directories(http PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${http_parser_SOURCE_DIR}
    ${uriparser_SOURCE_DIR}/include/uriparser
)

target_link_libraries(http PRIVATE
    uriparser
)
