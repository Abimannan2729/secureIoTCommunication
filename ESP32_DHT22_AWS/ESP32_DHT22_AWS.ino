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

// Certificates
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

const char* device_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUZLTrZev6EhzYFFedgXHI8mF9QvcwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTAwNzA2Mzcy
NloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANBjYdYrrXzNDJaKy0cl
qyEDj3z1+fj4CicNud/c8SIjflKVmHdLafOKPO/ir+KbTJDOFXNU2VkA8TNz1gwi
LEzmhYI7gQRYgQHQICCGfQ8iE+q3a2O3hv3uv8k+2A41JpjXzDMNazq0fO28krjw
4tgVt5f0Z1Tv1pWIgKUKofgjehaWVK0CcEfs/aGuI17rIyq69XfwEfCNoBZvJxag
LJchRkBtogJ+1oTjGcYXwq0g+Wm46U8t0SOsFKJb5YSvLE+/9MMzhVx82+3ZjQfZ
5JXkUIqbUsIVYXBefWpSYq4LczhAR7mFaLU1cUtlh2MXpljoOM9pupSzUobnEcph
mo0CAwEAAaNgMF4wHwYDVR0jBBgwFoAUNlZ/v7ayj7kd/UNRzIsf/P68AuEwHQYD
VR0OBBYEFDb2pdHqc3EXhbxy4V8sx1H0r3jCMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBaLbkXcFn1LJEPqu9i0rF13S1K
xfPaRozt/A5kMub+nt8a0KAer7X7oVPMZJzSNVX3PJE+Xm1/+z2kIoXnBHnzZ9Dp
1MQw8pARSbQIFQTRa1baSMrMHDD2wlVrPLpXNYRjk6esRjnuUZnGcpDQYaEs5DE3
YKSurIviepeGG6vMAUaqA7s/lZR/zwrk6Bys+ccXztNNHYwsVtiWEred6gSZFNyT
1Lll56w/kMkvBpNd4DAUc5Xl1YZSEG/zZ/VmjLPPdaYXs+B2dra0yIyETivk9lrO
qyybDXM021lNoAl4cZM3F6cpTD4TAjqn8KHJX1y6Zn4ytPTacqVGndHKP2eR
-----END CERTIFICATE-----
)EOF";

const char* private_key = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEA0GNh1iutfM0MlorLRyWrIQOPfPX5+PgKJw2539zxIiN+UpWY
d0tp84o87+Kv4ptMkM4Vc1TZWQDxM3PWDCIsTOaFgjuBBFiBAdAgIIZ9DyIT6rdr
Y7eG/e6/yT7YDjUmmNfMMw1rOrR87bySuPDi2BW3l/RnVO/WlYiApQqh+CN6FpZU
rQJwR+z9oa4jXusjKrr1d/AR8I2gFm8nFqAslyFGQG2iAn7WhOMZxhfCrSD5abjp
Ty3RI6wUolvlhK8sT7/0wzOFXHzb7dmNB9nkleRQiptSwhVhcF59alJirgtzOEBH
uYVotTVxS2WHYxemWOg4z2m6lLNShucRymGajQIDAQABAoIBACRL2yOpsesV8AZc
oHGA/yCd9SUn1uVllqccve5fFmUC16LcyZSlvwubXWfPDSGkOR7TtmrDMaROBLUY
jQTKAvtV/5UidiNiov3E2YOwySGUpvFwBzV0JwAd0Kvl1U4MNe/TTo7p3G56N3RV
Lx0lX+2RQswl1sW1jKO5Bec5ReLpQkk04saBDiTBGfXdsodaMLgnNtZUByE8FTz9
Vl+DizDj6yUPdkXX1Dcp0qH9e+HrUzV80eofbNX2xPMi92U8Hbxh9if/Y3ucQYRx
0ZccLfovKBWJvaozLWcM7Rccfdhz8M7bPKoOcw5yRZ7MS6+CN9og3JAiyUSW6UrP
fmZMSwUCgYEA+f6MQbfw02GEQ/QekYX+UJMs9xCrdvgtU6xbb7dp8maVtTEoY6Oa
yyOT96VO7pU1zIYaBzrIzXKDq1BtbG/e0FSoXKJnXdEUKKNAcV41AcaiaJSzg8FQ
EV2La0KacUEYpcO9NjrByPovGMv1IwVbyx1PxLmrZ7H6T/vi4wguFV8CgYEA1WT1
duGyLEzZM4XInT0trcg+jxC/wxumAyUaKsKhhd6pwImMnsDklNVGwBVBLDLZL7JZ
QwgB6X0uiSsAM1R4vTYl9SeaW0hncQzCpDKWFRq4Vcpm2dYebDa+nVs967VBt01a
pNThPzspxz0rk4ijW8CGS3hprTYwyxDXQeSny5MCgYEA2FuL6YiXHWolPWZ2Wj2c
JwHpBX5g1xrcp7DghaQ4RjiwUltT+D/sxOhqtAZWdLegEvzwY9dlWCFGgSqfORzs
umK6P0myqgg8KRt4t2Tv/TR69IXVgy7367+I3PCMl33eJgBsrhVWB2k2/3/tDT0i
/3vDwJ9sD9eBql12NEStFYMCgYEAvWqhTEYofpQ0VDyTh6cvbcuKZDmMziYzpSUA
5iXfdFHiQkqWLLCx9b6ez4/OSupTyLe57fskn1oDvbQSuH1psyJmbQcbR52sXDfk
ahWRDOir6VMBGqmqVYn+hvTfsOMykv+xzxA7ZIIion0Uuh+WSbJKQqF+xEaO5yFq
wU2y98sCgYEAuJpzTCkQay2WFy6SVr13JyZ7AFrM6Mp51qYFlm0nC9b6ETSZ6r6w
zIgX9jmmQDjQwCTov7t15NF4ZdbwCh6akAh/a7OZIrJb5/QmX9oBqxbwLbZ++0kW
kbkzIVcvJsQcGrVgkubqXKcgbJHCv2DHGTjDYrYfK8fSbQ4Y8btWhOY=
-----END RSA PRIVATE KEY-----
)EOF";

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
