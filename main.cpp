/*! \file main.cpp
 *  \brief Send/Receive data through sound
 *  \author Georgi Gerganov
 */

#include "wave-share.h"

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

constexpr double kBaseSampleRate = 48000.0;

static char *g_captureDeviceName = nullptr;

static int g_captureId = -1;
static int g_playbackId = -1;

static bool g_isInitialized = false;

static SDL_AudioDeviceID g_devIdIn = 0;
static SDL_AudioDeviceID g_devIdOut = 0;

static WaveShare *g_data = nullptr;

namespace {

template <class T>
float getTime_ms(const T & tStart, const T & tEnd) {
    return ((float)(std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count()))/1000.0;
}

}

int init() {
    if (g_isInitialized) return 0;

    printf("Initializing ...\n");

    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium", SDL_HINT_OVERRIDE);

    {
        int nDevices = SDL_GetNumAudioDevices(SDL_FALSE);
        printf("Found %d playback devices:\n", nDevices);
        for (int i = 0; i < nDevices; i++) {
            printf("    - Playback device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_FALSE));
        }
    }
    {
        int nDevices = SDL_GetNumAudioDevices(SDL_TRUE);
        printf("Found %d capture devices:\n", nDevices);
        for (int i = 0; i < nDevices; i++) {
            printf("    - Capture device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
        }
    }

    SDL_AudioSpec playbackSpec;
    SDL_zero(playbackSpec);

    playbackSpec.freq = ::kBaseSampleRate;
    playbackSpec.format = AUDIO_S16SYS;
    playbackSpec.channels = 1;
    playbackSpec.samples = 16*1024;
    playbackSpec.callback = NULL;

    SDL_AudioSpec obtainedSpecIn;
    SDL_AudioSpec obtainedSpecOut;

    int sampleSizeBytesIn = 4;
    int sampleSizeBytesOut = 2;

    SDL_zero(obtainedSpecIn);
    SDL_zero(obtainedSpecOut);

    if (g_playbackId >= 0) {
        printf("Attempt to open playback device %d : '%s' ...\n", g_playbackId, SDL_GetAudioDeviceName(g_playbackId, SDL_FALSE));
        g_devIdOut = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(g_playbackId, SDL_FALSE), SDL_FALSE, &playbackSpec, &obtainedSpecOut, 0);
    } else {
        printf("Attempt to open default playback device ...\n");
        g_devIdOut = SDL_OpenAudioDevice(NULL, SDL_FALSE, &playbackSpec, &obtainedSpecOut, 0);
    }

    if (!g_devIdOut) {
        printf("Couldn't open an audio device for playback: %s!\n", SDL_GetError());
        g_devIdOut = 0;
    } else {
        printf("Obtained spec for output device (SDL Id = %d):\n", g_devIdOut);
        printf("    - Sample rate:       %d (required: %d)\n", obtainedSpecOut.freq, playbackSpec.freq);
        printf("    - Format:            %d (required: %d)\n", obtainedSpecOut.format, playbackSpec.format);
        printf("    - Channels:          %d (required: %d)\n", obtainedSpecOut.channels, playbackSpec.channels);
        printf("    - Samples per frame: %d (required: %d)\n", obtainedSpecOut.samples, playbackSpec.samples);

        if (obtainedSpecOut.format != playbackSpec.format ||
            obtainedSpecOut.channels != playbackSpec.channels ||
            obtainedSpecOut.samples != playbackSpec.samples) {
            SDL_CloseAudio();
            throw std::runtime_error("Failed to initialize playback SDL_OpenAudio!");
        }
    }

    switch (obtainedSpecOut.format) {
        case AUDIO_U8:
        case AUDIO_S8:
            sampleSizeBytesOut = 1;
            break;
        case AUDIO_U16SYS:
        case AUDIO_S16SYS:
            sampleSizeBytesOut = 2;
            break;
        case AUDIO_S32SYS:
        case AUDIO_F32SYS:
            sampleSizeBytesOut = 4;
            break;
    }

    SDL_AudioSpec captureSpec;
    captureSpec = obtainedSpecOut;
    captureSpec.freq = ::kBaseSampleRate;
    captureSpec.format = AUDIO_F32SYS;
    captureSpec.samples = 4096;

    if (g_captureId >= 0) {
        printf("Attempt to open capture device %d : '%s' ...\n", g_captureId, SDL_GetAudioDeviceName(g_captureId, SDL_FALSE));
        g_devIdIn = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(g_captureId, SDL_TRUE), SDL_TRUE, &captureSpec, &obtainedSpecIn, 0);
    } else {
        printf("Attempt to open default capture device ...\n");
        g_devIdIn = SDL_OpenAudioDevice(g_captureDeviceName, SDL_TRUE, &captureSpec, &obtainedSpecIn, 0);
    }
    if (!g_devIdIn) {
        printf("Couldn't open an audio device for capture: %s!\n", SDL_GetError());
        g_devIdIn = 0;
    } else {
        printf("Obtained spec for input device (SDL Id = %d):\n", g_devIdIn);
        printf("    - Sample rate:       %d\n", obtainedSpecIn.freq);
        printf("    - Format:            %d (required: %d)\n", obtainedSpecIn.format, captureSpec.format);
        printf("    - Channels:          %d (required: %d)\n", obtainedSpecIn.channels, captureSpec.channels);
        printf("    - Samples per frame: %d\n", obtainedSpecIn.samples);
    }

    switch (obtainedSpecIn.format) {
        case AUDIO_U8:
        case AUDIO_S8:
            sampleSizeBytesIn = 1;
            break;
        case AUDIO_U16SYS:
        case AUDIO_S16SYS:
            sampleSizeBytesIn = 2;
            break;
        case AUDIO_S32SYS:
        case AUDIO_F32SYS:
            sampleSizeBytesIn = 4;
            break;
    }

    g_data = new WaveShare(
            obtainedSpecIn.freq,
            obtainedSpecOut.freq,
            1024,
            sampleSizeBytesIn,
            sampleSizeBytesOut);

    g_isInitialized = true;
    return 0;
}

// JS interface
extern "C" {
    int setText(int textLength, const char * text) {
        g_data->init(textLength, text);
        return 0;
    }

    int getText(char * text) {
        std::copy(g_data->getRxData().begin(), g_data->getRxData().end(), text);
        return 0;
    }

    int getSampleRate()             { return g_data->getSampleRateIn(); }
    float getAverageRxTime_ms()     { return g_data->getAverageRxTime_ms(); }
    int getFramesToRecord()         { return g_data->getFramesToRecord(); }
    int getFramesLeftToRecord()     { return g_data->getFramesLeftToRecord(); }
    int getFramesToAnalyze()        { return g_data->getFramesToAnalyze(); }
    int getFramesLeftToAnalyze()    { return g_data->getFramesLeftToAnalyze(); }
    int hasDeviceOutput()           { return g_devIdOut; }
    int hasDeviceCapture()          { return (g_data->getTotalBytesCaptured() > 0) ? g_devIdIn : 0; }
    int doInit()                    { return init(); }
    int setTxMode(int txMode) {
        g_data->setTxMode((WaveShare::TxMode)(txMode));
        g_data->init(0, "");
        return 0;
    }

    void setParameters(
        int paramFreqDelta,
        int paramFreqStart,
        int paramFramesPerTx,
        int paramBytesPerTx,
        int /*paramECCBytesPerTx*/,
        int paramVolume) {
        if (g_data == nullptr) return;

        g_data->setParameters(
                paramFreqDelta,
                paramFreqStart,
                paramFramesPerTx,
                paramBytesPerTx,
                paramVolume);

        g_data->init(0, "");
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

    if (g_data->getHasData() == false) {
        SDL_PauseAudioDevice(g_devIdOut, SDL_FALSE);

        static auto tLastNoData = std::chrono::high_resolution_clock::now();
        auto tNow = std::chrono::high_resolution_clock::now();

        if ((int) SDL_GetQueuedAudioSize(g_devIdOut) < g_data->getSamplesPerFrame()*g_data->getSampleSizeBytesOut()) {
            SDL_PauseAudioDevice(g_devIdIn, SDL_FALSE);
            if (::getTime_ms(tLastNoData, tNow) > 500.0f) {
                g_data->receive(CBDequeueAudio);
                if ((int) SDL_GetQueuedAudioSize(g_devIdIn) > 32*g_data->getSamplesPerFrame()*g_data->getSampleSizeBytesIn()) {
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

        g_data->send(cbQueueAudio);
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

    g_captureDeviceName = argv[1];
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

    g_captureDeviceName = nullptr;

    auto argm = parseCmdArguments(argc, argv);
    g_captureId = argm["c"].empty() ? 0 : std::stoi(argm["c"]);
    g_playbackId = argm["p"].empty() ? 0 : std::stoi(argm["p"]);
    int txProtocol = argm["t"].empty() ? 1 : std::stoi(argm["t"]);
#endif

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(update, 60, 1);
#else
    init();
    setTxMode(1);
    printf("Selecting Tx protocol %d\n", txProtocol);
    switch (txProtocol) {
        case 0:
            {
                printf("Using 'Normal' Tx Protocol\n");
                g_data->setParameters(1, 40, 9, 3, 50);
            }
            break;
        case 1:
            {
                printf("Using 'Fast' Tx Protocol\n");
                g_data->setParameters(1, 40, 6, 3, 50);
            }
            break;
        case 2:
            {
                printf("Using 'Fastest' Tx Protocol\n");
                g_data->setParameters(1, 40, 3, 3, 50);
            }
            break;
        case 3:
            {
                printf("Using 'Ultrasonic' Tx Protocol\n");
                g_data->setParameters(1, 320, 9, 3, 50);
            }
            break;
        default:
            {
                printf("Using 'Fast' Tx Protocol\n");
                g_data->setParameters(1, 40, 6, 3, 50);
            }
    };
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

    delete g_data;

    SDL_PauseAudioDevice(g_devIdIn, 1);
    SDL_CloseAudioDevice(g_devIdIn);
    SDL_PauseAudioDevice(g_devIdOut, 1);
    SDL_CloseAudioDevice(g_devIdOut);
    SDL_CloseAudio();
    SDL_Quit();

    return 0;
}
