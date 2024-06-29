#include <SPI.h>
#include <Wire.h>
int ledRojo = 3;
int ledVerde = 5;
int ledAzul = 6;
int pulsador = 8;
int pinBuzzer = 9;
const int tonos1[] = {261, 294, 330, 370, 415, 466};
const int tonos2[] = {277, 311, 349, 392, 440, 494};
const int tonos3[] = {261, 277, 311, 330, 370, 392};
const int tonos4[] = {294, 311, 349, 370, 392, 415};
const int tonos5[] = {311, 330, 349, 370, 392, 415};
const int countTonos = 6;
int estado;
int iTono;
int contador;
int UVOUT = A0; //Output from the sensor
int REF_3V3 = A1; //3.3V power on the Arduino board
 
void setup()
{
  Serial.begin(9600);
  pinMode(ledRojo,OUTPUT);
  pinMode(ledVerde,OUTPUT);
  pinMode(ledAzul,OUTPUT);
  pinMode(pulsador,INPUT_PULLUP); 
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);
  Serial.println("ML8511 example");
  
}
 
void loop()
{
  int uvLevel = averageAnalogRead(UVOUT);
  int refLevel = averageAnalogRead(REF_3V3);
  
  //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
  float outputVoltage = 3.3 / refLevel * uvLevel;
  
  float uvIntensity = (13.28 * outputVoltage) - 13.28; //Convert the voltage to a UV intensity level
 
  uvIntensity = discrete (uvIntensity);

  Serial.print("output: ");
  Serial.print(refLevel);
 
  Serial.print(" / ML8511 output: ");
  Serial.print(uvLevel);
 
  Serial.print(" / ML8511 voltage: ");
  Serial.print(outputVoltage);
 
  Serial.print(" / UV Intensity (mW/cm^2): ");
  Serial.print(uvIntensity);
  Serial.println();
}
 
//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 
 
  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;
 
  return(runningValue);
}

int discrete (float UVIML){
  if ((UVIML - (int) UVIML) <= 0,5){
    UVIML = UVIML - (int) UVIML;
  }else{
    UVIML = UVIML + (1 - (int) UVIML);
  }
  return UVIML;
}
 
float RGB (int UVI){
  switch (UVI) {
    case 0:
        RGB (0, 255, 0);
        Serial.print("UV Index level = 0, SAFE");
        break;
    case 1:
        RGB (0, 255, 0);
        Serial.print("UV Index level = 1, SAFE");
        break;
    case 2:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (0, 255, 0);
        Serial.print("UV Index level = 2, SAFE");
        buzzer (&tonos1[0]);
        break;
    case 3:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 255, 0);
        Serial.print("UV Index level = 3, MODERATE");
        buzzer (&tonos2[0]);
        break;
    case 4:
        RGB (255, 255, 0);
        Serial.print("UV Index level = 4, MODERATE");
        break;
    case 5:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 255, 0);
        Serial.print("UV Index level = 5, MODERATE");
        buzzer (&tonos2[0]);
        break;
    case 6:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 165, 0);
        Serial.print("UV Index level = 6, HIGH");
        buzzer (&tonos3[0]);
        break;
    case 7:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 165, 0);
        Serial.print("UV Index level = 7, HIGH");
        buzzer (&tonos3[0]);
        break;
    case 8:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 0, 0);
        Serial.print("UV Index level = 8, VERY HIGH");
        buzzer (&tonos4[0]);
        break;
    case 9:
        RGB (255, 0, 0);
        Serial.print("UV Index level = 9, VERY HIGH");
        break;
    case 10:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 0, 0);
        Serial.print("UV Index level = 10, VERY HIGH");
        buzzer (&tonos4[0]);
        break;
    case 11:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 0, 255);
        Serial.print("UV Index level = 11, EXTREME");
        buzzer (&tonos5[0]);
        break;
    default:
        contador = 0;
        iTono = 0;
        estado=1;
        RGB (255, 0, 255);
        Serial.print("UV Index level = 11+, EXTREME PLUS");
        buzzer (&tonos5[0]);
        break;
  }
}

void buzzer (int* buz){
    contador = 0;
    iTono = 0;
    estado=1;
    while(estado == 1){
     tone(pinBuzzer, *(buz + iTono));
     delay(1);
     contador++;
      if(iTono > countTonos){
       iTono = 0;
       }
      if(contador == 1000){
       iTono++;
       contador = 0;
       }
    estado = digitalRead(pulsador);
    }
  noTone(pinBuzzer);
 return 0;
}

void RGB (int a, int b, int c){
  digitalWrite(ledRojo, a);
  digitalWrite(ledVerde, b);
  digitalWrite(ledAzul, c);
  return 0;
}