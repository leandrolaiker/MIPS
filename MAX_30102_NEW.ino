
#define USEFIFO 1
#define DEBUG_FLEXIPLOT 1
#define DEBUG 1
#define DEBUGSPO2 1
#define DEBUGCRT 1
#define DEBUGEBPM 1


#include <Arduino.h>
#include <U8x8lib.h>
#include <SPI.h>
#include <Wire.h>
#include "MAX30105.h" //sparkfun MAX3010X library
MAX30105 particleSensor;
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

//if you have Sparkfun's MAX30105 breakout board
#define MAX30105

/****************** heart rate *********************/
static double fbpmrate = 0.95; // low pass filter coefficient for HRM in bpm
static uint32_t crosstime = 0; //falling edge , zero crossing time in msec
static uint32_t crosstime_prev = 0;//previous falling edge , zero crossing time in msec
static double bpm = 40.0;
static double ebpm = 40.0;
static double eir = 0.0; //estimated lowpass filtered IR signal to find falling edge without notch
static double firrate = 0.85; //IR filter coefficient to remove notch ,should be smaller than frate
static double eir_prev = 0.0;

#define TIMETOBOOT 3000 // wait for this time(msec) to output SpO2
#define SCALE 88.0 //adjust to display heart beat and SpO2 in the same scale
#define SAMPLING 1 //if you want to see heart beat more precisely , set SAMPLING to 1
#define FINGER_ON_RED 30000 // if red signal is lower than this , it indicates your finger is not on the sensor
#define MINIMUM_SPO2 80.0

#define FINGER_ON_IR 50000 // if ir signal is lower than this , it indicates your finger is not on the sensor
#define LED_PERIOD 100 // light up LED for this period in msec when zero crossing is found for filtered IR signal
#define MAX_BPS 180
#define MIN_BPS 45

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup(){
  boolean success;
  Serial.begin(115200);
  Serial.println("Initializing...");
  Serial.println();

  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.clear();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(1,1, "PulseOxi_Meter");
  u8x8.drawString(1,4, "HR  :      BPM");
  u8x8.drawString(1,5, "SpO2:       %");
  

  // Initialize MAX3010x sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30102 was not found. Please check wiring/power/solder jumper at MH-ET LIVE MAX30102 board. ");
    while (1);
  }

  //Setup MAX30102
  byte ledBrightness = 0x1F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  //Options: 1 = IR only, 2 = Red + IR on MH-ET LIVE MAX30102 board
  int sampleRate = 400; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384
  // Set up the wanted parameters
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
}

double avered = 0; double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100; //calculate SpO2 by this sampling interval

double ESpO2 = 95.0;//initial value of estimated SpO2
double FSpO2 = 0.7; //filter factor for estimated SpO2
double frate = 0.95; // .95 default low pass filter for IR/red LED value to eliminate AC component

void loop(){
  uint32_t ir, red , green;
  double fred, fir;
  double SpO2 = 0; //raw SpO2 before low pass filtered
  int idx=0;

  #ifdef USEFIFO
    particleSensor.check(); //Check the sensor, read up to 3 samples
    int Ebpm = 60;
    while (1) {//do we have new data

  #ifdef MAX30105
      red = particleSensor.getRed();  //Sparkfun's MAX30105
      ir = particleSensor.getIR();  //Sparkfun's MAX30105
  #else
      red = particleSensor.getRed(); //why getFOFOIR outputs Red data by MAX30102 on MH-ET LIVE breakout board
      ir = particleSensor.getIR(); //why getFIFORed outputs IR data by MAX30102 on MH-ET LIVE breakout board
  #endif
      i++;
      fred = (double)red; // 
      fir = (double)ir;
      avered = avered * frate + fred * (1.0 - frate);//average red level by low pass filter
      aveir = aveir * frate + fir * (1.0 - frate); //average IR level by low pass filter
      sumredrms += (fred - avered) * (fred - avered); //square sum of alternate component of red level
      sumirrms += (fir - aveir) * (fir - aveir);//square sum of alternate component of IR level
      
  #ifdef DEBUG_FLEXIPLOT
      Serial.print("{P0|RED|255,0,0|");
      Serial.print(red);
      Serial.print("|IR|0,0,255|");
      Serial.print(ir);
      Serial.print("|avered|255,0,0|");
      Serial.print(avered);
      Serial.print("|aveir|0,0,255|");
      Serial.print(aveir);    
      Serial.println("}");    
  #endif

      // True Result of MAX30102
      Ebpm = (int) HRM_estimator(fir, aveir); //Ebpm is estimated BPM
      #ifdef DEBUGEBPM
      Serial.print(" i : ");
      Serial.print(i);
      Serial.print("\t\t|\tEbpm : ");
      Serial.println(Ebpm);
      Serial.print("\t\t|\tESpO2 : ");
      Serial.println(ESpO2);
      displayDataOLED (Ebpm, ESpO2);
      #endif

      if (ir < FINGER_ON_IR)
          ESpO2 = MINIMUM_SPO2; //indicator for finger detached

      if ((i % Num) == 0) { //every Num samples
          double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir); // ratio of (redrms/avered) / (irrms/aveir)
          SpO2 = -23.3 * (R - 0.4) + 100; // SpO2 based on Ratio
          ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2; // SpO2 filtered

          #ifdef DEBUGSPO2        
          Serial.print(" SpO2 : ");
          Serial.print(SpO2);
          Serial.print(" ESpO2 : ");
          Serial.print(ESpO2);
          Serial.print(" R:");
          Serial.println(R);
          #endif  

          //reset sums and indices
          sumredrms = 0.0; 
          sumirrms = 0.0; 
          i = 0;

          //sendHRData(Ebpm, ESpO2);
          //sendSPOData(ESpO2);
          break;
      }
      //particleSensor.nextSample(); //We're finished with this sample so move to next sample
    }
  #endif //USE_FIFO
}

void displayDataOLED ( int Ebpm, double ESpO2) {
  // int red = particleSensor.getRed();
  // int ir = particleSensor.getIR();
  if (Ebpm >40 & ESpO2>80) {
    u8x8.setCursor(8, 4);
    u8x8.print(Ebpm);
    u8x8.setCursor(7, 5);
    u8x8.print(ESpO2);
    u8x8.drawString(1,7, "Measuring....."); 
  }
  else if(particleSensor.getRed() > 5000 && particleSensor.getIR() > 5000){
    u8x8.setCursor(8, 4);
    u8x8.print("__");
    u8x8.setCursor(7, 5);
    u8x8.print("__.__");
    u8x8.drawString(1,7, "Initializing..."); 
  } 
  else if (particleSensor.getRed() < 5000 && particleSensor.getIR() < 5000){
    u8x8.setCursor(8, 4);
    u8x8.print("--");
    u8x8.setCursor(7, 5);
    u8x8.print("--.--");
    u8x8.drawString(1,7, "Invalid Finger!");
  }  
}



double HRM_estimator( double fir , double aveir) {
  int CTdiff;

  //Heart Rate Monitor by finding falling edge
  eir = eir * firrate + fir * (1.0 - firrate); //estimated IR : low pass filtered IR signal

  if ( ((eir - aveir) * (eir_prev - aveir) < 0 ) && ((eir - aveir) < 0.0)) { //find zero cross at falling edge
    crosstime = millis(); //system time in msec of falling edge
    CTdiff = crosstime-crosstime_prev;
    
    #ifdef DEBUGCRT
    Serial.print("CRT: ");
    Serial.print(crosstime);
    Serial.print(" CRT_PREV: ");
    Serial.print(crosstime_prev);
    Serial.print(" CTdiff: ");
    Serial.println(CTdiff);
    #endif

    //if ( ((crosstime - crosstime_prev ) > (60 * 1000 / MAX_BPS)) &&((crosstime - crosstime_prev ) < (60 * 1000 / MIN_BPS)) ) {

    if ( ( CTdiff > 333 ) && ( CTdiff < 1333 ) ) {
      bpm = 60.0 * 1000.0 / (double)(crosstime - crosstime_prev) ; //get bpm
      // Serial.println("crossed");
      ebpm = ebpm * fbpmrate + (1.0 - fbpmrate) * bpm; //estimated bpm by low pass filtered
      
  #ifdef LED_SOUND_INDICATOR
      if (aveir > FINGER_ON) {
        digitalWrite(LEDPORT, HIGH);
        tone(BLIPSOUND-(100.0-ESpO2)*10.0);//when SpO2=80% BLIPSOUND drops 200Hz to indicate anormaly
      }
  #endif
    } else {
      //Serial.println("faild to find falling edge");
    }
    crosstime_prev = crosstime;
  }
   eir_prev = eir;
   return (ebpm);
}

