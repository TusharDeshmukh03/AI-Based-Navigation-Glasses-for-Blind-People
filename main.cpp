#include <WiFi.h>
#include <HTTPClient.h>
#include <Base64.h>
#include "esp_camera.h"
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "SS_ID";
const char* password = "PASSWORD";

// OpenAI API Key
const String apiKey = "API_KEY";  // Replace with your key

// UART Config
#define BAUD_RATE 115200

// Camera Pins (AI Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  Serial.begin(BAUD_RATE);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Camera Config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  delay(2000);
  captureAndAnalyzeImage();
}

void loop() {}

void captureAndAnalyzeImage() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  String base64Image = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  DynamicJsonDocument doc(4096);
  doc["model"] = "gpt-4o";

  JsonArray messages = doc.createNestedArray("messages");
  JsonObject msg = messages.createNestedObject();
  msg["role"] = "user";
  JsonArray content = msg.createNestedArray("content");

  JsonObject textObj = content.createNestedObject();
  textObj["type"] = "text";
  textObj["text"] = "I am blind. Describe what's in this room and how far it is.";

  JsonObject imgObj = content.createNestedObject();
  imgObj["type"] = "image_url";
  JsonObject imgUrl = imgObj.createNestedObject("image_url");
  imgUrl["url"] = "data:image/jpeg;base64," + base64Image;
  imgUrl["detail"] = "auto";

  doc["max_tokens"] = 400;

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin("https://api.openai.com/v1/chat/completions");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + apiKey);

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    String response = http.getString();
    DynamicJsonDocument resDoc(8192);
    DeserializationError err = deserializeJson(resDoc, response);
    if (!err) {
      String content = resDoc["choices"][0]["message"]["content"].as<String>();
      Serial.println(content); // Send over UART
    } else {
      Serial.println("JSON parse error");
    }
  } else {
    Serial.println("API request failed");
  }
  http.end();
}
