#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include <FirebaseArduino.h>

// WiFi bağlantı bilgileri
const char *ssid = "S24FE";      // WiFi ağı adı
const char *password = "furkanbal"; // WiFi ağı şifresi

// Web sunucu adresi ve portu
const char* web_adresi = "checkip.dyndns.org";
const uint16_t port = 80;
String uzanti = "/";

// Firebase bilgileri
#define FIREBASE_HOST "final2024-83658-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "sUQEwDuxtMOoxXrPe0cN8zYUD5tqURMXsPC5Uc0h"

// Thingspeak bilgileri
const unsigned long channelID = 2782306;
const char *readAPIKey = "S3RJE01HQL5GDSE8";
const int fieldNumber = 7;

WiFiClient wifi_istemci;

void setup() {
  Serial.begin(9600);
  delay(1500);
  Serial.println("WiFi ağına bağlanıyor...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi ağına bağlanıldı.");
  Serial.print("Alınan IP adresi: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(wifi_istemci);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() {
  // Web sunucusuna bağlanma
  if (!wifi_istemci.connect(web_adresi, port)) {
    Serial.println("Sunucuya bağlanma hatası.");
    delay(5000);
    return;
  }
  Serial.println("WEB sunucusuna bağlanıldı.");

  // Sunucudan IP adresini isteme
  String http_req = "GET " + uzanti + " HTTP/1.1\r\n";
  http_req += "Host: " + String(web_adresi) + "\r\n";
  http_req += "Connection: close\r\n\r\n";

  wifi_istemci.print(http_req);

  unsigned long onceki_zaman = millis();
  while (wifi_istemci.available() == 0) {
    if (millis() - onceki_zaman > 5000) {
      Serial.println("Bağlantı hatası.");
      wifi_istemci.stop();
      delay(2000);
      return;
    }
  }

  // Gelen veriyi işleme
  String ip_adresim = "";
  while (wifi_istemci.available()) {
    String satir = wifi_istemci.readStringUntil('\r');

    if (satir.indexOf("Address: ") != -1) {
      int baslangic = satir.indexOf("Address: ") + 9;
      int bitis = satir.indexOf("</body>");
      ip_adresim = satir.substring(baslangic, bitis);
      Serial.println("IP Adresi: " + ip_adresim);
    }
  }
  wifi_istemci.stop();

  // IP adresinin ilk üç hanesini integer olarak al
  int ip_ilk_uc = ip_adresim.toInt();
  Serial.println("IP ilk üç hane: " + String(ip_ilk_uc));

  // Thingspeak'ten veri okuma
  float deger = ThingSpeak.readFloatField(channelID, fieldNumber, readAPIKey);
  Serial.println("Thingspeak'ten okunan değer: " + String(deger));

  // Firebase'e sonuç yazma
  float sonuc = 2 * deger + ip_ilk_uc;
  String etiket = "202213172032: " + String(sonuc);

  Firebase.setFloat("Sonuc", sonuc);

  if (Firebase.failed()) {
    Serial.println("Firebase'e yazma hatası.");
    return;
  }

  Serial.println("Sonuç Firebase'e yazıldı: " + etiket);
  delay(15000); // Thingspeak'in güncelleme periyoduna uyum sağlamak için bekle
}
