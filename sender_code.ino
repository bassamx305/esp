#include <esp_now.h>     // لتشغيل بروتوكول ESP-NOW بدون WiFi
#include <WiFi.h>        // مطلوب لتشغيل الـ WiFi
#include <HTTPClient.h>  // لإرسال البيانات إلى سرفر HTTP مثل ThingSpeak
#include "DHT.h"         // لقراءة حاسس الحرارة والرطوبة

const char* ssid = "STC_5G";      // اسم شبكة الواي فاي
const char* password = "0504446887";  // كلمة مرور الواي فاي

String apiKey = "DEGKG0P5L0ZD04ZC";  // مفتاح الكتابة الخاص بقناتك في ThingSpeak

String server = "http://api.thingspeak.com/update"; // عنوان الخادم الذي يستقبل البيانات

// لوحة أخرى (LED) للجهاز المستقبل MAC عنوان
uint8_t broadcastAddress[] = {0x10, 0x97, 0x21, 0x22, 0x50, 0x18};

// هيكل البيانات التي سنرسلها عبر ESP-NOW
typedef struct struct_message {
  int trafficStatus; // مستوى الازدحام (2 / 1 / 0)
  float humidity;    // قيمة الرطوبة الفعلية
} struct_message;

struct_message myData; // متغير لتخزين البيانات قبل الإرسال

esp_now_peer_info_t peerInfo; // معلومات الجهاز المستقبل

const int dhtPin = 15; // رجل حساس الرطوبة
const int radarPin = 13; // رجل حساس الرادار
const int ldrPin = 34;   // رجل حساس الضوء LDR

DHT dht(dhtPin, DHT11);

// دالة لمعرفة حالة الإرسال ناجح أو فشل
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // يمكن استخدامها لمعرفة حالة الإرسال، تركتها فارغة لأننا لا نحتاجها حالياً
}

void setup() {
  Serial.begin(115200); // تشغيل الشاشة التسلسلية للمراقبة
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // انتظر حتى يتصل الجهاز بالإنترنت
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  
  // ------------------------- تهيئة ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // تسجيل دالة التغذية المرتدة بعد الإرسال
  esp_now_register_send_cb(OnDataSent);
  
  // إضافة الجهاز المستقبل
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;     // نفس القناة
  peerInfo.encrypt = false; // بدون تشفير
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  pinMode(radarPin, INPUT); // الرادار دخل
  pinMode(ldrPin, INPUT);   // الـ LDR دخل
  dht.begin();              // تشغيل حساس الرطوبة
}

void loop() {
  int ldrValue = digitalRead(ldrPin);
  // HIGH = ظلام
  // LOW = ضوء
  
  // إذا كان النهار = لا نرسل بيانات
  if (ldrValue == LOW) {
    myData.trafficStatus = 9; // كود خاص لوضع النوم
    myData.humidity = 0.0;
    
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    Serial.println("Day Mode (Sleep)");
    delay(2000);
    return; // لخروج من loop
  }
  
  int motionSamples = 0;
  
  for (int i = 0; i < 30; i++) {
    if (digitalRead(radarPin)) motionSamples++;
    delay(100);
  }
  
  // تحديد مستوى الازدحام
  int tStatus;
  if (motionSamples == 0)      tStatus = 0; // بعيد
  else if (motionSamples < 15) tStatus = 1; // متوسط
  else                         tStatus = 2; // عالي
  
  float h = dht.readHumidity();
  if (isnan(h)) h = 0.0; // حماية من القراءة الخاطئة
  
  myData.trafficStatus = tStatus;
  myData.humidity = h;
  
  // إرسال عبر بروتوكول ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  // إرسال إلى ThingSpeak عبر الواي فاي في حال كان متصلاً
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // تكوين الرابط مع الحقول
    String url = server + 
                 "?api_key=" + apiKey + 
                 "&field1=" + String(tStatus) + 
                 "&field2=" + String(h) + 
                 "&field3=" + String(ldrValue);
                 
    http.begin(url); // بدء الاتصال
    int httpCode = http.GET(); // إرسال الطلب
    http.end(); // إنهاء الاتصال
    
    Serial.print("HTTP Sent | Code: ");
    Serial.println(httpCode);
  }
  
  Serial.print("Night Mode | Traffic: ");
  Serial.print(tStatus);
  Serial.print(" | Humidity: ");
  Serial.println(h);
  
  // انتظار 20 ثانية (شرط ThingSpeak)
  delay(20000);
}
