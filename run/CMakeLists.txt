add_library(run STATIC
    src/process_wrap.cpp
    src/daemonize.cpp
)

target_include_directories(run PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
