#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define P_BECKEN D8
#define P_REFILL D2
#define P_PUMP D4

const char* WIFI_SSID = "MyWiFi";
const char* WIFI_PASSWORD = "great-WPA-passphrase";

ESP8266WebServer server(80);
unsigned int timerPump = 0;
unsigned long timerUptime = 0;
bool refillLeer = false;

void wifi_init() {
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.hostname("aquafill");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());
}

bool checkBeckenVoll() {
  if(digitalRead(P_BECKEN) == LOW)
    return false;
  else
    return true;
}

bool checkRefillLeer() {
  if(digitalRead(P_REFILL) == LOW) {
    refillLeer = true;
    return true;
  }
  else {
    refillLeer = false;
    return false;
  }
}

unsigned long CalculateUptimeSeconds(void) {
  static unsigned int  _rolloverCount   = 0;    // Number of 0xFFFFFFFF rollover we had in millis()
  static unsigned long _lastMillis      = 0;    // Value of the last millis()

  unsigned long currentMilliSeconds = millis();

  if (currentMilliSeconds < _lastMillis) {
    _rolloverCount++;
  }

  _lastMillis = currentMilliSeconds;

  return (0xFFFFFFFF / 1000 ) * _rolloverCount + (_lastMillis / 1000);
}

void setup() {
  Serial.begin(9600);
  wifi_init();
  Serial.println();
  pinMode(P_PUMP, OUTPUT);
  pinMode(P_BECKEN, INPUT);
  pinMode(P_REFILL, INPUT);
  server.on("/", sendMetrics);
  server.begin();
}

String buildMetric(String name, String value, String help, String type){
  String ret = "# HELP " + name + " " + help + "\n# TYPE " + name + " " + type + "\n" + name + " " + value + "\n\n";
  return ret;
}

void sendMetrics() {
  int container, tank;
  if(refillLeer)
    container=0;
  else
    container = 1;

  if(checkBeckenVoll())
    tank = 1;
  else
    tank = 0;

  String body = buildMetric("pmp_time", (String)timerPump, "Num of seconds pump is running since boot.", "counter");
  body += buildMetric("refill_status", (String)container, "Status of the refill container.", "gauge");
  body += buildMetric("tank_status", (String)tank, "Status of the main tank.", "gauge");
  body += buildMetric("uptime", (String)timerUptime, "System Uptime in seconds.", "counter");
  server.send(200, "text/plain", body);
}

void runPump() {
  while(!checkBeckenVoll()) {
    if(!checkRefillLeer()) {
      digitalWrite(P_PUMP, HIGH);
      delay(1000);
      timerPump++;
    } else {
      digitalWrite(P_PUMP, LOW);
    }
    commonTasks();
  }
  digitalWrite(P_PUMP, LOW);
}

void commonTasks() {
  timerUptime = CalculateUptimeSeconds();
  checkRefillLeer();
  server.handleClient();
  yield();
}

void loop() {
  if(!checkBeckenVoll())
    runPump();
  commonTasks();
  delay(20);
}
