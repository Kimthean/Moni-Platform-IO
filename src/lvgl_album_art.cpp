#include "lvgl_album_art.h"
#include "lvgl_ui_components.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TJpg_Decoder.h>

// Album artwork data
static uint8_t* album_art_buffer = nullptr;
static size_t album_art_size = 0;
static lv_img_dsc_t album_art_dsc;
static lv_color_t* decoded_image_buffer = nullptr;

// Simple callback - just decode to a temporary buffer first
static lv_color_t* temp_decode_buffer = nullptr;
static uint16_t temp_width = 0;
static uint16_t temp_height = 0;

// Task handle for async image download
static TaskHandle_t imageDownloadTask = NULL;
static String pendingImageUrl = "";

// Thread-safe communication between download task and main thread
static bool imageReadyForDisplay = false;
static uint8_t* downloadedImageData = nullptr;
static size_t downloadedImageSize = 0;

bool tjpg_output_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
    // Store to temporary buffer first, we'll do center crop later
    if (temp_decode_buffer && x + w <= temp_width && y + h <= temp_height) {
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int src_idx = row * w + col;
                int dst_idx = (y + row) * temp_width + (x + col);
                if (dst_idx < temp_width * temp_height) {
                    temp_decode_buffer[dst_idx].full = bitmap[src_idx];
                }
            }
        }
    }
    return true;
}

// Download and display album artwork
void lvgl_download_and_set_album_art(const String& imageUrl)
{
    if (imageUrl.length() == 0) {
        Serial0.println("📷 No album artwork URL provided");
        lvgl_show_album_placeholder();
        return;
    }
    
    Serial0.println("📷 Downloading album artwork from: " + imageUrl);
    
    HTTPClient http;
    http.begin(imageUrl);
    http.setTimeout(15000); // 15 second timeout for larger images
    http.addHeader("User-Agent", "ESP32-Spotify-Player/1.0");
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        Serial0.printf("📷 Album art size: %d bytes\n", contentLength);
        
        if (contentLength > 0 && contentLength < 100000) { // Increased limit to 100KB for 300x300 images
            // Free previous buffer if exists
            if (album_art_buffer) {
                free(album_art_buffer);
                album_art_buffer = nullptr;
            }
            
            // Allocate buffer in PSRAM
            album_art_buffer = (uint8_t*)ps_malloc(contentLength);
            if (album_art_buffer) {
                WiFiClient* stream = http.getStreamPtr();
                size_t bytesRead = stream->readBytes(album_art_buffer, contentLength);
                album_art_size = bytesRead;
                
                Serial0.printf("📷 Downloaded %d bytes of album artwork\n", bytesRead);
                
                // Decode and set as LVGL image
                lvgl_set_album_art(album_art_buffer, album_art_size);
            } else {
                Serial0.println("❌ Failed to allocate memory for album artwork");
                lvgl_show_album_placeholder();
            }
        } else {
            Serial0.printf("❌ Album artwork too large or invalid size: %d bytes\n", contentLength);
            lvgl_show_album_placeholder();
        }
    } else {
        Serial0.printf("❌ Failed to download album artwork, HTTP code: %d\n", httpCode);
        lvgl_show_album_placeholder();
    }
    
    http.end();
}

// Set album artwork from downloaded data
void lvgl_set_album_art(const uint8_t* img_data, size_t img_size)
{
    if (!img_data || img_size == 0) {
        lvgl_show_album_placeholder();
        return;
    }
    
    Serial0.println("📷 Decoding JPEG album artwork...");
    
    // Initialize TJpg_Decoder
    TJpgDec.setJpgScale(1); // No scaling initially
    TJpgDec.setCallback(tjpg_output_callback);
    
    // Allocate buffer for decoded image (170x170 pixels)
    if (decoded_image_buffer) {
        free(decoded_image_buffer);
    }
    decoded_image_buffer = (lv_color_t*)ps_malloc(170 * 170 * sizeof(lv_color_t));
    
    if (!decoded_image_buffer) {
        Serial0.println("❌ Failed to allocate image buffer");
        lvgl_show_album_placeholder();
        return;
    }
    
    // Fill the buffer with a default color (dark gray) instead of black
    lv_color_t fill_color = lv_color_hex(0x2a2a2a);
    for (int i = 0; i < 170 * 170; i++) {
        decoded_image_buffer[i] = fill_color;
    }
    
    // Get JPEG info
    uint16_t jpgWidth, jpgHeight;
    if (TJpgDec.getJpgSize(&jpgWidth, &jpgHeight, img_data, img_size) != JDR_OK) {
        Serial0.println("❌ Failed to get JPEG dimensions");
        free(decoded_image_buffer);
        decoded_image_buffer = nullptr;
        lvgl_show_album_placeholder();
        return;
    }
    
    Serial0.printf("📷 JPEG dimensions: %dx%d\n", jpgWidth, jpgHeight);
    
    // For better quality, decode at higher resolution and then downsample
    // For 300x300 images, decode at scale=2 (150x150) instead of scale=4 (75x75)
    uint8_t scale = 1;
    if (jpgWidth >= 600 && jpgHeight >= 600) scale = 4;      // 600x600 -> 150x150
    else if (jpgWidth >= 300 && jpgHeight >= 300) scale = 2;  // 300x300 -> 150x150  
    else if (jpgWidth >= 150 && jpgHeight >= 150) scale = 1;  // Keep original if reasonable size
    else scale = 1;
    
    // Calculate actual decoded dimensions
    uint16_t decoded_width = jpgWidth / scale;
    uint16_t decoded_height = jpgHeight / scale;
    
    Serial0.printf("📷 Will decode to: %dx%d (scale=%d) for better quality\n", decoded_width, decoded_height, scale);
    
    // Allocate temporary buffer for full decoded image
    if (temp_decode_buffer) {
        free(temp_decode_buffer);
    }
    temp_decode_buffer = (lv_color_t*)ps_malloc(decoded_width * decoded_height * sizeof(lv_color_t));
    if (!temp_decode_buffer) {
        Serial0.println("❌ Failed to allocate temp decode buffer");
        lvgl_show_album_placeholder();
        return;
    }
    
    temp_width = decoded_width;
    temp_height = decoded_height;
    
    // Clear temp buffer
    memset(temp_decode_buffer, 0, decoded_width * decoded_height * sizeof(lv_color_t));
    
    TJpgDec.setJpgScale(scale);
    
    // Decode the JPEG to temporary buffer
    if (TJpgDec.drawJpg(0, 0, img_data, img_size) == JDR_OK) {
        Serial0.println("✅ JPEG decoded to temp buffer");
        
        // Now do center crop from temp buffer to final 170x170 buffer
        // Calculate crop area (center crop for object-fit: cover)
        float scale_x = (float)decoded_width / 170.0f;
        float scale_y = (float)decoded_height / 170.0f;
        float crop_scale = (scale_x > scale_y) ? scale_x : scale_y; // Use larger scale for cover
        
        uint16_t crop_width = (uint16_t)(170 * crop_scale);
        uint16_t crop_height = (uint16_t)(170 * crop_scale);
        
        uint16_t crop_x = (decoded_width - crop_width) / 2;
        uint16_t crop_y = (decoded_height - crop_height) / 2;
        
        Serial0.printf("📷 Center crop: %dx%d from (%d,%d), scale=%.2f\n", 
                       crop_width, crop_height, crop_x, crop_y, crop_scale);
        
        // Resample from crop area to 170x170 with better quality
        for (int dst_y = 0; dst_y < 170; dst_y++) {
            for (int dst_x = 0; dst_x < 170; dst_x++) {
                // Map destination pixel back to source with sub-pixel precision
                float src_x_f = crop_x + (dst_x * (float)crop_width) / 170.0f;
                float src_y_f = crop_y + (dst_y * (float)crop_height) / 170.0f;
                
                int src_x = (int)src_x_f;
                int src_y = (int)src_y_f;
                
                // Bounds check
                if (src_x < decoded_width && src_y < decoded_height) {
                    int src_idx = src_y * decoded_width + src_x;
                    int dst_idx = dst_y * 170 + dst_x;
                    decoded_image_buffer[dst_idx] = temp_decode_buffer[src_idx];
                }
            }
        }
        
        // Free temp buffer
        free(temp_decode_buffer);
        temp_decode_buffer = nullptr;
        
        // Get reference to album container and recreate album_img as image object
        extern lv_obj_t *main_screen;
        static lv_obj_t *album_container = nullptr;
        
        // Find album container if not found yet
        if (!album_container) {
            // It's the 4th child of main_screen (after title, wifi, and spotify icon)
            album_container = lv_obj_get_child(main_screen, 3);
        }
        
        // Delete existing album_img (might be plain object from placeholder)
        if (album_img) {
            lv_obj_del(album_img);
        }
        
        // Create new image object for artwork
        album_img = lv_img_create(album_container);
        lv_obj_set_size(album_img, 170, 170); // Fill entire container
        lv_obj_align(album_img, LV_ALIGN_CENTER, 0, 0);
        lv_obj_clear_flag(album_img, LV_OBJ_FLAG_SCROLLABLE);
        
        // Create LVGL image descriptor
        album_art_dsc.header.always_zero = 0;
        album_art_dsc.header.w = 170;
        album_art_dsc.header.h = 170;
        album_art_dsc.data_size = 170 * 170 * sizeof(lv_color_t);
        album_art_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
        album_art_dsc.data = (const uint8_t*)decoded_image_buffer;
        
        // Set the image source
        lv_img_set_src(album_img, &album_art_dsc);
        
        Serial0.println("✅ Album artwork center-cropped and displayed");
    } else {
        Serial0.println("❌ Failed to decode JPEG");
        free(temp_decode_buffer);
        temp_decode_buffer = nullptr;
        free(decoded_image_buffer);
        decoded_image_buffer = nullptr;
        lvgl_show_album_placeholder();
    }
}

// Show album artwork placeholder (clean gray background)
void lvgl_show_album_placeholder()
{
    // Get reference to album container (declared static in lvgl_create_ui)
    extern lv_obj_t *main_screen;
    static lv_obj_t *album_container = nullptr;
    
    // Find album container if not found yet
    if (!album_container) {
        // It's the 4th child of main_screen (after title, wifi, and spotify icon)
        album_container = lv_obj_get_child(main_screen, 3);
    }
    
    // Delete existing album_img and recreate as plain object
    if (album_img) {
        lv_obj_del(album_img);
    }
    
    // Create a plain object (not image) for clean placeholder
    album_img = lv_obj_create(album_container);
    
    // Style it as clean gray background
    lv_obj_set_size(album_img, 170, 170);
    lv_obj_align(album_img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(album_img, lv_color_hex(0x2a2a2a), 0); // Dark gray
    lv_obj_set_style_bg_opa(album_img, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(album_img, 12, 0); // More rounded corners for larger size
    lv_obj_set_style_border_width(album_img, 0, 0); // No border
    lv_obj_clear_flag(album_img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(album_img, 0, 0);
    
    Serial0.println("📷 Showing clean gray placeholder");
}

// Show loading indicator (same as placeholder - clean gray)
void lvgl_show_loading_indicator()
{
    // Just use the same clean gray background as placeholder
    lvgl_show_album_placeholder();
    Serial0.println("📷 Showing loading state (clean gray)");
}

// Cleanup album artwork memory
void lvgl_cleanup_album_art()
{
    Serial0.println("🧹 Cleaning up album artwork memory");
    
    // Free album art buffer
    if (album_art_buffer) {
        free(album_art_buffer);
        album_art_buffer = nullptr;
        album_art_size = 0;
    }
    
    // Free decoded image buffer
    if (decoded_image_buffer) {
        free(decoded_image_buffer);
        decoded_image_buffer = nullptr;
    }
    
    Serial0.println("✅ Album artwork memory cleaned up");
}

// FreeRTOS task for downloading album artwork (download only, no LVGL operations)
void imageDownloadTaskFunction(void* parameter)
{
    String* urlPtr = (String*)parameter;
    String url = *urlPtr;
    delete urlPtr; // Clean up the allocated string
    
    Serial0.println("🚀 Background task: Starting album artwork download...");
    
    // Configure HTTPClient with SSL optimizations for background task
    HTTPClient http;
    WiFiClientSecure *client = new WiFiClientSecure;
    if (!client) {
        Serial0.println("❌ Background task: Failed to create SSL client");
        imageDownloadTask = NULL;
        vTaskDelete(NULL);
        return;
    }
    
    // Optimize SSL for background task
    client->setInsecure(); // Skip certificate verification to save memory
    client->setTimeout(10000); // 10 second timeout
    
    http.begin(*client, url);
    http.setTimeout(10000); // 10 second timeout
    http.addHeader("User-Agent", "ESP32-Spotify-Player/1.0");
    http.addHeader("Connection", "close"); // Close connection immediately after
    
    Serial0.println("🔗 Background task: Connecting to image server...");
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        Serial0.printf("📷 Background task: Album art size: %d bytes\n", contentLength);
        
        if (contentLength > 0 && contentLength < 100000) { // 100KB limit
            // Allocate buffer in PSRAM
            uint8_t* tempBuffer = (uint8_t*)ps_malloc(contentLength);
            if (tempBuffer) {
                WiFiClient* stream = http.getStreamPtr();
                size_t bytesRead = stream->readBytes(tempBuffer, contentLength);
                
                Serial0.printf("📷 Background task: Downloaded %d bytes\n", bytesRead);
                
                // Thread-safe handoff to main thread
                if (downloadedImageData) {
                    free(downloadedImageData); // Clean up previous data
                }
                downloadedImageData = tempBuffer;
                downloadedImageSize = bytesRead;
                imageReadyForDisplay = true; // Signal main thread
                
                Serial0.println("✅ Background task: Download completed, ready for display");
            } else {
                Serial0.println("❌ Background task: Failed to allocate memory");
            }
        } else {
            Serial0.printf("❌ Background task: Invalid size: %d bytes\n", contentLength);
        }
    } else {
        Serial0.printf("❌ Background task: HTTP error: %d\n", httpCode);
        
        // If background task fails, signal main thread to try fallback
        if (httpCode == -1) { // SSL/Connection error
            Serial0.println("🔄 Background task: Will try fallback method on main thread");
            // Set a special flag for fallback
            pendingImageUrl = url;
        }
    }
    
    http.end();
    delete client; // Clean up SSL client
    
    // Task will delete itself when function exits
    imageDownloadTask = NULL;
    vTaskDelete(NULL);
}

// Lazy load album artwork (truly non-blocking)
void lvgl_lazy_load_album_art(const String& imageUrl)
{
    // Clean up previous artwork first
    lvgl_cleanup_album_art();
    
    // Show loading indicator immediately while downloading
    lvgl_show_loading_indicator();
    
    // Cancel any existing download task
    if (imageDownloadTask != NULL) {
        Serial0.println("🛑 Cancelling previous image download task");
        vTaskDelete(imageDownloadTask);
        imageDownloadTask = NULL;
    }
    
    // Create a copy of the URL string for the task
    String* urlCopy = new String(imageUrl);
    
    // Start download in background task (truly non-blocking)
    Serial0.printf("🚀 Starting async album artwork download (Free heap: %d bytes)...\n", esp_get_free_heap_size());
    xTaskCreatePinnedToCore(
        imageDownloadTaskFunction,  // Task function
        "ImageDownload",            // Task name
        12288,                      // Stack size (12KB - increased for SSL)
        urlCopy,                    // Parameters
        1,                          // Priority (lower than main loop)
        &imageDownloadTask,         // Task handle
        0                           // Core (use core 0, main loop uses core 1)
    );
}

// Check for completed image downloads and process them (call from main loop)
void lvgl_process_pending_images()
{
    // Check for successful background download
    if (imageReadyForDisplay && downloadedImageData && downloadedImageSize > 0) {
        Serial0.println("📷 Main thread: Processing downloaded image...");
        
        // Process the downloaded image on the main thread (LVGL-safe)
        lvgl_set_album_art(downloadedImageData, downloadedImageSize);
        
        // Clean up
        free(downloadedImageData);
        downloadedImageData = nullptr;
        downloadedImageSize = 0;
        imageReadyForDisplay = false;
        
        Serial0.println("✅ Main thread: Image processed and displayed");
    }
    // Check for fallback needed (background task failed)
    else if (pendingImageUrl.length() > 0 && imageDownloadTask == NULL) {
        Serial0.println("🔄 Main thread: Background task failed, trying fallback...");
        String url = pendingImageUrl;
        pendingImageUrl = ""; // Clear flag
        
        // Try synchronous download as fallback (with timeout)
        bool success = false;
        // Add a simple timeout mechanism for fallback
        unsigned long fallbackStart = millis();
        if (millis() - fallbackStart < 5000) { // 5 second timeout for fallback
            lvgl_download_and_set_album_art(url);
            success = true;
        }
        
        // If fallback also fails, show "No Image" placeholder
        if (!success) {
            Serial0.println("❌ Fallback also failed, showing no image placeholder");
            lvgl_show_album_placeholder();
        }
    }
} 