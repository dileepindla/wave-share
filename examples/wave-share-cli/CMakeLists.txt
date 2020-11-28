add_executable(wave-share-cli main.cpp)

target_include_directories(wave-share-cli PRIVATE
    ..
    ${SDL2_INCLUDE_DIRS}
    )

target_link_libraries(wave-share-cli PRIVATE
    wave-share
    wave-share-common
    wave-share-common-sdl2
    ${CMAKE_THREAD_LIBS_INIT}
    )
