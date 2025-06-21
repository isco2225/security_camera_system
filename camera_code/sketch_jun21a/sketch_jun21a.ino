#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "soc/soc.h"          
#include "soc/rtc_cntl_reg.h" 
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <time.h>

// Replace with your network credentials
const char *ssid = "Your wifi ssid";
const char *password = "wifi password";


// RGB LED pinleri
#define R_PIN 14
#define G_PIN 12
#define B_PIN 15

// PWM
#define PWM_FREQ 5000  // PWM frekansı
#define PWM_RESOLUTION 8  
#define R_CHANNEL 0  // PWM kanal numaraları
#define G_CHANNEL 1
#define B_CHANNEL 2


// PIR sensor pin
#define PIR_SENSOR_PIN 13

// Firebase credentials
#define API_KEY "Your Api Key"
#define USER_EMAIL "example@gmail.com"
#define USER_PASSWORD "example"
#define STORAGE_BUCKET_ID "your firebase STORAGE_BUCKET_ID"  
#define REALTIME_DATABASE_URL "your firebase REALTIME_DATABASE_URL"  

// Photo File Name to save in LittleFS
#define FILE_PHOTO_PATH "/photo.jpg"
#define BUCKET_PHOTO "/data/photo.jpg"

// Firebase Realtime Database paths
#define DB_Photo_DATE_PATH "/uploadedDate"

// Camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22



boolean takeNewPhoto = true;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 10800; // UTC+3
const int daylightOffset_sec = 0;

void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) { 
    delay(1000);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("Time synced");
}

String getCurrentTime() {
  time_t now;
  struct tm timeinfo;
  char strftime_buf[64];

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return String("N/A");
  }

  strftime(strftime_buf, sizeof(strftime_buf), "%Y/%m/%d %H:%M:%S", &timeinfo);
  return String(strftime_buf);
}

void capturePhotoSaveLittleFS() {
  camera_fb_t* fb = NULL;

  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len);
    Serial.printf("Picture saved to %s - Size: %d bytes\n", FILE_PHOTO_PATH, fb->len);
  }
  file.close();
  esp_camera_fb_return(fb);
}

void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  setLEDColor(0, 255, 0); //Yeşil
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS");
    ESP.restart();
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

void initCamera() {
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
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    ESP.restart();
  }
}

void updateFirebasePhotoDate(String date, String photoUrl) {
  String basePath = "/url_and_uploadedDate/file1";

  // Yüklenme tarihini güncelle
  if (Firebase.RTDB.setString(&fbdo, basePath + "/uploadedDate", date)) {
    Serial.println("Uploaded date updated successfully in Firebase");
  } else {
    Serial.println("Failed to update uploaded date in Firebase");
    Serial.println(fbdo.errorReason());
  }

  // Fotoğraf URL'sini güncelle
  if (Firebase.RTDB.setString(&fbdo, basePath + "/url", photoUrl)) {
    Serial.println("Photo URL updated successfully in Firebase");
  } else {
    Serial.println("Failed to update photo URL in Firebase");
    Serial.println(fbdo.errorReason());
  }
}

void initPWM() {
  ledcSetup(R_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(R_PIN, R_CHANNEL);

  ledcSetup(G_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(G_PIN, G_CHANNEL);

  ledcSetup(B_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(B_PIN, B_CHANNEL);
}

void setLEDColor(int r, int g, int b) {
  ledcWrite(R_CHANNEL, 255 - r);
  ledcWrite(G_CHANNEL, 255 - g);
  ledcWrite(B_CHANNEL, 255 - b);
}

void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  pinMode(R_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(4, OUTPUT);  // GPIO4 pinini çıkış olarak ayarla
  digitalWrite(4, LOW); // Flaşı kapat
  initPWM();
  pinMode(PIR_SENSOR_PIN, INPUT);
  setLEDColor(255, 0, 0);//kırmızı
  initWiFi();
  initLittleFS();
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();
  setupTime();

  configF.api_key = API_KEY;
  configF.database_url = REALTIME_DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  configF.token_status_callback = tokenStatusCallback;

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
      setLEDColor(255, 0, 0); // Kırmızı
      initWiFi();
    }
  if (digitalRead(PIR_SENSOR_PIN) == HIGH) {
    Serial.println("Motion detected!");
    setLEDColor(0, 0, 255); //mavi
    digitalWrite(4, HIGH); // Flaşı kapat
    capturePhotoSaveLittleFS();
    digitalWrite(4, LOW); // Flaşı aç
    String currentTime = getCurrentTime();
    
    // Fotoğrafın benzersiz bir adını oluştur
    String uniquePhotoPath = "/data/photo_" + currentTime + ".jpg";
    uniquePhotoPath.replace(" ", "_"); 
    uniquePhotoPath.replace(":", "-"); 
    Serial.printf("Unique photo path: %s\n", uniquePhotoPath.c_str());
    
    String photoUrl = "";  

    if (Firebase.ready()) {
      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, uniquePhotoPath.c_str(), "image/jpeg")) {
        photoUrl = fbdo.downloadURL().c_str();
        Serial.printf("Uploaded photo: %s\n", photoUrl.c_str());
        
        // Firebase'e tarih ve URL'yi güncelle
        updateFirebasePhotoDate(currentTime, photoUrl);
        LittleFS.remove(FILE_PHOTO_PATH); // Yüklendikten sonra fotoğrafı sil
        Serial.println("Photo deleted from LittleFS after upload.");
        setLEDColor(0, 255, 0); // Yeşil

      } else {
        Serial.println(fbdo.errorReason());
      }
    }

    delay(3000); // Tekrarlanan tetiklemeleri önlemek için gecikme
  }
}