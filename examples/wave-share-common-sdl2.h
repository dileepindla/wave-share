#pragma once

#include "wave-share/wave-share.h"

#include <SDL2/SDL.h>

bool initSDL2ForWaveShare(
        bool & isInitialized,
        const int playbackId,
        SDL_AudioDeviceID & devIdIn,
        const int captureId,
        SDL_AudioDeviceID & devIdOut,
        WaveShare *& waveShare,
        const char * defaultCaptureDeviceName = nullptr);
