# list(APPEND HEADER_FILES engine.cpp engine.hpp )

add_library(engine "")

target_sources(engine
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/engine.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/engine.hpp
    )

target_include_directories(engine
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    )

# Allow render to access SDL
target_link_libraries(engine 
PRIVATE
    libs
)
