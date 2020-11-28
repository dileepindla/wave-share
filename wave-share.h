#pragma once

#include <array>
#include <complex>
#include <cstdint>
#include <functional>

enum TxMode {
    FixedLength = 0,
    VariableLength,
};

constexpr double    kBaseSampleRate = 48000.0;
constexpr auto      kMaxSamplesPerFrame = 1024;
constexpr auto      kMaxDataBits = 256;
constexpr auto      kMaxDataSize = 256;
constexpr auto      kMaxLength = 140;
constexpr auto      kMaxSpectrumHistory = 4;
constexpr auto      kMaxRecordedFrames = 64*10;
constexpr auto      kDefaultFixedLength = 82;

using AmplitudeData   = std::array<float, kMaxSamplesPerFrame>;
using AmplitudeData16 = std::array<int16_t, kMaxRecordedFrames*kMaxSamplesPerFrame>;
using SpectrumData    = std::array<float, kMaxSamplesPerFrame>;
using RecordedData    = std::array<float, kMaxRecordedFrames*kMaxSamplesPerFrame>;

namespace RS {
class ReedSolomon;
}

struct DataRxTx {
    using CBQueueAudio = std::function<void(const void * data, uint32_t nBytes)>;
    using CBDequeueAudio = std::function<uint32_t(void * data, uint32_t nMaxBytes)>;

    DataRxTx(
            int aSampleRateIn,
            int aSampleRateOut,
            int aSamplesPerFrame,
            int aSampleSizeBytesIn,
            int aSampleSizeBytesOut);

    void init(int textLength, const char * stext);

    void send(const CBQueueAudio & cbQueueAudio);
    void receive(const CBDequeueAudio & CBDequeueAudio);

    int nIterations;
    bool needUpdate = false;

    int paramFreqDelta = 6;
    int paramFreqStart = 40;
    int paramFramesPerTx = 6;
    int paramBytesPerTx = 2;
    int paramECCBytesPerTx = 32;
    int paramVolume = 10;

    // Rx
    bool receivingData;
    bool analyzingData;

    int recvDuration_frames;
    int totalBytesCaptured;

    float averageRxTime_ms = 0.0;

    std::array<float, kMaxSamplesPerFrame> fftIn;
    std::array<std::complex<float>, kMaxSamplesPerFrame> fftOut;

    ::AmplitudeData sampleAmplitude;
    ::SpectrumData sampleSpectrum;

    std::array<std::uint8_t, ::kMaxDataSize> rxData;
    std::array<std::uint8_t, ::kMaxDataSize> encodedData;

    int historyId = 0;
    ::AmplitudeData sampleAmplitudeAverage;
    std::array<::AmplitudeData, ::kMaxSpectrumHistory> sampleAmplitudeHistory;

    ::RecordedData recordedAmplitude;

    // Tx
    bool hasData;

    float freqDelta_hz;
    float freqStart_hz;
    float hzPerFrame;
    float ihzPerFrame;
    float isamplesPerFrame;
    float sampleRateIn;
    float sampleRateOut;
    float sendVolume;

    int dataId;
    int frameId;
    int framesLeftToAnalyze;
    int framesLeftToRecord;
    int framesPerTx;
    int framesToAnalyze;
    int framesToRecord;
    int freqDelta_bin = 1;
    int nBitsInMarker;
    int nDataBitsPerTx;
    int nECCBytesPerTx;
    int nMarkerFrames;
    int nPostMarkerFrames;
    int nRampFrames;
    int nRampFramesBegin;
    int nRampFramesBlend;
    int nRampFramesEnd;
    int sampleSizeBytesIn;
    int sampleSizeBytesOut;
    int samplesPerFrame;
    int sendDataLength;

    std::string textToSend;

    ::TxMode txMode = ::TxMode::FixedLength;

    ::AmplitudeData outputBlock;
    ::AmplitudeData16 outputBlock16;

    std::array<bool, ::kMaxDataBits> dataBits;
    std::array<double, ::kMaxDataBits> phaseOffsets;
    std::array<double, ::kMaxDataBits> dataFreqs_hz;

    std::array<::AmplitudeData, ::kMaxDataBits> bit1Amplitude;
    std::array<::AmplitudeData, ::kMaxDataBits> bit0Amplitude;

    RS::ReedSolomon * rsData = nullptr;
    RS::ReedSolomon * rsLength = nullptr;
};
