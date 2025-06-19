#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>

// Audio visualization constants
#define NUM_BANDS 8 // Number of frequency bands for visualization
#define FFT_SIZE 256
#define BUFFER_SIZE 1024

// I2S Configuration - Shared bus for INMP441 and MAX98357A
#define I2S_WS_PIN 15  // GPIO15 -> LRCLK (Word Select) - shared
#define I2S_SCK_PIN 14 // GPIO14 -> BCLK (Bit Clock) - shared

// Separate data pins for input and output
#define I2S_SD_IN_PIN 13  // GPIO13 -> INMP441 microphone data input
#define I2S_SD_OUT_PIN 21 // GPIO21 -> MAX98357A amplifier data output

// Audio settings for output (speaker)
#define OUTPUT_SAMPLE_RATE 44100 // CD quality for output
#define OUTPUT_BITS_PER_SAMPLE 16
#define OUTPUT_CHANNELS 1 // Mono

// Audio settings for input (microphone) - optimized for voice
#define INPUT_SAMPLE_RATE 16000  // Better for voice, reduces noise
#define INPUT_BITS_PER_SAMPLE 32 // Better signal-to-noise ratio
#define INPUT_CHANNELS 1         // Mono

// Beep parameters
#define BEEP_FREQUENCY 1000 // 1kHz tone
#define BEEP_DURATION 500   // 500ms

class AudioManager
{
public:
    AudioManager();
    bool init();
    void playBeep();
    void playBeep(uint16_t frequency, uint16_t duration);
    void setVolume(uint8_t volume); // 0-100
    bool isPlaying();
    void stop();

    // Microphone functions
    bool startRecording();
    void stopRecording();
    bool isRecording();
    float getAudioLevel();
    void getFrequencyBands(float *bands, int numBands);
    void processAudioData();

private:
    bool initialized;
    bool playing;
    bool recording;
    uint8_t currentVolume;

    // Audio buffers
    int16_t *audioInputBuffer;
    int16_t *audioOutputBuffer;
    float *fftBuffer;
    float frequencyBands[NUM_BANDS];
    float currentLevel;

    void generateTone(uint16_t frequency, uint16_t duration);
    bool configureI2SForOutput();
    bool configureI2SForInput();
    void performFFT(int16_t *samples, float *output);
    void updateFrequencyBands(float *fftData);
};

extern AudioManager audioManager;

#endif