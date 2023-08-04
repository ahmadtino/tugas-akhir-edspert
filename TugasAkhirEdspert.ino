/* Include library-library yang diperlukan */
#include <PZEM004Tv30.h> // Library sensor daya PZEM004T
#include <LiquidCrystal_I2C.h> // Library untuk Layar LCD
#include <WiFi.h>   // WiFi
#include <PubSubClient.h>   // MQTT

/* Deklarasi konstanta */
#define RELAY_1 4
#define RELAY_2 15
#define LED_INDICATOR 2

/* Inisialisasi MQTT */
WiFiClient espClient;
PubSubClient client(espClient);

/* Inisialisasi sensor PZEM004T */
PZEM004Tv30 pzem(Serial2, 16, 17);

/* Inisialisasi layar LCD 16x2, I2C address 0x27 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* Deklarasi variabel */
const char* ssid = "BLOK I 1-1";
const char* password = "hksangkuriang45b";

const char* mqtt_server = "192.168.1.15";

unsigned long prevTime = 0;
const long delayTime = 4000;

/* Deklarasi variabel-variabel untuk mengambil data dari sensor */
float power;
float energy;
float voltage;
float current;

/* Deklarasi karakter-karakter pada layar LCD */
char power_lcd[14];
char energy_lcd[14];

void setup() {
  /* Inisialisasi serial, serial 2 untuk pzem */
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  /* Inisialisasi LCD */
  lcd.init();
  lcd.backlight();
  
  pinMode(LED_INDICATOR,OUTPUT);
  pinMode(RELAY_1, OUTPUT); 
  pinMode(RELAY_2, OUTPUT);
  delay(1000);

  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  digitalWrite(LED_INDICATOR, LOW);
  lcd.setCursor(0, 0);
  lcd.printstr("CONNECTING...");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printstr("CONNECTED!");
  digitalWrite(LED_INDICATOR, HIGH);
  delay(1000);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(200);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  /* Ambil data dari sensor setiap 4 detik  */
  if (millis() - prevTime > 4000) {
    prevTime = millis();
    voltage = pzem.voltage();
    current = pzem.current();
    power = pzem.power();
    energy = pzem.energy();
    delay(100);
    display_sensor();

    // Convert the value to a char array
    char powerString[8];
    dtostrf(power, 1, 1, powerString);
    client.publish("esp32/edspert/power", powerString);

    // Convert the value to a char array
    char energyString[8];
    dtostrf(energy, 1, 2, energyString);
    client.publish("esp32/edspert/energy", energyString);

    // Convert the value to a char array
    char voltageString[8];
    dtostrf(voltage, 1, 2, voltageString);
    client.publish("esp32/edspert/voltage", voltageString);

    // Convert the value to a char array
    char currentString[8];
    dtostrf(current, 1, 2, currentString);
    client.publish("esp32/edspert/current", currentString);
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  if (String(topic) == "esp32/edspert/homecontrol") {
    if(messageTemp == "on1"){
      digitalWrite(RELAY_1, HIGH);
    }
    else if(messageTemp == "off1"){
      digitalWrite(RELAY_1, LOW);
    }
    else if(messageTemp == "on2"){
      digitalWrite(RELAY_2, HIGH);
    }
    else if(messageTemp == "off2"){
      digitalWrite(RELAY_2, LOW);
    }
  }
}

void display_sensor() {
  sprintf(power_lcd, "P: %.1fW", power);
  sprintf(energy_lcd, "E: %.2fWh", energy);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printstr(power_lcd);
  lcd.setCursor(0, 1);
  lcd.printstr(energy_lcd);
}

void reconnect() {
  while(!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.print("connected");
      client.subscribe("esp32/edspert/homecontrol");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Try again in 5 seconds");
      delay(5000);
    }
  }
}
