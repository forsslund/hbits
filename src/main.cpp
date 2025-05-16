/*#include <Arduino.h>

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(100);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(100);                      // wait for a second
}
*/
#include <Arduino.h>
#include <Control_Surface.h>
#include <MIDI_Interfaces/BluetoothMIDI_Interface.hpp> 

BluetoothMIDI_Interface midi;

NoteLED led { LED_BUILTIN, MIDI_Notes::D[4] };

void setup() {
  delay(1000); 
  Control_Surface.begin();
}

void loop() {
  Control_Surface.loop();
}

