#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>
#include <TimerOne.h>

int lightSensorPin = A6;   // Pin for light sensor
int isrPin = 2; // Pin for ISR (Digital signal). Must be either pin 2 or 3 on a nano.

// RS pin = Arduino digital 3
// Enable pin = Arduino digital 4
// d4-d7 = Arduino digital pins 5-8
const int rs = 3, en = 4, d4 = 8, d5 = 7, d6 = 6, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void ISR_D();
void timer_routine();

// For pin_ISR
int puls = 0;
// Digital signal frequency
float frequency;
// For timer_routine
volatile byte time = 0;

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

  // Letter Ä for LCD
  createFinCharacter();
}

void loop() {
  double lightLevelVoltage = (analogRead(lightSensorPin) / 1024.0)*5.0; // 0-5V, 0V is 0% light level, 5V = 100% light level
  int lightLevelPercentage = (lightLevelVoltage / 5)*100; 

  // Print voltage
  // Only used for debug -> can be printed to only serial monitor, no need for lcd print
  // Serial.println(lightLevelVoltage)

  // Print on LCD (row 2)
  lcd.setCursor(0, 1);
  lcd.print("VALOISUUS: ");
  lcd.print(lightLevelPercentage);
  lcd.print("%");

  // 600ms delay on loop() as updates to light level are not needed that often
  // Could be increased to multiple seconds
  delay(600);
}

void ISR_D() // Interrupt service routine
{
  puls++;
}
void timerRoutine()
{
  time++;

  // Every 5 seconds calculate frequency and reset time
  if(time > 2)
  {
    time = 0;
    frequency = (float)puls/3.0;

    Serial.print(puls);
    puls = 0;
    Serial.println("<- puls");
    Serial.print(frequency);
    Serial.println("<- taajuus"); 

// If assumptions about signal are correct, this should work
    float moisturePercentage = ((8567.0/1000.0) - (frequency/1000.0)) * 60.0;
    // Print on LCD (row 1)
    lcd.setCursor(0, 0);
    lcd.print("ILMANKOSTEUS: ");  
    lcd.print(moisturePercentage, 2);  
    lcd.print("%"); 

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

