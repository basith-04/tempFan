#include <ESP8266WiFi.h>

namespace {

constexpr char kWifiSsid[] = "hello";  // Open network: no password.
constexpr char kServerHost[] = "fansss.eu1.netbird.services";
constexpr uint16_t kServerPort = 43240;
constexpr uint32_t kWifiRetryMs = 10000;
constexpr uint32_t kTcpRetryMs = 5000;
constexpr uint32_t kHeartbeatMs = 15000;
constexpr uint32_t kBareCommandIdleMs = 100;

WiFiClient server;
uint32_t lastWifiAttempt = 0;
uint32_t lastTcpAttempt = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastReceivedByte = 0;
bool wifiAttempted = false;
bool tcpAttempted = false;
uint32_t tcpConnectCount = 0;
uint32_t pingSendCount = 0;
uint32_t commandCount = 0;
char word[12];
uint8_t wordLength = 0;
bool wordOverflow = false;

void forwardWord() {
  if (!wordOverflow && wordLength != 0) {
    word[wordLength] = '\0';
    if (strcmp(word, "start") == 0) {
      Serial.print("start\n");
      ++commandCount;
    } else if (strcmp(word, "stop") == 0) {
      Serial.print("stop\n");
      ++commandCount;
    }
  }
  wordLength = 0;
  wordOverflow = false;
}

void receiveByte(char c, uint32_t now) {
  lastReceivedByte = now;
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
    if (wordLength < sizeof(word) - 1) {
      word[wordLength++] = (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
    } else {
      wordOverflow = true;
    }
  } else {
    forwardWord();
  }
}

void maintainWifi(uint32_t now) {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  server.stop();
  forwardWord();
  if (!wifiAttempted || now - lastWifiAttempt >= kWifiRetryMs) {
    wifiAttempted = true;
    lastWifiAttempt = now;
    WiFi.begin(kWifiSsid);
  }
}

void maintainServer(uint32_t now) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  while (server.available() > 0) {
    receiveByte(static_cast<char>(server.read()), millis());
  }
  if (wordLength != 0 && now - lastReceivedByte >= kBareCommandIdleMs) {
    forwardWord();
  }

  if (!server.connected()) {
    forwardWord();
    server.stop();
    if (!tcpAttempted || now - lastTcpAttempt >= kTcpRetryMs) {
      tcpAttempted = true;
      lastTcpAttempt = now;
      if (server.connect(kServerHost, kServerPort)) {
        ++tcpConnectCount;
        server.setNoDelay(true);
        if (server.write(reinterpret_cast<const uint8_t *>("ping"), 4) == 4) {
          ++pingSendCount;
        }
        lastHeartbeat = millis();
      }
    }
    return;
  }

  if (now - lastHeartbeat >= kHeartbeatMs) {
    if (server.write(reinterpret_cast<const uint8_t *>("ping"), 4) == 4) {
      ++pingSendCount;
    } else {
      server.stop();
    }
    lastHeartbeat = now;
  }
}

void processStatusQuery() {
  while (Serial.available() > 0) {
    const int request = Serial.read();
    if (request == '?') {
      Serial.print("#wifi=");
      Serial.print(WiFi.status() == WL_CONNECTED ? 1 : 0);
      Serial.print(",wifi_code=");
      Serial.print(static_cast<int>(WiFi.status()));
      Serial.print(",ip=");
      Serial.print(WiFi.localIP());
      Serial.print(",tcp=");
      Serial.print(server.connected() ? 1 : 0);
      Serial.print(",connects=");
      Serial.print(tcpConnectCount);
      Serial.print(",pings=");
      Serial.print(pingSendCount);
      Serial.print(",commands=");
      Serial.println(commandCount);
    } else if (request == 's') {
      const int found = WiFi.scanNetworks();
      Serial.print("#scan_count=");
      Serial.print(found);
      Serial.print(",hello=");
      bool helloFound = false;
      for (int i = 0; i < found; ++i) {
        if (WiFi.SSID(i) == kWifiSsid) {
          helloFound = true;
        }
      }
      Serial.println(helloFound ? 1 : 0);
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(kWifiSsid);
  wifiAttempted = true;
  lastWifiAttempt = millis();
}

void loop() {
  const uint32_t now = millis();
  maintainWifi(now);
  maintainServer(now);
  processStatusQuery();
  delay(1);
}
