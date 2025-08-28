/* Analog Read to LED
 * ------------------ 
 *
 * turns on and off a light emitting diode(LED) connected to digital  
 * pin 13. The amount of time the LED will be on and off depends on
 * the value obtained by analogRead(). In the easiest case we connect
 * a potentiometer to analog pin 2.
 *
 * Created 1 December 2005
 * copyleft 2005 DojoDave <http://www.0j0.org>
 * http://arduino.berlios.de
 *
 */
#include <Wire.h>
#include "Adafruit_DRV2605.h"

int maxPot = 1023; //Use if potentiometer is connected to 5V pin
//int maxPot = 666;  //Use if potentiometer is connected to 3.3V pin
Adafruit_DRV2605 drv;

uint8_t effect = 1;

int pot1Pin = A0;    // select the input pin for the potentiometer of Effects
int pot2Pin = A1;   // select the input pin for the potentiometer of Volume

int ledPin = 13;   // select the pin for the LED
int val = 0;       // variable to store the value coming from the Potentiometer 1
int vol = 0;      // variable to store the value coming from the Potentiometer 2
int old_val = -1;
int old_vol = -1;
int curvalue = -1;
int curvol = -1;
uint8_t rtp_index = 0;
uint8_t rtp_size;



const uint8_t CLICK_CHANGE[] = {
  0, 0, 0, 0,
  0, 0, 0, 0, 
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0, 
  0, 0, 0, 0,
   0, 0, 0, 0,
  0, 0, 0, 0, 
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  10, 1, 0, 0, 
  0, 0, 0, 0,
   0, 0, 0, 0,
  0, 0, 0, 0, 
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0, 
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0, 
  0, 0, 0, 0,
};


const uint8_t EFFECT_RANDOM_VIBRATION[] = {
  21, 10, 0, 10,
  21, 10, 10, 10,
  21, 10, 0, 10,
  21, 10, 10, 10,
  21, 10, 21, 10,
  21, 10, 21, 10, 
  0, 10, 0, 10,
  21, 10, 0, 10,
  21, 10, 10, 10,
  21, 10, 0, 10, 
  21, 10, 10, 10,
  21, 10, 0, 10,
  21, 10, 21, 10,
  0, 10, 0, 10,
  21, 10, 0, 10,
  21, 10, 0, 10, 
  21, 10, 10, 10,
  21, 10, 0, 10,
  21, 10, 21, 10,
  21, 10, 21, 10, 
  0, 10, 0, 10, 
  21, 10, 0, 10,
  21, 10, 10, 10,
  10, 10, 5, 10,
  18, 10, 2, 10,
};

const uint8_t EFFECT_PULSE_ECHO[] = {
  21, 10, 0, 10,
  0, 10, 0, 10, 
  21, 10, 0, 10,
  0, 10, 0, 10,
  21, 10, 0, 10,
  0, 10, 0, 10, 
  18, 10, 0, 10,
  0, 10, 0, 10,
  10, 10, 0, 10, 
  0, 10, 0, 10,
  10, 10, 0, 10, 
  0, 10, 0, 10, 
  6, 10, 0, 10, 
  0, 10, 0, 10, 
  6, 10, 0, 10, 
  0, 10, 0, 10, 
  5, 10, 0, 10,
  0, 10, 0, 10, 
  4, 10, 0, 10,
  0, 10, 0, 10, 
  3, 10, 0, 10,
  0, 10, 0, 10,
  10, 10, 0, 10, 
  0, 10, 0, 10,
  10, 10, 0, 10, 
};

const uint8_t EFFECT_PULSE_PURR2[] = {
  21, 10, 10, 10,
  21, 10, 10, 10, 
  21, 10, 10, 10,
  21, 10, 10, 10,
  21, 10, 10, 10,
  20, 10, 10, 10, 
  18, 10, 9, 10,
  15, 10, 8, 10,
  10, 10, 5, 10, 
  10, 10, 5, 10,
  10, 10, 5, 10, 
  9, 10, 3, 10, 
  6, 10, 2, 10, 
  6, 10, 2, 10, 
  6, 10, 2, 10, 
  5, 10, 1, 10, 
  5, 10, 1, 10,
  4, 10, 0, 10, 
  4, 10, 0, 10,
  3, 10, 0, 10, 
  3, 10, 0, 10,
  2, 10, 0, 10, 
  2, 10, 0, 10,
  2, 10, 0, 10, 
  1, 10, 0, 1000,
};

const uint8_t EFFECT_PULSE_PURR[] = {
  21, 10, 0, 10,
  21, 10, 0, 10, 
  21, 10, 0, 10,
  21, 10, 0, 10,
  21, 10, 0, 10,
  20, 10, 0, 10, 
  18, 10, 0, 10,
  15, 10, 0, 10,
  10, 10, 0, 10, 
  10, 10, 0, 10,
  10, 10, 0, 10, 
  9, 10, 0, 10, 
  6, 10, 0, 10, 
  6, 10, 0, 10, 
  6, 10, 0, 10, 
  5, 10, 0, 10, 
  5, 10, 0, 10,
  4, 10, 0, 10, 
  4, 10, 0, 10,
  3, 10, 0, 10, 
  3, 10, 0, 10,
  2, 10, 0, 10, 
  2, 10, 0, 10,
  2, 10, 0, 10, 
  1, 10, 0, 1000,
};

const uint8_t EFFECT_PULSE_INV_PURR[] = {
  1, 10, 0, 10,
  2, 10, 0, 10, 
  2, 10, 0, 10,
  2, 10, 0, 10,
  3, 10, 0, 10, 
  3, 10, 0, 10,
  4, 10, 0, 10, 
  4, 10, 0, 10,
  5, 10, 0, 10, 
  5, 10, 0, 10,
  6, 10, 0, 10, 
  6, 10, 0, 10, 
  6, 10, 0, 10,
  9, 10, 0, 10,
  10, 10, 0, 10, 
  10, 10, 0, 10,
  10, 10, 0, 10,
  15, 10, 0, 10,
  18, 10, 0, 10,
  20, 10, 0, 10,
  21, 10, 0, 10,
  21, 10, 0, 10, 
  21, 10, 0, 10,
  21, 10, 0, 10,
  21, 10, 0, 1000,
};

const uint8_t EFFECT_PULSE_INV[] = {
  1, 1000, 1, 10,
  2, 10, 2, 10, 
  2, 10, 2, 10,
  2, 10, 2, 10,
  3, 10, 3, 10, 
  3, 10, 3, 10,
  4, 10, 4, 10, 
  4, 10, 4, 10,
  5, 10, 5, 10, 
  5, 10, 5, 10,
  6, 10, 6, 10, 
  6, 10, 6, 10, 
  6, 10, 6, 10,
  9, 10, 9, 10,
  10, 10, 10, 10, 
  10, 10, 10, 10,
  10, 10, 10, 10,
  15, 10, 15, 10,
  18, 10, 18, 10,
  20, 10, 20, 10,
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10,
};

const uint8_t EFFECT_PULSE[] = {
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10,
  20, 10, 20, 10, 
  18, 10, 18, 10,
  15, 10, 15, 10,
  10, 10, 10, 10, 
  10, 10, 10, 10,
  10, 10, 10, 10, 
  9, 10, 9, 10, 
  6, 10, 6, 10, 
  6, 10, 6, 10, 
  6, 10, 6, 10, 
  5, 10, 5, 10, 
  5, 10, 5, 10,
  4, 10, 4, 10, 
  4, 10, 4, 10,
  3, 10, 3, 10, 
  3, 10, 3, 10,
  2, 10, 2, 10, 
  2, 10, 2, 10,
  2, 10, 2, 10, 
  1, 10, 1, 1000,
};

const uint8_t EFFECT_RAMP_UP[] = {
  0, 10, 0, 10, 
  1, 10, 1, 10, 
  2, 10, 2, 10, 
  3, 10, 3, 10, 
  4, 10, 4, 10, 
  5, 10, 5, 10, 
  6, 10, 6, 10, 
  7, 10, 7, 10,
  8, 10, 8, 10, 
  9, 10, 9, 10,
  10, 10, 10, 10, 
  11, 10, 11, 10,
  12, 10, 12, 10, 
  13, 10, 13, 10,
  14, 10, 14, 10, 
  15, 10, 15, 10,
   16, 10, 16, 10, 
  17, 10, 17, 10,
   18, 10, 18, 10, 
  19, 10, 19, 10,
   20, 10, 20, 10, 
  20, 10, 20, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10
};

const uint8_t EFFECT_TWO_PULSE[] = {
  21, 10, 1, 10,
  21, 10, 1, 10, 
  21, 10, 1, 10,
  21, 10, 1, 10,
  21, 10, 1, 10,
  21, 10, 1, 10, 
  21, 10, 1, 10,
  21, 10, 1, 10,
  21, 10, 1, 10, 
  21, 10, 1, 10,
  21, 10, 1, 10, 
  21, 10, 1, 10, 
  1, 10, 1, 10, 
  1, 10, 1, 10, 
  1, 10, 1, 10, 
  1, 10, 1, 10, 
  1, 10, 1, 10,
  1, 10, 1, 10, 
  1, 10, 1, 10,
  1, 10, 1, 10, 
  1, 10, 1, 10,
  1, 10, 1, 10, 
  1, 10, 1, 10,
  1, 10, 1, 10, 
  1, 10, 1, 10,
};

const uint8_t EFFECT_STRONG_BUZZ[] = {
  0, 10, 0, 10,
  0, 10, 0, 10, 
  0, 10, 0, 10,
  0, 10, 0, 10,
  0, 10, 0, 10,
  3, 10, 3, 10, 
  3, 10, 3, 10,
  3, 10, 3, 10,
  6, 10, 6, 10, 
  6, 10, 6, 10,
  6, 10, 6, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  6, 10, 6, 10, 
  6, 10, 6, 10,
  6, 10, 6, 10, 
  3, 10, 3, 10,
  3, 10, 3, 10, 
  3, 10, 3, 10,
  0, 10, 0, 10, 
  0, 10, 0, 10,
  0, 10, 0, 10, 
  0, 10, 0, 10,
};

const uint8_t EFFECT_CONST_VIBE[] = {
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
};

const uint8_t BLUES_RYTHM[] = {
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
  21, 10, 21, 10, 
  21, 10, 21, 10,
};

const int effects_size= 10;

uint8_t *effects[effects_size]={BLUES_RYTHM, //0
                                EFFECT_RAMP_UP, //1
                                EFFECT_STRONG_BUZZ, //2
                                EFFECT_TWO_PULSE, //3
                                EFFECT_PULSE_INV_PURR, //4
                                EFFECT_PULSE_PURR, //5
                                EFFECT_PULSE_PURR2, //6
                                EFFECT_PULSE_ECHO, //7
                                EFFECT_RANDOM_VIBRATION, //8 
                                EFFECT_CONST_VIBE}; //9


uint8_t rtp_current[100]={0};
uint8_t *rtp_original;

bool effectChanged;

void changeVolume(uint8_t value){
  bool odd = true;
  for (int i=0; i < rtp_size; i++){
    
     if(odd) {
      if(rtp_original[i] != 0){ //not a gap
           rtp_current[i] = rtp_original[i] * value;
//          Serial.print("current_value:");
//          Serial.println(rtp_current[i]);
      }
      odd = false;
     }
     else{
//      Serial.print("current_delay:");
//      Serial.println(rtp_current[i]);
      odd = true;
     }
   }
}

void changeEffect(int value){

  if(value < effects_size){
    rtp_original = effects[value];
    memcpy(rtp_current, effects[value], 100);

    changeVolume(old_vol); //debug

    effectChanged = true;
  }
}

void playClick()
{
  if (rtp_index < sizeof(rtp_current)/sizeof(rtp_current[0])) {
    drv.setRealtimeValue(CLICK_CHANGE[rtp_index]);
    rtp_index++;
    delay(CLICK_CHANGE[rtp_index]);
    rtp_index++;
  } 
  else {
    drv.setRealtimeValue(0x00);
    //delay(10);
    rtp_index = 0;
    effectChanged = false;
    Serial.println("Click finished");

  }
}

void playEffect()
{
 if (rtp_index < sizeof(rtp_current)/sizeof(rtp_current[0])) {
    drv.setRealtimeValue(rtp_current[rtp_index]);
    rtp_index++;
    delay(rtp_current[rtp_index]);
    rtp_index++;
  } 
  else {
    drv.setRealtimeValue(0x00);
    //delay(300);
    rtp_index = 0;
  }
}


void setup() {
  rtp_size = sizeof(rtp_current);
  //effects_size = sizeof(effects);
  
  pinMode(ledPin, OUTPUT);  // declare the ledPin as an OUTPUT
  Serial.begin(9600);
  Serial.println("DRV and Potentiometer");
  drv.begin();
  drv.useLRA(); //comment if using ERM
  
  // Set Real-Time Playback mode
  drv.setMode(DRV2605_MODE_REALTIME);
}


void loop() {
  val = analogRead(pot2Pin);    // read the value from the sensor
  vol = analogRead(pot1Pin);
  


  
  //Serial.println(val);
  val = map(val, 0, maxPot, 0, effects_size);
  if(val > effects_size-1) val = effects_size-1;
  
  if((val + 1) <= curvalue || (val - 1) >= curvalue){
    if(val != old_val){

      
      
      Serial.print("Potentiometer effect:");
      Serial.println(val);
      
      changeEffect(val);
      
      old_val = val;
    }
    curvalue = val;
  }


  
  vol = constrain(map(vol, 0, maxPot, 0, 6), 0, 6);
  
  if((vol + 1) <= curvol || (vol - 1) >= curvol){
    if(vol != old_vol){
      Serial.print("Potentiometer volume:");
      Serial.println(vol);
      changeVolume(vol);
      old_vol = vol;
    }
    
    curvol = vol;
  }
  
//  if(effectChanged)
//    playClick();
//  else
    playEffect();

}
