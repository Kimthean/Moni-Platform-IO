#include "lvgl_track_info.h"
#include "lvgl_ui_components.h"
#include "lvgl_album_art.h"
#include <Arduino.h>

// Update track information
void lvgl_update_track_info(const SpotifyTrack& track, bool trackValid)
{
    static String lastImageUrl = "";
    
    if (!trackValid) {
        lv_label_set_text(track_label, "No track playing");
        lv_label_set_text(artist_label, "Start playing on Spotify");
        lv_label_set_text(album_label, "");
        lv_label_set_text(status_label, "Stopped");
        
        // Show placeholder when no track
        lvgl_show_album_placeholder();
        lastImageUrl = "";
        return;
    }

    // Update track info
    lv_label_set_text(track_label, track.name.c_str());
    lv_label_set_text(artist_label, track.artist.c_str());
    lv_label_set_text(album_label, track.album.c_str());
    
    // Update status with enhanced playback information
    String statusText = "";
    if (track.isPlaying) {
        statusText = "Playing";
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x1db954), 0);
    } else {
        statusText = "Paused";
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xffa500), 0);
    }
    
    // Add shuffle/repeat indicators
    if (track.shuffleState) {
        statusText += " 🔀";
    }
    if (track.repeatState == "track") {
        statusText += " 🔂";
    } else if (track.repeatState == "context") {
        statusText += " 🔁";
    }
    
    // Add device info if available
    if (track.deviceName.length() > 0) {
        statusText += " • " + track.deviceName;
        if (track.deviceVolume > 0) {
            statusText += " (" + String(track.deviceVolume) + "%)";
        }
    }
    
    lv_label_set_text(status_label, statusText.c_str());
    
    // Add progress information to Serial output for debugging
    if (track.duration_ms > 0 && track.progress_ms >= 0) {
        int progressPercent = (track.progress_ms * 100) / track.duration_ms;
        int progressMinutes = track.progress_ms / 60000;
        int progressSeconds = (track.progress_ms % 60000) / 1000;
        int totalMinutes = track.duration_ms / 60000;
        int totalSeconds = (track.duration_ms % 60000) / 1000;
        
        Serial0.printf("⏱️ Progress: %d:%02d / %d:%02d (%d%%)\n", 
                      progressMinutes, progressSeconds, 
                      totalMinutes, totalSeconds, 
                      progressPercent);
    }
    
    // Handle album artwork changes with lazy loading
    if (track.imageUrl != lastImageUrl) {
        if (track.imageUrl.length() > 0) {
            Serial0.println("🎨 New album artwork detected, lazy loading...");
            lastImageUrl = track.imageUrl;
            // Start lazy loading in background (non-blocking)
            lvgl_lazy_load_album_art(track.imageUrl);
        } else {
            // No artwork available, clean up and show placeholder
            lastImageUrl = "";
            lvgl_cleanup_album_art();
            lvgl_show_album_placeholder();
        }
    }
} 