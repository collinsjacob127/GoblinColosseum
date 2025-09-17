# list(APPEND HEADER_FILES engine.cpp engine.hpp )

add_library(characters "")

target_sources(characters
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/hunko.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/hunko.hpp
        ${CMAKE_CURRENT_LIST_DIR}/characters.hpp
    )

target_include_directories(characters
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    )

# Allow render to access SDL
target_link_libraries(characters 
PRIVATE
    libs
    util
    engine
)
