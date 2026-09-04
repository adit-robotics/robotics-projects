#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <time.h>

#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 17
#define TOUCH_PIN 32

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const float LATITUDE = 18.5204;
const float LONGITUDE = 73.8567;
const long GMT_OFFSET_SEC = 19800;
const int DAYLIGHT_OFFSET_SEC = 0;

enum Mode { EYES_MODE, TIME_MODE, WEATHER_MODE };
Mode currentMode = EYES_MODE;

bool lastTouchState = false;
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE = 300;

#define EYE_Y 78
#define LEFT_EYE_X 40
#define RIGHT_EYE_X 88
#define EYE_W 35
#define EYE_H 46

float leftPupilX = 0, rightPupilX = 0;
float targetLeftPupilX = 0, targetRightPupilX = 0;
float leftPupilY = 0, rightPupilY = 0;
float targetLeftPupilY = 0, targetRightPupilY = 0;

unsigned long lastEyeUpdate = 0;
unsigned long nextLookTime = 0;
bool blinking = false;
unsigned long blinkStart = 0;
unsigned long nextBlink = 0;
const float EYE_SMOOTHING = 0.18;

String weatherText = "Loading...";
float temperature = 0;
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_UPDATE_INTERVAL = 15UL * 60UL * 1000UL;

void drawEyes();
void drawClosedEyes();
void updateEyes();
void chooseNewLook();
void drawTime();
void drawWeather();
void checkTouch();
void getWeather();
String getWeatherDescription(int code);

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST7735_BLACK);

  randomSeed(analogRead(34));
  unsigned long now = millis();
  nextLookTime = now + random(1000, 2500);
  nextBlink = now + random(2500, 5000);
  drawEyes();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long wifiStart = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");

  if (WiFi.status() == WL_CONNECTED) getWeather();
}

void loop() {
  checkTouch();

  if (currentMode == EYES_MODE) updateEyes();

  static unsigned long lastTimeDraw = 0;
  if (currentMode == TIME_MODE && millis() - lastTimeDraw > 1000) {
    lastTimeDraw = millis();
    drawTime();
  }

  if (millis() - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL) {
    lastWeatherUpdate = millis();
    if (WiFi.status() == WL_CONNECTED) {
      getWeather();
      if (currentMode == WEATHER_MODE) drawWeather();
    }
  }
}

void drawCuteEye(int x, int y, float pupilX, float pupilY) {
  tft.fillRoundRect(x - EYE_W / 2, y - EYE_H / 2, EYE_W, EYE_H, 14, ST7735_WHITE);
  tft.fillCircle(x + pupilX, y + pupilY, 13, ST7735_CYAN);
  tft.fillCircle(x + pupilX, y + pupilY, 7, ST7735_BLACK);
  tft.fillCircle(x + pupilX - 3, y + pupilY - 3, 3, ST7735_WHITE);
}

void drawEyes() {
  tft.fillRect(0, 50, 128, 58, ST7735_BLACK);
  drawCuteEye(LEFT_EYE_X, EYE_Y, leftPupilX, leftPupilY);
  drawCuteEye(RIGHT_EYE_X, EYE_Y, rightPupilX, rightPupilY);
}

void drawClosedEyes() {
  tft.fillRect(0, 50, 128, 58, ST7735_BLACK);
  tft.drawLine(LEFT_EYE_X - 14, EYE_Y, LEFT_EYE_X + 14, EYE_Y, ST7735_WHITE);
  tft.drawLine(RIGHT_EYE_X - 14, EYE_Y, RIGHT_EYE_X + 14, EYE_Y, ST7735_WHITE);
}

void chooseNewLook() {
  int direction = random(0, 7);
  float x = 0, y = 0;

  if (direction <= 2) {
    x = random(-2, 3);
    y = random(-2, 3);
  } else if (direction == 3) {
    x = random(-6, -3);
    y = random(-2, 3);
  } else if (direction == 4) {
    x = random(3, 7);
    y = random(-2, 3);
  } else if (direction == 5) {
    x = random(-3, 4);
    y = random(-4, -1);
  } else {
    x = random(-3, 4);
    y = random(1, 4);
  }

  targetLeftPupilX = x + random(-10, 11) / 10.0;
  targetRightPupilX = x + random(-10, 11) / 10.0;
  targetLeftPupilY = y + random(-5, 6) / 10.0;
  targetRightPupilY = y + random(-5, 6) / 10.0;
}

void updateEyes() {
  unsigned long now = millis();

  if (!blinking) {
    leftPupilX += (targetLeftPupilX - leftPupilX) * EYE_SMOOTHING;
    rightPupilX += (targetRightPupilX - rightPupilX) * EYE_SMOOTHING;
    leftPupilY += (targetLeftPupilY - leftPupilY) * EYE_SMOOTHING;
    rightPupilY += (targetRightPupilY - rightPupilY) * EYE_SMOOTHING;
  }

  if (now - lastEyeUpdate >= 33) {
    lastEyeUpdate = now;
    if (!blinking) drawEyes();
  }

  if (!blinking && now >= nextLookTime) {
    chooseNewLook();
    nextLookTime = now + random(900, 2800);
  }

  if (!blinking && now >= nextBlink) {
    blinking = true;
    blinkStart = now;
    drawClosedEyes();
  }

  if (blinking && now - blinkStart >= 140) {
    blinking = false;
    drawEyes();
    nextBlink = now + random(2500, 6500);
  }
}

void checkTouch() {
  bool touchState = digitalRead(TOUCH_PIN);

  if (touchState && !lastTouchState && millis() - lastTouchTime > TOUCH_DEBOUNCE) {
    lastTouchTime = millis();

    if (currentMode == EYES_MODE) currentMode = TIME_MODE;
    else if (currentMode == TIME_MODE) currentMode = WEATHER_MODE;
    else currentMode = EYES_MODE;

    tft.fillScreen(ST7735_BLACK);

    if (currentMode == EYES_MODE) drawEyes();
    else if (currentMode == TIME_MODE) drawTime();
    else drawWeather();
  }

  lastTouchState = touchState;
}

void drawTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 65);
    tft.print("No time");
    return;
  }

  char timeString[10];
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);

  char dateString[20];
  strftime(dateString, sizeof(dateString), "%d/%m/%Y", &timeinfo);

  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(2);
  tft.setCursor(15, 55);
  tft.print(timeString);

  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(37, 85);
  tft.print(dateString);
  tft.setCursor(42, 110);
  tft.print("^_^");
}

void getWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    weatherText = "No WiFi";
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url =
    "https://api.open-meteo.com/v1/forecast?"
    "latitude=" + String(LATITUDE, 4) +
    "&longitude=" + String(LONGITUDE, 4) +
    "&current=temperature_2m,weather_code";

  Serial.println("Getting weather...");
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      temperature = doc["current"]["temperature_2m"];
      int weatherCode = doc["current"]["weather_code"];
      weatherText = getWeatherDescription(weatherCode);
      Serial.print("Temperature: ");
      Serial.println(temperature);
      Serial.print("Weather: ");
      Serial.println(weatherText);
    }
  } else {
    Serial.print("Weather HTTP error: ");
    Serial.println(httpCode);
    weatherText = "Weather error";
  }

  http.end();
}

String getWeatherDescription(int code) {
  if (code == 0) return "Clear";
  if (code == 1 || code == 2 || code == 3) return "Cloudy";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 95) return "Storm";
  return "Unknown";
}

void drawWeather() {
  tft.fillScreen(ST7735_BLACK);

  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(2);
  tft.setCursor(25, 20);
  tft.print("WEATHER");

  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(3);
  tft.setCursor(25, 55);
  tft.print(temperature, 1);
  tft.print("C");

  tft.setTextSize(1);
  int textWidth = weatherText.length() * 6;
  int textX = (128 - textWidth) / 2;
  if (textX < 0) textX = 0;

  tft.setCursor(textX, 95);
  tft.print(weatherText);
  tft.setCursor(48, 120);
  tft.print("^_^");
}
