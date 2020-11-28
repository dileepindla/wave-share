find_package(Threads REQUIRED)

add_library(wave-share-common ${WAVESHARE_LIBRARY_TYPE}
    wave-share-common.cpp
    )

target_link_libraries(wave-share-common PRIVATE
    )

if (WAVESHARE_SUPPORT_SDL2)
    # SDL2

    find_package(SDL2)

    if (NOT USE_FINDSDL2 AND NOT SDL2_FOUND)
        message(WARNING "Unable to find SDL2 library. It is either not installed or CMake cannot find it."
            " In the latter case, setting the USE_FINDSDL2 variable might help:\n"
            "   $ cmake -D USE_FINDSDL2 .."
            )

        message(FATAL_ERROR "Aborting")
    endif()

    string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

    # wave-share-common-sdl2

    add_library(wave-share-common-sdl2 ${WAVESHARE_LIBRARY_TYPE}
        wave-share-common-sdl2.cpp
        )

    target_include_directories(wave-share-common-sdl2 PUBLIC
        ${SDL2_INCLUDE_DIRS}
        )

    target_link_libraries(wave-share-common-sdl2 PRIVATE
        wave-share
        ${SDL2_LIBRARIES}
        )
endif()

if (WAVESHARE_SUPPORT_SDL2)
    # sdl2 examples

    add_subdirectory(wave-share-cli)
endif()

if (EMSCRIPTEN)
    # emscripten examples

    return()
endif()
