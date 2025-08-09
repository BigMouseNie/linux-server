add_library(business STATIC
    src/echo_biz.cpp
)

target_include_directories(business PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(business PRIVATE
    logger pool http network run
)
