/*! \file main.cpp
 *  \brief Send/Receive data through sound in the terminal
 *  \author Georgi Gerganov
 */

#include "wave-share/wave-share.h"

#include "wave-share-common-sdl2.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include <cstdio>
#include <string>
#include <chrono>
#include <ctime>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifdef __EMSCRIPTEN__
#include "build_timestamp.h"
#include "emscripten/emscripten.h"
#else
#include <mutex>
#include <thread>
#include <iostream>
#endif

#ifdef main
#undef main
#endif

static char *g_defaultCaptureDeviceName = nullptr;

static int g_captureId = -1;
static int g_playbackId = -1;

static bool g_isInitialized = false;

static SDL_AudioDeviceID g_devIdIn = 0;
static SDL_AudioDeviceID g_devIdOut = 0;

static WaveShare *g_waveShare = nullptr;

namespace {

template <class T>
float getTime_ms(const T & tStart, const T & tEnd) {
    return ((float)(std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count()))/1000.0;
}

}

// JS interface
extern "C" {
    int setText(int textLength, const char * text) {
        g_waveShare->init(textLength, text);
        return 0;
    }

    int getText(char * text) {
        std::copy(g_waveShare->getRxData().begin(), g_waveShare->getRxData().end(), text);
        return 0;
    }

    int getSampleRate()             { return g_waveShare->getSampleRateIn(); }
    float getAverageRxTime_ms()     { return g_waveShare->getAverageRxTime_ms(); }
    int getFramesToRecord()         { return g_waveShare->getFramesToRecord(); }
    int getFramesLeftToRecord()     { return g_waveShare->getFramesLeftToRecord(); }
    int getFramesToAnalyze()        { return g_waveShare->getFramesToAnalyze(); }
    int getFramesLeftToAnalyze()    { return g_waveShare->getFramesLeftToAnalyze(); }
    int hasDeviceOutput()           { return g_devIdOut; }
    int hasDeviceCapture()          { return (g_waveShare->getTotalBytesCaptured() > 0) ? g_devIdIn : 0; }

    int doInit()                    {
        return initSDL2ForWaveShare(
                g_isInitialized,
                g_playbackId,
                g_devIdIn,
                g_captureId,
                g_devIdOut,
                g_waveShare,
                g_defaultCaptureDeviceName);
    }

    int setTxMode(int txMode) {
        g_waveShare->setTxMode((WaveShare::TxMode)(txMode));
        g_waveShare->init(0, "");
        return 0;
    }

    void setParameters(
        int paramFreqDelta,
        int paramFreqStart,
        int paramFramesPerTx,
        int paramBytesPerTx,
        int /*paramECCBytesPerTx*/,
        int paramVolume) {
        if (g_waveShare == nullptr) return;

        g_waveShare->setParameters(
                paramFreqDelta,
                paramFreqStart,
                paramFramesPerTx,
                paramBytesPerTx,
                paramVolume);

        g_waveShare->init(0, "");
    }
}

// main loop
void update() {
    if (g_isInitialized == false) return;

    SDL_Event e;
    SDL_bool shouldTerminate = SDL_FALSE;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            shouldTerminate = SDL_TRUE;
        }
    }

    static WaveShare::CBQueueAudio cbQueueAudio = [&](const void * data, uint32_t nBytes) {
        SDL_QueueAudio(g_devIdOut, data, nBytes);
    };

    static WaveShare::CBDequeueAudio CBDequeueAudio = [&](void * data, uint32_t nMaxBytes) {
        return SDL_DequeueAudio(g_devIdIn, data, nMaxBytes);
    };

    if (g_waveShare->getHasData() == false) {
        SDL_PauseAudioDevice(g_devIdOut, SDL_FALSE);

        static auto tLastNoData = std::chrono::high_resolution_clock::now();
        auto tNow = std::chrono::high_resolution_clock::now();

        if ((int) SDL_GetQueuedAudioSize(g_devIdOut) < g_waveShare->getSamplesPerFrame()*g_waveShare->getSampleSizeBytesOut()) {
            SDL_PauseAudioDevice(g_devIdIn, SDL_FALSE);
            if (::getTime_ms(tLastNoData, tNow) > 500.0f) {
                g_waveShare->receive(CBDequeueAudio);
                if ((int) SDL_GetQueuedAudioSize(g_devIdIn) > 32*g_waveShare->getSamplesPerFrame()*g_waveShare->getSampleSizeBytesIn()) {
                    SDL_ClearQueuedAudio(g_devIdIn);
                }
            } else {
                SDL_ClearQueuedAudio(g_devIdIn);
            }
        } else {
            tLastNoData = tNow;
            //SDL_ClearQueuedAudio(g_devIdIn);
            //SDL_Delay(10);
        }
    } else {
        SDL_PauseAudioDevice(g_devIdOut, SDL_TRUE);
        SDL_PauseAudioDevice(g_devIdIn, SDL_TRUE);

        g_waveShare->send(cbQueueAudio);
    }

    if (shouldTerminate) {
        SDL_PauseAudioDevice(g_devIdIn, 1);
        SDL_CloseAudioDevice(g_devIdIn);
        SDL_PauseAudioDevice(g_devIdOut, 1);
        SDL_CloseAudioDevice(g_devIdOut);
        SDL_CloseAudio();
        SDL_Quit();
        #ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
        #endif
    }
}

static std::map<std::string, std::string> parseCmdArguments(int argc, char ** argv) {
    int last = argc;
    std::map<std::string, std::string> res;
    for (int i = 1; i < last; ++i) {
        if (argv[i][0] == '-') {
            if (strlen(argv[i]) > 1) {
                res[std::string(1, argv[i][1])] = strlen(argv[i]) > 2 ? argv[i] + 2 : "";
            }
        }
    }

    return res;
}

int main(int argc, char** argv) {
#ifdef __EMSCRIPTEN__
    printf("Build time: %s\n", BUILD_TIMESTAMP);
    printf("Press the Init button to start\n");

    g_defaultCaptureDeviceName = argv[1];
#else
    printf("Usage: %s [-cN] [-pN] [-tN]\n", argv[0]);
    printf("    -cN - select capture device N\n");
    printf("    -pN - select playback device N\n");
    printf("    -tN - transmission protocol:\n");
    printf("          -t0 : Normal\n");
    printf("          -t1 : Fast (default)\n");
    printf("          -t2 : Fastest\n");
    printf("          -t3 : Ultrasonic\n");
    printf("\n");

    g_defaultCaptureDeviceName = nullptr;

    auto argm = parseCmdArguments(argc, argv);
    g_captureId = argm["c"].empty() ? 0 : std::stoi(argm["c"]);
    g_playbackId = argm["p"].empty() ? 0 : std::stoi(argm["p"]);
    int txProtocol = argm["t"].empty() ? 1 : std::stoi(argm["t"]);
#endif

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(update, 60, 1);
#else
    initSDL2ForWaveShare(
            g_isInitialized,
            g_playbackId,
            g_devIdIn,
            g_captureId,
            g_devIdOut,
            g_waveShare);
    setTxMode(1);

    printf("Selecting Tx protocol %d\n", txProtocol);
    switch (txProtocol) {
        case 0:
            {
                printf("Using 'Normal' Tx Protocol\n");
                g_waveShare->setParameters(1, 40, 9, 3, 50);
            }
            break;
        case 1:
            {
                printf("Using 'Fast' Tx Protocol\n");
                g_waveShare->setParameters(1, 40, 6, 3, 50);
            }
            break;
        case 2:
            {
                printf("Using 'Fastest' Tx Protocol\n");
                g_waveShare->setParameters(1, 40, 3, 3, 50);
            }
            break;
        case 3:
            {
                printf("Using 'Ultrasonic' Tx Protocol\n");
                g_waveShare->setParameters(1, 320, 9, 3, 50);
            }
            break;
        default:
            {
                printf("Using 'Fast' Tx Protocol\n");
                g_waveShare->setParameters(1, 40, 6, 3, 50);
            }
    };
    g_waveShare->init(0, "");
    printf("\n");

    std::mutex mutex;
    std::thread inputThread([&mutex]() {
        std::string inputOld = "";
        while (true) {
            std::string input;
            std::cout << "Enter text: ";
            getline(std::cin, input);
            if (input.empty()) {
                std::cout << "Re-sending ... " << std::endl;
                input = inputOld;
            } else {
                std::cout << "Sending ... " << std::endl;
            }
            {
                std::lock_guard<std::mutex> lock(mutex);
                setText(input.size(), input.data());
            }
            inputOld = input;
        }
    });

    while (true) {
        SDL_Delay(1);
        {
            std::lock_guard<std::mutex> lock(mutex);
            update();
        }
    }

    inputThread.join();
#endif

    delete g_waveShare;

    SDL_PauseAudioDevice(g_devIdIn, 1);
    SDL_CloseAudioDevice(g_devIdIn);
    SDL_PauseAudioDevice(g_devIdOut, 1);
    SDL_CloseAudioDevice(g_devIdOut);
    SDL_CloseAudio();
    SDL_Quit();

    return 0;
}
