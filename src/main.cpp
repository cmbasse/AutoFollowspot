#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <DW1000Ranging.h>

const char* ssid = "CB";
const char* password = "5w1mm!ng";
const char* mqtt_server = "192.168.30.105";
String clientId = "Anchor01-";
const uint8_t PIN_SCK = 18;
const uint8_t PIN_MOSI = 23;
const uint8_t PIN_MISO = 19;
const uint8_t PIN_SS = 2;
const uint8_t PIN_RST = 15;
const uint8_t PIN_IRQ = 17;
char anchorID[90] = "AA:03:AA:AA:AA:AA:AA:AA";
const char rangeTopic[15] = "Anchor01/range";
WiFiClient espClient;
PubSubClient client(espClient);

void newRange();
void newBlink(DW1000Device* device);
void inactiveDevice(DW1000Device* device);

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_dw1000(){
  delay(1000);
  //init the configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin

  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachBlinkDevice(newBlink);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  //Enable the filter to smooth the distance
  DW1000Ranging.useRangeFilter(true);
  //DW1000Ranging.setRangeFilterValue(5); // newVal >= 2 default 15


  //we start the module as an anchor
  DW1000Ranging.startAsAnchor(anchorID, DW1000.MODE_LONGDATA_RANGE_ACCURACY);
  DW1000.getPrintableExtendedUniqueIdentifier(anchorID);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Publish an announcement

      char __clientId[clientId.length() + 1];
      clientId.toCharArray(__clientId, sizeof(__clientId));
      client.publish("whosIn", __clientId);

      // resubscribe
      client.subscribe("test");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_dw1000();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  DW1000Ranging.loop();
}

void newRange() {
  unsigned long timestamp = millis();
  unsigned int tagID = DW1000Ranging.getDistantDevice()->getShortAddress();
  float tagRange = DW1000Ranging.getDistantDevice()->getRange();
  float tagPower = DW1000Ranging.getDistantDevice()->getRXPower();

  // send to UDP
  String msgUdp = String(anchorID) + "," + String(tagID) + "," + tagRange + "," + tagPower;
  //Serial.println(msgUdp);

  char __msgUdp[msgUdp.length() + 1];
  msgUdp.toCharArray(__msgUdp, sizeof(__msgUdp));

  client.publish(rangeTopic, __msgUdp);
}

void newBlink(DW1000Device* device) {
  Serial.print("blink; 1 device added ! -> ");
  Serial.print(" short:");
  Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);
}
