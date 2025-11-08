# list(APPEND HEADER_FILES net.cpp net.hpp )

add_library(net "")

target_sources(net
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/packets.cpp
        ${CMAKE_CURRENT_LIST_DIR}/net.cpp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/packets.hpp
        ${CMAKE_CURRENT_LIST_DIR}/net.hpp
    )

target_include_directories(net
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    )

# Allow render to access SDL
target_link_libraries(net 
PRIVATE
    libs
    buttons
    engine
    util
)

IF (WIN32)
target_link_libraries(net 
PRIVATE
  ws2_32
)
ENDIF()
