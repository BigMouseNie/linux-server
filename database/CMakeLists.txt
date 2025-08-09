find_package(SQLite3 REQUIRED)

add_library(database STATIC
    src/table.cpp
    src/field.cpp
    src/sqlite3_db.cpp
)

target_include_directories(database
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(database PRIVATE
    ${SQLite3_LIBRARIES}
)
