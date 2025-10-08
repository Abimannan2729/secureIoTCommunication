#include <DHT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <AESLib.h>
#include "mbedtls/md.h"

// ------------------------ DHT Setup ------------------------
#define DHTPIN 26
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ------------------------ WiFi ------------------------
const char* ssid = "Abimannan";
const char* password = "abi272901";

// ------------------------ AWS IoT ------------------------
const char* aws_endpoint = "a2jis5k15pyxz8-ats.iot.us-east-1.amazonaws.com";



// ---------------- AES & HMAC ----------------
AESLib aesLib;
byte aes_key[16] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
                     0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81 };

byte aes_iv[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

const char* hmac_key = "S3cUr3!HMACk3y1234";

// ---------------- Network Clients ----------------
WiFiClientSecure net;
PubSubClient client(net);

// ---------------- Helper Functions ----------------
String bytesToHex(const byte* buf, int len) {
  String s;
  for (int i = 0; i < len; i++) {
    char hex[3];
    sprintf(hex, "%02X", buf[i]);
    s += hex;
  }
  return s;
}

String encryptAES(String msg) {
  int blockSize = 16;
  int msgLen = msg.length();
  int paddedLen = ((msgLen + blockSize - 1) / blockSize) * blockSize;

  byte plain[paddedLen];
  memset(plain, 0, paddedLen);            // Zero padding
  msg.getBytes(plain, msgLen + 1);

  byte cipher[paddedLen];
  aesLib.encrypt(plain, paddedLen, cipher, aes_key, 128, aes_iv);

  return bytesToHex(cipher, paddedLen);
}

String generateHMAC(String data) {
  byte hmac_result[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)hmac_key, strlen(hmac_key));
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)data.c_str(), data.length());
  mbedtls_md_hmac_finish(&ctx, hmac_result);
  mbedtls_md_free(&ctx);

  return bytesToHex(hmac_result, 32);
}

// ---------------- WiFi & AWS Functions ----------------
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

void connectToAWS() {
  net.setCACert(root_ca);
  net.setCertificate(device_cert);
  net.setPrivateKey(private_key);
  client.setServer(aws_endpoint, 8883);

  while (!client.connected()) {
    Serial.println("Connecting to AWS IoT...");
    if (client.connect("ESP32Abi")) {
      Serial.println("✅ Connected to AWS IoT!");
    } else {
      Serial.print("❌ Connection failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ---------------- Setup & Loop ----------------
void setup() {
  Serial.begin(115200);
  dht.begin();
  connectToWiFi();
  connectToAWS();
}

void loop() {
  if (!client.connected()) connectToAWS();
  client.loop();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    String json = "{\"temperature\":" + String(temp, 2) + ",\"humidity\":" + String(hum, 2) + "}";
    String encrypted = encryptAES(json);
    String hmac = generateHMAC(encrypted);

    String payload = "{\"data\":\"" + encrypted + "\",\"hmac\":\"" + hmac + "\"}";
    client.publish("esp32/dht22", payload.c_str());
    Serial.println("🔐 Published Secure Payload: " + payload);
  } else {
    Serial.println("⚠️ Failed to read DHT22 sensor");
  }

  delay(5000);
}
