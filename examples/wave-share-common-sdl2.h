#pragma once

#include <SDL2/SDL.h>

class WaveShare;

bool initSDL2ForWaveShare(
        bool & isInitialized,
        const int playbackId,
        SDL_AudioDeviceID & devIdIn,
        const int captureId,
        SDL_AudioDeviceID & devIdOut,
        WaveShare *& waveShare,
        const char * defaultCaptureDeviceName = nullptr);
