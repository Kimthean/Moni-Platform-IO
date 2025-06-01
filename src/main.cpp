#include <Arduino.h>
#include <WiFi.h>
#include <esp_smartconfig.h>
#include <nvs_flash.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#include "wifi_manager.h"
#include "time_manager.h"
#include "weather.h"
#include "config.h"

// TFT Display
TFT_eSPI tft = TFT_eSPI();

// LVGL display buffer - Updated for LVGL 8.x with proper alignment
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[240 * 10] __attribute__((aligned(4))); // Add alignment
static lv_disp_drv_t disp_drv;

// LVGL objects
lv_obj_t *main_screen;
lv_obj_t *status_label;
lv_obj_t *wifi_label;
lv_obj_t *time_label;
lv_obj_t *date_label;
lv_obj_t *temp_label;
lv_obj_t *weather_label;
lv_obj_t *humidity_label;
lv_obj_t *wind_label;

// Manager objects
WiFiManager wifiManager;
TimeManager timeManager;
WeatherManager weatherManager;

// Timing
unsigned long lastStateCheck = 0;
unsigned long lastDataOutput = 0;
unsigned long lastDisplayUpdate = 0;

enum SystemState
{
  STATE_CONNECTING,
  STATE_NORMAL,
  STATE_ERROR
};

SystemState currentState = STATE_CONNECTING;

// LVGL display flush callback - Updated for LVGL 8.x
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)color_p, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

// Initialize LVGL - Updated for LVGL 8.x
void lvgl_init()
{
  lv_init();

  // Verify TFT dimensions before buffer initialization
  if (tft.width() == 0 || tft.height() == 0) {
    Serial0.println("ERROR: Invalid TFT dimensions");
    return;
  }
  
  Serial0.printf("TFT dimensions: %dx%d\n", tft.width(), tft.height());
  Serial0.printf("Buffer size: %d pixels\n", sizeof(buf) / sizeof(lv_color_t));

  // Initialize display buffer
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, sizeof(buf) / sizeof(lv_color_t));

  // Initialize display driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = tft.width();
  disp_drv.ver_res = tft.height();
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
}

// Create UI elements
void create_ui()
{
  // Create main screen
  main_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(main_screen, lv_color_black(), 0);
  lv_scr_load(main_screen);

  // Status label
  status_label = lv_label_create(main_screen);
  lv_obj_set_pos(status_label, 10, 10);
  lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, 0);
  lv_label_set_text(status_label, "CONNECTING...");

  // WiFi status label
  wifi_label = lv_label_create(main_screen);
  lv_obj_set_pos(wifi_label, 10, 40);
  lv_obj_set_style_text_color(wifi_label, lv_color_white(), 0);
  lv_label_set_text(wifi_label, "WiFi: Disconnected");

  // Time label
  time_label = lv_label_create(main_screen);
  lv_obj_set_pos(time_label, 10, 70);
  lv_obj_set_style_text_color(time_label, lv_color_make(0, 255, 255), 0); // Cyan
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
  lv_label_set_text(time_label, "--:--:--..");

  // Date label
  date_label = lv_label_create(main_screen);
  lv_obj_set_pos(date_label, 10, 110);
  lv_obj_set_style_text_color(date_label, lv_color_make(0, 255, 255), 0); // Cyan
  lv_obj_set_style_text_font(date_label, &lv_font_montserrat_16, 0);
  lv_label_set_text(date_label, "--/--/----");

  // Temperature label
  temp_label = lv_label_create(main_screen);
  lv_obj_set_pos(temp_label, 10, 150);
  lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_20, 0);
  lv_label_set_text(temp_label, "--°C");

  // Weather condition label
  weather_label = lv_label_create(main_screen);
  lv_obj_set_pos(weather_label, 10, 180);
  lv_obj_set_style_text_color(weather_label, lv_color_white(), 0);
  lv_label_set_text(weather_label, "Loading...");

  // Humidity label
  humidity_label = lv_label_create(main_screen);
  lv_obj_set_pos(humidity_label, 10, 200);
  lv_obj_set_style_text_color(humidity_label, lv_color_white(), 0);
  lv_label_set_text(humidity_label, "Humidity: --%");

  // Wind label
  wind_label = lv_label_create(main_screen);
  lv_obj_set_pos(wind_label, 10, 220);
  lv_obj_set_style_text_color(wind_label, lv_color_white(), 0);
  lv_label_set_text(wind_label, "Wind: -- m/s");
}

// Update UI with current data
void update_ui()
{
  TimeData timeData = timeManager.getTimeData();
  WeatherData weatherData = weatherManager.getWeatherData();

  // Update status
  switch (currentState)
  {
  case STATE_CONNECTING:
    lv_obj_set_style_text_color(status_label, lv_color_make(255, 255, 0), 0); // Yellow
    lv_label_set_text(status_label, "CONNECTING...");
    break;
  case STATE_NORMAL:
    lv_obj_set_style_text_color(status_label, lv_color_make(0, 255, 0), 0); // Green
    lv_label_set_text(status_label, "ONLINE");
    break;
  case STATE_ERROR:
    lv_obj_set_style_text_color(status_label, lv_color_make(255, 0, 0), 0); // Red
    lv_label_set_text(status_label, "ERROR");
    break;
  }

  // Update WiFi status
  if (wifiManager.isConnected())
  {
    lv_obj_set_style_text_color(wifi_label, lv_color_make(0, 255, 0), 0); // Green
    lv_label_set_text(wifi_label, "WiFi: Connected");
  }
  else
  {
    lv_obj_set_style_text_color(wifi_label, lv_color_make(255, 0, 0), 0); // Red
    lv_label_set_text(wifi_label, "WiFi: Disconnected");
  }

  // Update time
  if (timeData.timeValid)
  {
    lv_label_set_text_fmt(time_label, "%02d:%02d:%02d", timeData.hour, timeData.minute, timeData.second);
    lv_label_set_text_fmt(date_label, "%02d/%02d/%04d", timeData.day, timeData.month, timeData.year);
  }
  else
  {
    lv_label_set_text(time_label, "Time: N/A");
    lv_label_set_text(date_label, "Date: N/A");
  }

  // Update weather
  if (weatherData.dataValid)
  {
    lv_label_set_text_fmt(temp_label, "%.1f°C", weatherData.temperature);
    lv_label_set_text(weather_label, weatherData.condition.c_str());
    lv_label_set_text_fmt(humidity_label, "Humidity: %.1f%%", weatherData.humidity);
    lv_label_set_text_fmt(wind_label, "Wind: %.1f m/s", weatherData.windSpeed);
  }
  else
  {
    lv_label_set_text(temp_label, "--°C");
    lv_label_set_text(weather_label, "Weather: N/A");
    lv_label_set_text(humidity_label, "Humidity: --%");
    lv_label_set_text(wind_label, "Wind: -- m/s");
  }
}

SystemState checkDataStatus()
{
  bool wifiConnected = wifiManager.isConnected();
  TimeData timeData = timeManager.getTimeData();
  WeatherData weatherData = weatherManager.getWeatherData();

  Serial0.print("Status check - WiFi: ");
  Serial0.print(wifiConnected ? "Connected" : "Disconnected");
  Serial0.print(" | Time valid: ");
  Serial0.print(timeData.timeValid ? "Yes" : "No");
  Serial0.print(" | Weather valid: ");
  Serial0.println(weatherData.dataValid ? "Yes" : "No");

  if (!wifiConnected)
  {
    return STATE_CONNECTING;
  }

  if (!timeData.timeValid || !weatherData.dataValid)
  {
    if (millis() > 60000)
    { // After 1 minute, show error
      return STATE_ERROR;
    }
    return STATE_CONNECTING;
  }

  return STATE_NORMAL;
}

void tryToRecover()
{
  static unsigned long lastRecovery = 0;
  unsigned long now = millis();

  if (now - lastRecovery < 5000)
  {
    return; // Try recovery every 5 seconds
  }
  lastRecovery = now;

  Serial0.println("Attempting recovery...");

  if (!wifiManager.isConnected())
  {
    wifiManager.update();
  }

  TimeData timeData = timeManager.getTimeData();
  if (wifiManager.isConnected() && !timeData.timeValid)
  {
    timeManager.update();
  }

  WeatherData weatherData = weatherManager.getWeatherData();
  if (wifiManager.isConnected() && !weatherData.dataValid)
  {
    weatherManager.update();
  }
}

void outputCurrentData()
{
  TimeData timeData = timeManager.getTimeData();
  WeatherData weatherData = weatherManager.getWeatherData();

  Serial0.println("=== Current Data ===");

  if (timeData.timeValid)
  {
    Serial0.printf("Time: %02d:%02d:%02d\n", timeData.hour, timeData.minute, timeData.second);
    Serial0.printf("Date: %02d/%02d/%04d\n", timeData.day, timeData.month, timeData.year);
    Serial0.printf("Weekday: %d\n", timeData.weekday);
  }
  else
  {
    Serial0.println("Time: Not available");
  }

  if (weatherData.dataValid)
  {
    Serial0.print("Weather: ");
    Serial0.println(weatherData.condition);
    Serial0.printf("Temperature: %.1f°C\n", weatherData.temperature);
    Serial0.printf("Humidity: %.1f%%\n", weatherData.humidity);
    Serial0.printf("Wind: %.1f m/s\n", weatherData.windSpeed);
  }
  else
  {
    Serial0.println("Weather: Not available");
  }

  Serial0.printf("WiFi: %s\n", wifiManager.isConnected() ? "Connected" : "Disconnected");
  Serial0.println("===================");
}

void setup()
{
  Serial0.begin(115200);
  delay(1000);
  Serial0.println("Starting Weather Station with BGR Display...");

  // Initialize NVS FIRST - before any other components
  Serial0.println("Initializing NVS...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    Serial0.println("NVS partition was truncated, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  if (ret != ESP_OK)
  {
    Serial0.printf("NVS initialization failed: %s\n", esp_err_to_name(ret));
    // Try to continue anyway
  }
  else
  {
    Serial0.println("NVS initialized successfully");
  }

  delay(2000);

  // Initialize TFT Display for BGR with error checking
  Serial0.println("Initializing TFT display...");

  tft.init();

  // Verify TFT is working
  if (tft.width() == 0 || tft.height() == 0)
  {
    Serial0.println("ERROR: TFT initialization failed - invalid dimensions");
    while (1)
    {
      delay(1000);
    } // Halt execution
  }

  Serial0.printf("TFT initialized: %dx%d\n", tft.width(), tft.height());

  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(TFT_BLACK);

  // Test BGR colors
  Serial0.println("Testing BGR display colors...");
  tft.fillRect(0, 0, 50, 50, TFT_RED);    // Should appear red
  tft.fillRect(50, 0, 50, 50, TFT_GREEN); // Should appear green
  tft.fillRect(100, 0, 50, 50, TFT_BLUE); // Should appear blue
  delay(2000);
  tft.fillScreen(TFT_BLACK);

  Serial0.println("TFT BGR Display initialized");

  // Initialize LVGL
  Serial0.println("Initializing LVGL...");
  lvgl_init();
  Serial0.println("LVGL initialized");

  // Create UI
  Serial0.println("Creating UI...");
  create_ui();
  Serial0.println("UI created");

  // Initialize managers AFTER NVS is ready
  Serial0.println("Initializing WiFi manager...");
  if (!wifiManager.hasStoredCredentials())
  {
    wifiManager.startSmartConfig();
  }

  Serial0.println("Initializing time manager...");
  timeManager.init();

  Serial0.println("Initializing weather manager...");
  weatherManager.init();

  // Set to connecting state after initialization
  currentState = STATE_CONNECTING;
  Serial0.println("Setup complete! State set to CONNECTING");
}

void loop()
{
  // Handle LVGL tasks - Updated for LVGL 8.x
  lv_timer_handler();

  // Update managers
  wifiManager.update();
  timeManager.update();
  weatherManager.update();

  // Check state every 1000ms
  unsigned long now = millis();
  if (now - lastStateCheck > 1000)
  {
    lastStateCheck = now;

    SystemState newState = checkDataStatus();

    if (newState != currentState)
    {
      Serial0.print("State changing from ");
      Serial0.print(currentState);
      Serial0.print(" to ");
      Serial0.println(newState);
      currentState = newState;
    }

    // Handle recovery if in error state
    if (currentState == STATE_ERROR)
    {
      tryToRecover();
    }
  }

  // Update UI every 1000ms
  if (now - lastDisplayUpdate > 1000)
  {
    lastDisplayUpdate = now;
    update_ui();
  }

  // Output data every 10 seconds when in normal state
  if (currentState == STATE_NORMAL && now - lastDataOutput > 10000)
  {
    lastDataOutput = now;
    outputCurrentData();
  }

  // Small delay to prevent overwhelming the system
  delay(5);
}
