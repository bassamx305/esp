#include <esp_now.h> // مكتبة الاتصال اللاسلكي ESP-NOW
#include <WiFi.h>    // ضرورية لتفعيل وضع WIFI_STA

// الأبيض -> ازدحام بسيط LED
const int WHITE_LED = 12;

// الأصفر -> ازدحام متوسط LED
const int YELLOW_LED = 26;

// الأحمر -> ازدحام عالي أو رطوبة عالية LED
const int RED_LED = 27;

// يجب أن يكون مطابقاً تماماً لهيكل المرسل
typedef struct struct_message {
  int trafficStatus; // مستوى الازدحام (0، 1، 2) أو (9)
  float humidity;    // قيمة الرطوبة %
} struct_message;

struct_message myData; // متغير لتخزين البيانات المستلمة

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // نسخ البيانات القادمة إلى المتغير myData
  memcpy(&myData, incomingData, sizeof(myData));
  
  digitalWrite(WHITE_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  
  // وضع النهار (Sleep Mode)
  // المرسل يرسل القيمة 9 عندما يكون النهار ولا نكمل التنفيذ، هنا نطفئ كل الـ LEDs
  if (myData.trafficStatus == 9) {
    Serial.println("Day Mode: All LEDs OFF");
    return;
  }
  
  Serial.print("Humidity: ");
  Serial.print(myData.humidity);
  Serial.print("% | Traffic: ");
  
  // تحويل أرقام الازدحام إلى وصف نصي
  if (myData.trafficStatus == 0) {
    Serial.println("Clear (Low Traffic)");
  } else if (myData.trafficStatus == 1) {
    Serial.println("Medium Traffic");
  } else if (myData.trafficStatus == 2) {
    Serial.println("High Traffic");
  }
  
  // أولوية قصوى للرطوبة العالية //
  // أحمر LED -> إذا كانت الرطوبة أكبر من 60%
  if (myData.humidity > 60.0) {
    digitalWrite(RED_LED, HIGH);
  }
  // إذا لم تكن الرطوبة عالية //
  // تتحكم حسب الازدحام
  else if (myData.trafficStatus == 2) {
    digitalWrite(RED_LED, HIGH);
  }
  else if (myData.trafficStatus == 1) {
    digitalWrite(YELLOW_LED, HIGH);
  }
  else {
    digitalWrite(WHITE_LED, HIGH);
  }
}

void setup() {
  // تشغيل الشاشة التسلسلية للمراقبة
  Serial.begin(115200);
  
  pinMode(WHITE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  digitalWrite(WHITE_LED, HIGH);
  delay(200);
  digitalWrite(WHITE_LED, LOW);
  
  digitalWrite(YELLOW_LED, HIGH);
  delay(200);
  digitalWrite(YELLOW_LED, LOW);
  
  digitalWrite(RED_LED, HIGH);
  delay(200);
  digitalWrite(RED_LED, LOW);
  
  WiFi.mode(WIFI_STA);
  
  // تهيئة ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  
  // تسجيل دالة الاستقبال
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  
  Serial.println("ESP-NOW Receiver Ready");
}

void loop() {
  // كل العمل يتم عند استقبال البيانات
  delay(100);
}
