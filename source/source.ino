#include <WiFi.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include "info.dev.h"

// Cấu hình chân
#define LCD_CS 15
#define BOOT_BUTTON 0

TFT_eSPI tft = TFT_eSPI();

// Biến WiFi
const char* ssidList[] = { SECRET_SSID_1, SECRET_SSID_2 }; //SSID
const char* passwordList[] = { SECRET_PASS_1, SECRET_PASS_2 }; //PASSWORD
int netCount = sizeof(ssidList) / sizeof(ssidList[0]);
int indexWiFi;
String info = "HaoCoder ";

// Biến thời gian
const char* ntpServer = "time.google.com";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;
String country[] = { "VN", "US", "FR", "JP" };
String cities[] = { "Ho Chi Minh", "New York", "Paris", "Tokyo" };
long gmtOffsets[] = { 7 * 3600, -4 * 3600, 2 * 3600, 9 * 3600 };
const char* daysOfWeek[] = { "Sunday   ", "Monday   ", "Tuesday  ", "Wednesday", "Thursday ", "Friday   ", "Saturday " };
int countryIdx = 0;

// Biến thời tiết
const String apiKey = SECRET_API_KEY; //API KEY
String weatherTemp = "--°C";
String weatherDesc = "...";
uint16_t tempColor = TFT_WHITE;
uint16_t descColor = TFT_WHITE;
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherInterval = 600000;

// Biến đồ họa
int catTailAngle = 0;
int catDirection = 1;

void setup() {
  Serial.begin(115200);

  // Khởi tạo nút bấm BOOT
  pinMode(BOOT_BUTTON, INPUT_PULLUP);

  // Khởi tạo màn hình
  pinMode(LCD_CS, OUTPUT);
  digitalWrite(LCD_CS, LOW);
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Kết nối Wi-Fi
  bool connected = false;
  for (indexWiFi = 0; indexWiFi < netCount; indexWiFi++) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 10);
    tft.setTextSize(2);
    Serial.printf("Trying WiFi %d/%d:\n%s", indexWiFi + 1, netCount, ssidList[indexWiFi]);
    tft.printf("Connecting to %s", ssidList[indexWiFi]);

    WiFi.begin(ssidList[indexWiFi], passwordList[indexWiFi]);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
      delay(500);
      if (retry < 3) {
        tft.print(".");
        Serial.print(".");
      }
      retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(10, 10);
      tft.println("Wi-Fi Connected!");
      Serial.println("\nConnected!");
      break;
    } else {
      Serial.println("\nFailed.");
      WiFi.disconnect();
    }
  }

  if (!connected) {
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 10);
    tft.println("No WiFi found! Restarting...");
    delay(2000);
    ESP.restart();
  }

  delay(1000);

  // Cấu hình thời gian
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("Syncing Time...");

  configTime(gmtOffsets[countryIdx], 0, ntpServer);
  struct tm timeinfo;
  int retry = 0;
  while (!getLocalTime(&timeinfo) && retry < 20) {
    delay(500);
    tft.print(".");
    retry++;
  }

  updateWeather();
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  if (digitalRead(BOOT_BUTTON) == LOW) {
    delay(50);
    if (digitalRead(BOOT_BUTTON) == LOW) {

      countryIdx++;
      if (countryIdx >= 4) countryIdx = 0;
      configTime(gmtOffsets[countryIdx], 0, ntpServer);

      Serial.printf("Switched to %s\n", country[countryIdx].c_str());
      
      updateWeather();

      while (digitalRead(BOOT_BUTTON) == LOW) { delay(10); }
    }
  }
  if (millis() - lastWeatherUpdate >= weatherInterval) {
    updateWeather();
  }

  display();
  delay(500);
}

void display() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  // Hiển thị thông tin cá nhân
  tft.setCursor(20, 10);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.printf("%s", info);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.print("T");
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print("D");
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.print("T");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("U");

  // Hiển thị Thứ/Ngày/Tháng/Năm
  tft.setCursor(55, 80);
  tft.setTextSize(3);
  tft.printf("%s", daysOfWeek[timeinfo.tm_wday]);
  tft.setCursor(255, 80);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.printf("%02d/%02d/%d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

  // Hiển thị Giờ/Phút/Giấy
  tft.setCursor(40, 140);
  tft.setTextSize(7);
  tft.printf("%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Hiển thị Quốc gia
  tft.setCursor(370, 165);
  tft.setTextSize(3);
  tft.printf(" [%s]", country[countryIdx]);

  // Hiển thị Thời tiết
  tft.setTextSize(2);
  tft.setTextColor(tempColor, TFT_BLACK);
  tft.setCursor(390, 140);
  tft.printf("%s   ", weatherTemp.c_str());
  // tft.setTextColor(descColor, TFT_BLACK);
  // tft.setCursor(310, 30);
  // tft.printf("(%s)     ", weatherDesc.c_str());

  // Vẽ gì đó
  // drawCat(240, 290);  //Vẽ mèo

  // In ra Serial để debug
  Serial.printf("%02d/%02d/%02d\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  Serial.printf("%s", country[countryIdx]);
  Serial.printf("%s", weatherTemp.c_str());
  Serial.printf("\n");
}

void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String cityName = cities[countryIdx];
    cityName.replace(" ", "%20"); 
    
    // URL gọi API lấy thời tiết (Sử dụng cityName đã mã hóa)
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + cityName + "&appid=" + apiKey + "&units=metric";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);
      
      // Nếu Serial hiện code: 200 là thành công, 404 là sai tên
      Serial.printf("API Response Code: %d\n", httpCode); 

      float temp = doc["main"]["temp"];
      const char* desc = doc["weather"][0]["main"];
      
      weatherTemp = String((int)temp) + " \xF7" + "C";
      weatherDesc = String(desc);
      lastWeatherUpdate = millis();

      int t = (int)temp;
      if (t >= 40) {
        tempColor = TFT_RED;     
      } else if (t >= 30) {
        tempColor = TFT_ORANGE;
      } else if (t >= 20) {
        tempColor = TFT_GOLD;
      } else if (t >= 10) {
        tempColor = TFT_YELLOW;
      } else if (t >= 0) {
        tempColor = TFT_WHITE; 
      } else {
        tempColor = tft.color565(0, 191, 255);
      }

      // if (weatherDesc == "Clear") {
      //   descColor = tft.color565(0, 191, 255);
      // } else if (weatherDesc == "Clouds") {
      //   descColor = TFT_LIGHTGREY;
      // } else if (weatherDesc == "Rain" || weatherDesc == "Drizzle") {
      //   descColor = TFT_CYAN;
      // } else if (weatherDesc == "Thunderstorm") {
      //   descColor = TFT_YELLOW;
      // } else {
      //   descColor = TFT_WHITE;
      // }

    } else {
      Serial.printf("HTTP Client Error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}

// void drawCat(int x, int y) {
//   tft.fillCircle(x, y - 30, 20, TFT_WHITE);
//   tft.fillTriangle(x - 18, y - 40, x - 12, y - 55, x - 5, y - 45, TFT_WHITE);
//   tft.fillTriangle(x + 18, y - 40, x + 12, y - 55, x + 5, y - 45, TFT_WHITE);

//   if (millis() % 4000 < 3800) {
//     tft.fillCircle(x - 7, y - 35, 3, TFT_BLACK);
//     tft.fillCircle(x + 7, y - 35, 3, TFT_BLACK);
//   } else {
//     tft.drawFastHLine(x - 10, y - 35, 6, TFT_BLACK);
//     tft.drawFastHLine(x + 4, y - 35, 6, TFT_BLACK);
//   }
//   tft.drawLine(x - 20, y - 32, x - 35, y - 35, TFT_WHITE);
//   tft.drawLine(x - 20, y - 30, x - 35, y - 30, TFT_WHITE);
//   tft.drawLine(x - 20, y - 28, x - 35, y - 25, TFT_WHITE);
//   tft.drawLine(x + 20, y - 32, x + 35, y - 35, TFT_WHITE);
//   tft.drawLine(x + 20, y - 30, x + 35, y - 30, TFT_WHITE);
//   tft.drawLine(x + 20, y - 28, x + 35, y - 25, TFT_WHITE);
// }