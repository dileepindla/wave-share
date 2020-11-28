# core

add_library(wave-share ${WAVESHARE_LIBRARY_TYPE}
    wave-share.cpp
    )

target_include_directories(wave-share PUBLIC
    .
    ../include
    )

target_link_libraries(wave-share PUBLIC
    )

target_link_libraries(wave-share PRIVATE
    ${CMAKE_DL_LIBS}
    )
