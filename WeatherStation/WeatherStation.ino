#include <LiquidCrystal.h>
#include <TimerOne.h>
int lightSensorPin = A6;   // Pin for light sensor
int pin2 = 2; // Only pins 2 and 3 usable for ISR TODO
// TODO: Change pin 2/3 as they are needed for ISR
// RS pin = Arduino digital 2
// Enable pin = Arduino digital 3
// d4-d7 = Arduino digital pins 4-7
const int rs = 2, en = 3, d7 = 4, d6 = 5, d5 = 6, d4 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  Serial.begin(9600);
  //pinMode(lightSensorPin, INPUT);
  pinMode(pin2, INPUT);
  lcd.begin(20, 2);
  lcd.setCursor(0, 0);
  createFinCharacters();
  //attachInterrupt(digitalPinToInterrupt(pin2), pin_ISR, FALLING); // Pin 2, Routine: pin_ISR, falling Edge
}

void loop() {
  //Timer1.initialize(1000000) // 1 second (1 000 000 uS)
  //Timer1.attachInterrupt(timer_routine)
  double lightLevelVoltage = (analogRead(lightSensorPin) / 1024.0)*5.0; // 0-5V, 5V on 100% valoisuus
  int lightLevelPercentage = (lightLevelVoltage / 5)*100; // En tiedä tarvitaanko
  String voltageString = "JZNNITE ";
  
  voltageString.replace('Z', byte(1));
  lcd.print((String)voltageString + String(lightLevelVoltage) + String("V"));
  lcd.setCursor(0, 1);
  lcd.print(String("VALOISUUS ") + String(lightLevelPercentage) + String("%"));
  lcd.setCursor(0, 0);


  Serial.println(analogRead(lightSensorPin));
  Serial.print(lightLevelVoltage);
  Serial.println("V");
  Serial.print(lightLevelPercentage);
  Serial.println("% valoisuus");
  delay(300);
}

/*
void pin_ISR // Interrupt service routine
{
 // TODO
}
void timer_routine
{
  // TODO
}
*/

void createFinCharacters()
{
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

