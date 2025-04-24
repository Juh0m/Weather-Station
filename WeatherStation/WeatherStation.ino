#include <Ethernet.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>
#include <TimerOne.h>

// -- Keypad --
const byte ROWS = 1; // One row
const byte COLS = 4; // Three columns

char keys[ROWS][COLS] = {
  {'1','2','3','A'}
};

byte rowPins[ROWS] = {A5}; // Arduino A4 pin for row
byte colPins[COLS] = {A0, A1, A3, A4}; // Arduino pins A1, A2, A3, A4 for columns
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); 

// -- MQTT --
byte server[] = {10,6,0,23 }; // MQTT- server IP-address
unsigned int port = 1883; // MQTT- server port
EthernetClient ethClient;
void callback(char* topic, byte* payload, unsigned int length);

PubSubClient client(server, port, callback, ethClient); 
// A8 61 0A AE 59 C3
static uint8_t macAddress[6] {0xA8, 0x61, 0x0A, 0xAE, 0x59, 0xC3}; 
// Device ID, client ID and client secret for ethClient connection
char* deviceId = "wysi727";
char* clientId = "a7272727w";
char* deviceSecret = "oq2if";

// Topics
#define outTopic "ICT4_out_2020"
#define outTopicLight "w727_valoisuus"
#define outTopicMoisture "w727_kosteusIn"

int lightSensorPin = A6; // Pin for light sensor (Analog signal)
int isrPin = 2; // Pin for ISR (Digital signal).

// RS pin = Arduino digital 3
// Enable pin = Arduino digital 4
// d4-d7 = Arduino digital pins 5-8

// -- LCD --
const int rs = 3, en = 4, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Function prototypes
void connectMQTTServer();
void fetchIP();
void ISR_R();
void showIP();
void showSignals();
void showSignalMinMax();
void timer_routine();
void updateMinMax();

// For ISR
int puls = 0;
// Digital signal frequency
float frequency;
// Measurements in percentage
int lightLevelPercentage;
int moisturePercentage;

// Min percentages set to above 100% to guarantee the min value is actually the lowest read value
int minMoisturePercentage = 100;
int maxMoisturePercentage = 0;
int minLightLevelPercentage = 100;
int maxLightLevelPercentage = 0;

// Current min/max selection on LCD
String current = "max";

// 10 second averages
float moisture10s;
float lightLevel10s;
// For timer_routine
// Time2 is for 10s average
volatile byte time = 0;
volatile byte time2 = 0;


void setup() {
  // ISR pin
  pinMode(isrPin, INPUT);
  // Interrupt Service Routine for digital signal
  // Every time the signal falls from 5V to 0V ISR_R is called and puls increments by one.
  // Every 2 seconds the frequency of the signal is calculated from puls.
  // According to project requirements, relative humidity of 40% is 7.9kHz and relative humidity of 100% is 6.9kHz
  // A 1% increase in humidity corresponds to a (-1/60)kHz decrease in frequency.
  // 0% humidity would then be 8.567kHz (8.56666...)
  // Humidity can then be calculated using the formula:
  // RH = (0% RHf - frequency) * 60 -> (8567Hz - f)*60

  attachInterrupt(digitalPinToInterrupt(isrPin), ISR_R, FALLING); // Pin 2, Routine: ISR_R, falling Edge

  // Timer for ISR
  // timerRoutine is called once every 1 second (1 000 000 uS).
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerRoutine);

  Serial.begin(9600);
  // LCD begin, 20 positions, 2 rows
  // Set cursor to first position
  lcd.begin(20, 2);
  lcd.setCursor(0, 0);
  // Prints a message on the LCD to indicate the code is running on the arduino
  lcd.print("Arduino ready");

  // Begin ethernet connection and fetch IP
  fetchIP();
  // Connect to broker
  connectMQTTServer();
  // Letter Ä for LCD
  createFinCharacter();
}

void loop() {
  // Pressed key on keypad
  char key = keypad.getKey();
  if (key){
    switch(key)
    {
    case '1':
      showIP();
      break;
    case '2':
      showSignalMinMax();
      break;
    case '3':
      showSignals();
      break;
    case 'A':
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("727");
      break;
    }
  }
}

void ISR_R() // Interrupt service routine
{
  // Increment by one every time voltage falls
  puls++; 
}
void showIP()
{
  // Prints IP and connection status of the ethernet module on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP: ");
  lcd.print(Ethernet.localIP());
  lcd.setCursor(0, 1);
  if(client.connected())
  {
    lcd.print("Connected to broker");
  }
  else
  {
    lcd.print("Not connected to broker");
  }
}
void showSignals()
{
  // Prints the current signal values to the LCD
  // Does not update until the key is pressed again
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ilmankosteus: ");  
  lcd.print(moisturePercentage, 1);  
  lcd.print("%"); 

  lcd.setCursor(0, 1);
  lcd.print("Valoisuus: ");
  lcd.print(lightLevelPercentage);
  lcd.print("%");
}
void showSignalMinMax()
{
  if(current == "min")
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kosteus MAX: ");
    lcd.print(maxMoisturePercentage, 1);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Valoisuus MAX: ");
    lcd.print(maxLightLevelPercentage);
    lcd.print("%");
    current = "max";
  }
  else if(current == "max")
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kosteus MIN: ");
    lcd.print(minMoisturePercentage, 1);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Valoisuus MIN: ");
    lcd.print(minLightLevelPercentage);
    lcd.print("%");
    current = "min";
  }

}
void timerRoutine()
{
  time++;
  // Every 2 seconds calculate frequency and reset time
  if(time > 1)
  {
    time = 0;

    // Light level
    float lightLevelVoltage = (analogRead(lightSensorPin) / 1024.0)*5.0; // 0-5V, 0V is 0% light level, 5V = 100% light level
    lightLevelPercentage = (lightLevelVoltage / 5)*100; // Convert to percentage
    lightLevel10s += lightLevelPercentage; // Add to 10s average
    puls = 0;

    // Moisture (in)
    frequency = (float)puls/2.0; 
    moisturePercentage = ((8567.0/1000.0) - (frequency/1000.0)) * 60.0; // Convert frequency to percentage
    if(moisturePercentage > 100)
    {
      moisturePercentage = 100;
    }
    updateMinMax();
    moisture10s += moisturePercentage; // Add to 10s average
    time2++; // Increment time2 by one
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
  // sprintf(buffer, "IOTJS={\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\",\"%s\":\"%s\"}","S_name1", outTopicLight, "S_value1", lightLevel, "S_name2", outTopicMoisture, "S_value2", moisture);
  // Check connection to MQTT broker
  if (!client.connected()) 
  { 
    connectMQTTServer(); // If not connected, connect
  }
  if (client.connected()) { // If connected

      sprintf(buffer, "IOTJS={\"%s\":\"%s\",\"%s\":\"%s\"}", "S_name", outTopicLight, "S_value", lightLevel);
      client.publish(outTopic, buffer); // Send message to broker
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
void updateMinMax()
{
    // Check if signal value was lower than minimum or higher than maximum
    // The percentages will always be positive and under 100% so no need for additional checks
    if(lightLevelPercentage > maxLightLevelPercentage)
    {
      maxLightLevelPercentage = lightLevelPercentage;
    }
    if(lightLevelPercentage < minLightLevelPercentage)
    {
      minLightLevelPercentage = lightLevelPercentage;
    }

    if(moisturePercentage > maxMoisturePercentage)
    {
      maxMoisturePercentage = moisturePercentage;
    }
    if(moisturePercentage < minMoisturePercentage)
    {
      minMoisturePercentage = moisturePercentage;
    }
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