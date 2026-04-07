add_library(logger STATIC
    src/logger.cpp
)

# 0 = BlockingQueue, 1 = MPSCQueue
set(LOGGER_USE_MPSC_QUEUE 0)

if(LOGGER_USE_MPSC_QUEUE)
    target_compile_definitions(logger PUBLIC LOGGER_USE_MPSC_QUEUE)
endif()

target_include_directories(logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(logger PUBLIC
    container
)
