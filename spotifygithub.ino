#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// OLED Screen Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C // If screen stays black, try 0x3D

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================== CONFIGURATION ====================
const char* ssid         = "wifi name";
const char* password     = "wifi pass";

const char* clientId     = "client id given by spotify ";
const char* clientSecret = "client id password";
const char* refreshToken = "refresh token ";
// =======================================================

// Spotify Playback State
String accessToken = "";
unsigned long tokenExpiresAt = 0;
bool isPlaying = false;
String trackName = "No track playing";
String artistName = "";
long progressMs = 0;
long durationMs = 0;

unsigned long lastPollTime = 0;
const unsigned long pollInterval = 2000; // Poll Spotify every 2 seconds
int scrollX = 0;

// 16x16 Pixel Bitmaps for Play (▶) and Pause (❚❚) Icons
const unsigned char PROGMEM playIcon[] = {
  0x00, 0x00, 0x01, 0x80, 0x03, 0x80, 0x07, 0x80, 
  0x0f, 0x80, 0x1f, 0x80, 0x3f, 0x80, 0x7f, 0x80, 
  0x7f, 0x80, 0x3f, 0x80, 0x1f, 0x80, 0x0f, 0x80, 
  0x07, 0x80, 0x03, 0x80, 0x01, 0x80, 0x00, 0x00
};

const unsigned char PROGMEM pauseIcon[] = {
  0x00, 0x00, 0x73, 0xce, 0x73, 0xce, 0x73, 0xce, 
  0x73, 0xce, 0x73, 0xce, 0x73, 0xce, 0x73, 0xce, 
  0x73, 0xce, 0x73, 0xce, 0x73, 0xce, 0x73, 0xce, 
  0x73, 0xce, 0x73, 0xce, 0x73, 0xce, 0x00, 0x00
};

// Request a fresh 1-hour access token using the Refresh Token
bool refreshAccessToken() {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure(); // Bypass SSL fingerprint checking

  HTTPClient http;
  http.begin(client, "https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "grant_type=refresh_token&refresh_token=" + String(refreshToken) +
                    "&client_id=" + String(clientId) +
                    "&client_secret=" + String(clientSecret);

  int httpCode = http.POST(postData);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    accessToken = doc["access_token"].as<String>();
    int expiresIn = doc["expires_in"].as<int>();
    tokenExpiresAt = millis() + ((expiresIn - 60) * 1000UL); // Refresh 1 min early
    http.end();
    return true;
  }

  http.end();
  return false;
}

// Ask Spotify what is currently playing
void fetchCurrentlyPlaying() {
  if (millis() > tokenExpiresAt || accessToken == "") {
    if (!refreshAccessToken()) return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization", "Bearer " + accessToken);

  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      isPlaying   = doc["is_playing"].as<bool>();
      trackName   = doc["item"]["name"].as<String>();
      artistName  = doc["item"]["artists"][0]["name"].as<String>();
      progressMs  = doc["progress_ms"].as<long>();
      durationMs  = doc["item"]["duration_ms"].as<long>();
    }
  } else if (httpCode == 204) { // 204: Spotify is idle/no playback
    isPlaying  = false;
    trackName  = "Playback Paused";
    artistName = "Play on Spotify";
    progressMs = 0;
    durationMs = 0;
  }

  http.end();
}

// Draw the UI on the 0.96" OLED
void renderDisplay() {
  display.clearDisplay();

  // 1. Play / Pause Icon at the top center
  int iconX = (SCREEN_WIDTH - 16) / 2;
  int iconY = 1;
  if (isPlaying) {
    display.drawBitmap(iconX, iconY, playIcon, 16, 16, SSD1306_WHITE);
  } else {
    display.drawBitmap(iconX, iconY, pauseIcon, 16, 16, SSD1306_WHITE);
  }

  // 2. Divider line under header
  display.drawFastHLine(0, 19, SCREEN_WIDTH, SSD1306_WHITE);

  // 3. Song Title (with smooth marquee scrolling for long titles)
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (trackName.length() > 21) {
    display.setCursor(scrollX, 24);
    display.print(trackName);
    scrollX -= 2;
    if (abs(scrollX) > (int)(trackName.length() * 6)) {
      scrollX = SCREEN_WIDTH;
    }
  } else {
    display.setCursor(0, 24);
    display.print(trackName);
  }

  // 4. Artist Name
  display.setCursor(0, 38);
  if (artistName.length() > 21) {
    display.print(artistName.substring(0, 18) + "...");
  } else {
    display.print(artistName);
  }

  // 5. Progress Bar at bottom
  display.drawRect(0, 54, SCREEN_WIDTH, 6, SSD1306_WHITE);
  if (durationMs > 0) {
    int barWidth = map(progressMs, 0, durationMs, 0, SCREEN_WIDTH - 4);
    display.fillRect(2, 56, constrain(barWidth, 0, SCREEN_WIDTH - 4), 2, SSD1306_WHITE);
  }

  display.display();
}

void setup() {
  Serial.begin(115200);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED init failed. Check wiring!"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(15, 25);
  display.print("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  display.clearDisplay();
  display.setCursor(15, 25);
  display.print("Connecting Spotify...");
  display.display();

  refreshAccessToken();
}

void loop() {
  // Query Spotify every 2 seconds
  if (millis() - lastPollTime >= pollInterval) {
    lastPollTime = millis();
    fetchCurrentlyPlaying();
  }

  renderDisplay();
  delay(40); // Smooth scroll frame rate
}
