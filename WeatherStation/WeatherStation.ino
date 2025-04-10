#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>
#include <TimerOne.h>

byte server[] = {10,6,0,23 }; // MQTT- server IP-address
unsigned int port = 1883; // MQTT- server port
EthernetClient ethClient;
void callback(char* topic, byte* payload, unsigned int length);

PubSubClient client(server, port, callback, ethClient); 
// A8 61 0A AE 59 C3
static uint8_t macAddress[6] {0xA8, 0x61, 0x0A, 0xAE, 0x59, 0xC3}; 
// Device ID 
char* deviceId = "wysi727";
// Client ID
char* clientId = "a7272727w"; 
char* deviceSecret = "oq2if";

// Topics
#define outTopic "ICT4_out_2020"
#define outTopicLight "w727_valoisuus"
#define outTopicMoisture "w727_kosteusIn"

int lightSensorPin = A6;   // Pin for light sensor
int isrPin = 2; // Pin for ISR (Digital signal). Must be either pin 2 or 3 on a nano.

// RS pin = Arduino digital 3
// Enable pin = Arduino digital 4
// d4-d7 = Arduino digital pins 5-8
const int rs = 3, en = 4, d4 = 8, d5 = 7, d6 = 6, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);



void ISR_D();
void timer_routine();
void fetchIP();
void connectMQTTServer();

// For pin_ISR
int puls = 0;
// Digital signal frequency
float frequency;
float moisture10s;
float lightLevel10s;
// For timer_routine
volatile byte time = 0;
volatile byte time2 = 0;

void setup() {
  // ISR pin
  pinMode(isrPin, INPUT);
  // Interrupt Service Routine for digital signal
  // Every time the signal falls from 5V to 0V pin_ISR is called and puls increments by one.
  // Every 5 seconds the frequency of the signal is calculated from puls.
  // According to project requirements, relative humidity of 40% is 7.9kHz and relative humidity of 100% is 6.9kHz

  // A 1% increase in humidity corresponds to a (-1/60*100)kHz decrease in frequency.
  // 0% humidity would then be 8.567kHz (8.56666...)
  // Humidity can then be calculated using the formula:
  // RH = (0% RHf - frequency) * 60 -> (8567Hz - f)*60

  attachInterrupt(digitalPinToInterrupt(isrPin), ISR_D, FALLING); // Pin 2, Routine: pin_ISR, falling Edge

  // Timer for ISR
  // timerRoutine is called once every 1 second (1 000 000 uS).
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerRoutine);

  // Begin serial data transmission at 9600 baud (for printing values to the serial monitor)
  Serial.begin(9600);
  // LCD begin, 20 positions, 2 rows (TODO: is 2 rows the max?)
  // Set cursor at first position
  lcd.begin(20, 2);
  lcd.setCursor(0, 0);

  fetchIP();
  connectMQTTServer();
  // Letter Ä for LCD
  createFinCharacter();
}

void loop() {
  // Nothing is done in the loop as everything is done with interrupts
}

void ISR_D() // Interrupt service routine
{
  // Increment by one every time voltage falls
  puls++; 
}
void timerRoutine()
{
  time++;

  // Every 2 seconds calculate frequency and reset time
  if(time > 1)
  {
    time = 0;
    frequency = (float)puls/2.0;

    // Light level
    float lightLevelVoltage = (analogRead(lightSensorPin) / 1024.0)*5.0; // 0-5V, 0V is 0% light level, 5V = 100% light level
    int lightLevelPercentage = (lightLevelVoltage / 5)*100; 
    lightLevel10s += lightLevelPercentage;
    // Print on LCD (row 2)
    lcd.setCursor(0, 1);
    lcd.print("VALOISUUS: ");
    lcd.print(lightLevelPercentage);
    lcd.print("%");

    puls = 0;

    // Moisture (in)
    float moisturePercentage = ((8567.0/1000.0) - (frequency/1000.0)) * 60.0;
    moisture10s += moisturePercentage;
    // Print on LCD (row 1)
    lcd.setCursor(0, 0);
    lcd.print("ILMANKOSTEUS: ");  
    lcd.print(moisturePercentage, 1);  
    lcd.print("%"); 

    time2++;
  }

  // Every 10 seconds calculate average values of that time frame and send them to the broker
  if(time2 > 4)
  {
    moisture10s = moisture10s/5.0;
    lightLevel10s = lightLevel10s/5.0;
    sendMQTTMessage();

    // Reset values
    time2 = 0;
    moisture10s = 0;
    lightLevel10s = 0;
  }

}
void connectMQTTServer()
{
  if(client.connected())
  {
    Serial.println("Already connected.");
    return;
  }
    Serial.println("Connecting to MQTT broker");
    if (client.connect(clientId, deviceId, deviceSecret)) {
        Serial.println("Connected successfully!");
    } else {
        Serial.println("Connection to broker failed."); 
    }    
}
void sendMQTTMessage()
{
  char lightLevel[3];
  char moisture[6];
  dtostrf(moisture10s, 3, 0, moisture);
  dtostrf(lightLevel10s, 2, 0, lightLevel);
  char buffer[100];

  // Does not work currently as the MQTT broker code is not working
  //sprintf(buffer, "IOTJS={\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}","S_name1", outTopicLight, "S_value1", lightLevel, "S_name2", outTopicMoisture, "S_value2", moisture);
  
  if (!client.connected()) { // Check connection to MQTT broker
      connectMQTTServer(); // If not connected, connect
  }
  if (client.connected()) { // If connected
      client.publish(outTopic, buffer); // Send message to broker
      sprintf(buffer, "IOTJS={\"%s\":\"%s\",\"%s\":\"%s\"}", "S_name", outTopicLight, "S_value", lightLevel);
      Serial.println(buffer);
      sprintf(buffer, "IOTJS={\"%s\":\"%s\",\"%s\":\"%s\"}", "S_name", outTopicMoisture, "S_value", moisture);
      client.publish(outTopic, buffer); // Send message to broker
      Serial.println("Messages sent to MQTT broker."); 
      Serial.println(buffer);
  } else {
      Serial.println("Failed to send message: not connected to MQTT broker.");
  }
}
// Fetch IP
void fetchIP()
{
  bool connectionSuccess = Ethernet.begin(macAddress);
  if(!connectionSuccess)
  {
    Serial.println("Failed to fetch IP");
  }
  else
  {
    Serial.println(Ethernet.localIP());
  }
}
// PSC Callback
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.println(topic);
}

void createFinCharacter()
{
  // Letter Ä
  // Usable with byte(1)
  byte CapitalAwithDots[8] = {
  B10001,
  B01110,
  B10001,
  B10001,
  B11111,
  B10001,
  B10001,
  };
  lcd.createChar(1, CapitalAwithDots);
}

