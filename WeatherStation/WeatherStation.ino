
int sensorPin = A0;   // PIN, TBD

void setup() {
  pinMode(sensorPin, INPUT); // TODO
  Serial.begin(9600);
}

void loop() {
  int lightLevelVoltage = (analogRead(sensorPin) / 1024)*5; // 0-5V, 5V on 100% valoisuus
  int lightLevelPergentage = (lightLevelVoltage / 5)*100; // En tiedä tarvitaanko
  Serial.print(lightLevelVoltage);
  Serial.println("V");
  delay(1000);


}
