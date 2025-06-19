#include "audio_manager.h"
#include <math.h>

AudioManager audioManager;

AudioManager::AudioManager()
{
    initialized = false;
    playing = false;
    recording = false;
    currentVolume = 50; // Default 50% volume

    // Initialize buffers
    audioInputBuffer = nullptr;
    audioOutputBuffer = nullptr;
    fftBuffer = nullptr;
    currentLevel = 0.0f;

    // Initialize frequency bands
    for (int i = 0; i < NUM_BANDS; i++)
    {
        frequencyBands[i] = 0.0f;
    }
}

bool AudioManager::init()
{
    Serial0.println("Initializing I2S Audio System...");

    // Allocate audio buffers
    audioInputBuffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
    audioOutputBuffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
    fftBuffer = (float *)malloc(FFT_SIZE * sizeof(float));

    if (!audioInputBuffer || !audioOutputBuffer || !fftBuffer)
    {
        Serial0.println("Failed to allocate audio buffers");
        return false;
    }

    // Initially configure for output (speaker)
    if (!configureI2SForOutput())
    {
        Serial0.println("Failed to configure I2S for output");
        return false;
    }

    initialized = true;
    Serial0.println("I2S Audio System initialized successfully!");
    return true;
}

bool AudioManager::configureI2SForOutput()
{
    Serial0.println("Configuring I2S for audio output...");

    // Uninstall existing driver if present (ignore errors if not installed)
    esp_err_t uninstall_err = i2s_driver_uninstall(I2S_NUM_0);
    if (uninstall_err != ESP_OK && uninstall_err != ESP_ERR_INVALID_STATE) {
        Serial0.printf("Warning: I2S uninstall returned: %s\n", esp_err_to_name(uninstall_err));
    }

    // Configure I2S for output (MAX98357A)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = OUTPUT_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = true, // Use APLL for better audio quality
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0};

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK)
    {
        Serial0.printf("Failed to install I2S driver for output: %s\n", esp_err_to_name(err));
        return false;
    }

    // Set I2S pins for output
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_SD_OUT_PIN, // GPIO21 for MAX98357A
        .data_in_num = I2S_PIN_NO_CHANGE};

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK)
    {
        Serial0.printf("Failed to set I2S pins for output: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial0.println("I2S configured for audio output (MAX98357A)");
    return true;
}

bool AudioManager::configureI2SForInput()
{
    Serial0.println("Configuring I2S for audio input...");

    // Uninstall existing driver (ignore errors if not installed)
    esp_err_t uninstall_err = i2s_driver_uninstall(I2S_NUM_0);
    if (uninstall_err != ESP_OK && uninstall_err != ESP_ERR_INVALID_STATE) {
        Serial0.printf("Warning: I2S uninstall returned: %s\n", esp_err_to_name(uninstall_err));
    }

    // Configure I2S for input (INMP441)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = INPUT_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = true, // Use APLL for stable clock
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK)
    {
        Serial0.printf("Failed to install I2S driver for input: %s\n", esp_err_to_name(err));
        return false;
    }

    // Set I2S pins for input
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_IN_PIN // GPIO13 for INMP441
    };

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK)
    {
        Serial0.printf("Failed to set I2S pins for input: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial0.println("I2S configured for audio input (INMP441)");
    return true;
}

void AudioManager::playBeep(uint16_t frequency, uint16_t duration)
{
    if (!initialized)
    {
        Serial0.println("Audio not initialized!");
        return;
    }

    bool wasRecording = recording;

    // If recording, temporarily switch to output mode
    if (recording)
    {
        stopRecording();
    }

    if (playing)
    {
        Serial0.println("Already playing audio");
        if (wasRecording)
            startRecording(); // Restore recording mode
        return;
    }

    Serial0.printf("Playing beep: %dHz for %dms\n", frequency, duration);

    // Ensure we're in output mode
    configureI2SForOutput();

    generateTone(frequency, duration);

    // Restore recording mode if it was active
    if (wasRecording)
    {
        startRecording();
    }
}

void AudioManager::generateTone(uint16_t frequency, uint16_t duration)
{
    playing = true;

    const int samples = (OUTPUT_SAMPLE_RATE * duration) / 1000;
    const float amplitude = (32767.0 * currentVolume) / 100.0; // Scale by volume

    int16_t *audio_buffer = (int16_t *)malloc(samples * sizeof(int16_t));
    if (!audio_buffer)
    {
        Serial0.println("Failed to allocate audio buffer");
        playing = false;
        return;
    }

    // Generate sine wave
    for (int i = 0; i < samples; i++)
    {
        float t = (float)i / OUTPUT_SAMPLE_RATE;
        float sample = amplitude * sin(2.0 * PI * frequency * t);

        // Apply envelope to avoid clicks (fade in/out)
        float envelope = 1.0;
        int fade_samples = OUTPUT_SAMPLE_RATE / 100; // 10ms fade

        if (i < fade_samples)
        {
            envelope = (float)i / fade_samples;
        }
        else if (i > samples - fade_samples)
        {
            envelope = (float)(samples - i) / fade_samples;
        }

        audio_buffer[i] = (int16_t)(sample * envelope);
    }

    // Write to I2S
    size_t bytes_written;
    esp_err_t err = i2s_write(I2S_NUM_0, audio_buffer, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);

    if (err != ESP_OK)
    {
        Serial0.printf("I2S write failed: %s\n", esp_err_to_name(err));
    }
    else
    {
        Serial0.printf("Wrote %d bytes to I2S\n", bytes_written);
    }

    free(audio_buffer);
    playing = false;
}

void AudioManager::setVolume(uint8_t volume)
{
    currentVolume = constrain(volume, 0, 100);
    Serial0.printf("Volume set to %d%%\n", currentVolume);
}

bool AudioManager::isPlaying()
{
    return playing;
}

void AudioManager::stop()
{
    if (playing)
    {
        i2s_zero_dma_buffer(I2S_NUM_0);
        playing = false;
        Serial0.println("Audio stopped");
    }
}

// Add these missing method implementations:
bool AudioManager::startRecording()
{
    if (!initialized)
    {
        Serial0.println("Audio not initialized!");
        return false;
    }

    if (recording)
    {
        Serial0.println("Already recording");
        return true;
    }

    // Configure I2S for input (microphone)
    configureI2SForInput();

    // Clear any existing data in DMA buffers
    i2s_zero_dma_buffer(I2S_NUM_0);

    recording = true;
    Serial0.println("Started audio recording");
    return true;
}

void AudioManager::stopRecording()
{
    if (!recording)
    {
        return;
    }

    recording = false;

    // Reconfigure I2S back to output mode
    configureI2SForOutput();

    Serial0.println("Stopped audio recording");
}

bool AudioManager::isRecording()
{
    return recording;
}

float AudioManager::getAudioLevel()
{
    if (!recording)
    {
        return 0.0;
    }

    // Read audio data and calculate RMS level
    int16_t audio_buffer[64];
    size_t bytes_read;

    esp_err_t err = i2s_read(I2S_NUM_0, audio_buffer, sizeof(audio_buffer), &bytes_read, 10);
    if (err != ESP_OK || bytes_read == 0)
    {
        return 0.0;
    }

    // Calculate RMS
    float sum = 0.0;
    int samples = bytes_read / sizeof(int16_t);

    for (int i = 0; i < samples; i++)
    {
        float sample = (float)audio_buffer[i] / 32768.0;
        sum += sample * sample;
    }

    return sqrt(sum / samples);
}

void AudioManager::processAudioData()
{
    if (!recording)
    {
        return;
    }

    // Read audio data for processing
    int32_t samples[BUFFER_SIZE];
    size_t bytes_read;

    esp_err_t err = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, 10);
    if (err != ESP_OK || bytes_read == 0)
    {
        return;
    }

    int num_samples = bytes_read / sizeof(int32_t);

    // Convert 32-bit samples to 16-bit and calculate RMS level
    float sum = 0.0;
    for (int i = 0; i < num_samples && i < BUFFER_SIZE; i++)
    {
        // Convert from 32-bit to 16-bit (INMP441 outputs 24-bit data in 32-bit format)
        int16_t sample = (samples[i] >> 11) & 0xFFFF;
        audioInputBuffer[i] = sample;

        float normalized = (float)sample / 32768.0f;
        sum += normalized * normalized;
    }

    // Update current audio level (RMS)
    currentLevel = sqrt(sum / num_samples);

    // Perform FFT for frequency analysis
    if (num_samples >= FFT_SIZE)
    {
        performFFT(audioInputBuffer, fftBuffer);
        updateFrequencyBands(fftBuffer);
    }
}

void AudioManager::performFFT(int16_t *samples, float *output)
{
    // Simple FFT implementation for frequency analysis
    // This is a simplified version - you might want to use a proper FFT library
    for (int i = 0; i < FFT_SIZE; i++)
    {
        if (i < BUFFER_SIZE)
        {
            output[i] = (float)samples[i] / 32768.0f;
        }
        else
        {
            output[i] = 0.0f;
        }
    }
}

void AudioManager::updateFrequencyBands(float *fftData)
{
    // Simple frequency band calculation
    int samples_per_band = FFT_SIZE / NUM_BANDS;

    for (int band = 0; band < NUM_BANDS; band++)
    {
        float sum = 0.0f;
        int start = band * samples_per_band;
        int end = start + samples_per_band;

        for (int i = start; i < end && i < FFT_SIZE; i++)
        {
            sum += fabs(fftData[i]);
        }

        frequencyBands[band] = sum / samples_per_band;
    }
}

void AudioManager::getFrequencyBands(float *bands, int numBands)
{
    int copyBands = min(numBands, NUM_BANDS);
    for (int i = 0; i < copyBands; i++)
    {
        bands[i] = frequencyBands[i];
    }

    // Fill remaining bands with zeros if needed
    for (int i = copyBands; i < numBands; i++)
    {
        bands[i] = 0.0f;
    }
}

void AudioManager::playBeep()
{
    playBeep(BEEP_FREQUENCY, BEEP_DURATION);
}
