cmake_minimum_required (VERSION 3.0)
project (wave-share)

set(CMAKE_EXPORT_COMPILE_COMMANDS "on")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(WAVESHARE_STANDALONE ON)
    include(cmake/GitVars.cmake)
    include(cmake/BuildTypes.cmake)
else()
    set(WAVESHARE_STANDALONE OFF)
endif()

if (EMSCRIPTEN)
    set(WAVESHARE_SUPPORT_SDL2_DEFAULT ON)
    set(WAVESHARE_LIBRARY_TYPE STATIC)
else()
    set(WAVESHARE_SUPPORT_SDL2_DEFAULT ON)
    if (BUILD_SHARED_LIBS)
        set(WAVESHARE_LIBRARY_TYPE SHARED)
    else ()
        set(WAVESHARE_LIBRARY_TYPE STATIC)
    endif()
endif()

# options

option(USE_FINDSDL2                      "wave-share: use the FindSDL2.cmake script" OFF)

option(WAVESHARE_ALL_WARNINGS            "wave-share: enable all compiler warnings" ON)
option(WAVESHARE_ALL_WARNINGS_3RD_PARTY  "wave-share: enable all compiler warnings in 3rd party libs" ON)

option(WAVESHARE_SANITIZE_THREAD         "wave-share: enable thread sanitizer" OFF)
option(WAVESHARE_SANITIZE_ADDRESS        "wave-share: enable address sanitizer" OFF)
option(WAVESHARE_SANITIZE_UNDEFINED      "wave-share: enable undefined sanitizer" OFF)

option(WAVESHARE_SUPPORT_SDL2            "wave-share: support for libSDL2" ${WAVESHARE_SUPPORT_SDL2_DEFAULT})

option(WAVESHARE_BUILD_EXAMPLES          "wave-share: build examples" ${WAVESHARE_STANDALONE})

# sanitizers

if (WAVESHARE_SANITIZE_THREAD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif()

if (WAVESHARE_SANITIZE_ADDRESS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
endif()

if (WAVESHARE_SANITIZE_UNDEFINED)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()

# dependencies

# main

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WAVESHARE_ALL_WARNINGS)
    if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    else()
        # todo : windows
    endif()
endif()

add_subdirectory(src)

if (WAVESHARE_STANDALONE AND WAVESHARE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

#add_executable(wave-share main.cpp wave-share.cpp)
#target_include_directories(wave-share PUBLIC ${SDL2_INCLUDE_DIRS})
#target_link_libraries(wave-share PUBLIC ${CMAKE_THREAD_LIBS_INIT} ${SDL2_LIBRARIES})
