find_package(Threads REQUIRED)

add_library(wave-share-common ${WAVESHARE_LIBRARY_TYPE}
    wave-share-common.cpp
    )

target_link_libraries(wave-share-common PRIVATE
    )

if (WAVESHARE_SUPPORT_SDL2)
    # SDL2

    if (EMSCRIPTEN)
        set (CMAKE_CXX_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s USE_SDL=2 -s ASSERTIONS=1 -s DISABLE_EXCEPTION_CATCHING=0 -s 'EXTRA_EXPORTED_RUNTIME_METHODS=[\"writeArrayToMemory\"]'")
    endif()

    add_library(wave-share-common-sdl2 ${WAVESHARE_LIBRARY_TYPE}
        wave-share-common-sdl2.cpp
        )

    if (NOT EMSCRIPTEN)
        find_package(SDL2)

        if (NOT USE_FINDSDL2 AND NOT SDL2_FOUND)
            message(WARNING "Unable to find SDL2 library. It is either not installed or CMake cannot find it."
                " In the latter case, setting the USE_FINDSDL2 variable might help:\n"
                "   $ cmake -D USE_FINDSDL2 .."
                )

            message(FATAL_ERROR "Aborting")
        endif()

        string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
    endif()

    # wave-share-common-sdl2

    target_include_directories(wave-share-common-sdl2 PUBLIC
        ${SDL2_INCLUDE_DIRS}
        )

    target_link_libraries(wave-share-common-sdl2 PRIVATE
        wave-share
        ${SDL2_LIBRARIES}
        )
endif()

if (WAVESHARE_SUPPORT_SDL2)
    if (EMSCRIPTEN)
        # emscripten sdl2 examples

        add_subdirectory(wave-share-wasm)
    else()
        # non-emscripten sdl2 examples

        add_subdirectory(wave-share-cli)
    endif()
endif()
