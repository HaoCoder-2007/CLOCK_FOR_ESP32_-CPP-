#include <WiFi.h>
#include "time.h"
#include <SPI.h>
#include <TFT_eSPI.h> 

TFT_eSPI tft = TFT_eSPI(); 

const char* ssid     = "/your wifi name/";
const char* password = "/your wifi password/";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  // Khởi tạo LCD
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("WiFi Connected!");

  // Cấu hình thời gian
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
}

void loop() {
  displayLocalTime();
  delay(1000);
}

void displayLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    tft.setCursor(0, 40);
    tft.println("Failed to get time");
    return;
  }

  // Hiển thị Ngày tháng
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.printf("%02d/%02d/%d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

  // Hiển thị Giờ:Phút:Giây với font lớn hơn
  //tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // Sử dụng font số 7 (như đồng hồ điện tử) nếu đã khai báo trong build_flags
  //tft.drawString(asctime(&timeinfo), 10, 100, 4); 
  
  // Hoặc dùng printf đơn giản
  tft.setCursor(10, 150);
  tft.setTextSize(5);
  tft.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  // In ra Serial để debug
  Serial.printf("%02d/%02d/%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}
