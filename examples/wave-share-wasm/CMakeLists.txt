set(TARGET wave-share-wasm)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build_timestamp.h.tmpl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/build_timestamp.h @ONLY)

add_executable(${TARGET}
    main.cpp
    )

target_include_directories(${TARGET} PRIVATE
    ..
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/
    )

target_link_libraries(${TARGET} PRIVATE
    wave-share
    wave-share-common
    wave-share-common-sdl2
    )
