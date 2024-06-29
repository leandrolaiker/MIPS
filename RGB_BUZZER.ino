int ledRojo = 3;
int ledVerde = 5;
int ledAzul = 6;
int pulsador = 8;
int pinBuzzer = 9;
const int tonos[] = {261, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
const int countTonos = 10;
int estado;
int iTono;
int contador;
const long eventTime_Tem = 2000; //delay time
unsigned long previousTime_Tem = 0; 
int x;
unsigned long temptot = 0;

void setup() {
  pinMode(ledRojo,OUTPUT);
  pinMode(ledVerde,OUTPUT);
  pinMode(ledAzul,OUTPUT);
  Serial.begin(9600);
  pinMode(pulsador,INPUT_PULLUP);  
  analogReference(INTERNAL); 
}

void loop() {
  contador = 0;
  iTono = 0;
  estado=1;
  digitalWrite(ledRojo,0);
  digitalWrite(ledVerde,255);
  digitalWrite(ledAzul,0);
  int sensorValue = temperatura();
  Serial.println(sensorValue);
  delay(1);
  while(temperatura() > 37.5){
    digitalWrite(ledRojo,255);
    digitalWrite(ledVerde,0);
    digitalWrite(ledAzul,0);
    while(temperatura() > 37.5 && estado == 1){
         tone(pinBuzzer, tonos[iTono]);
         delay(1);
         contador++;
         if(iTono >= countTonos){
           iTono = 0;
         }
         if(contador == 1000){
           iTono++;
           contador = 0;
         }
       estado = digitalRead(pulsador);
      }
    noTone(pinBuzzer);
  }
  while(temperatura() < 19.5){
    digitalWrite(ledRojo,0);
    digitalWrite(ledVerde,0);
    digitalWrite(ledAzul,255);
    while(temperatura() < 32.5 && estado == 1){
      iTono=10;
         tone(pinBuzzer, tonos[iTono]);
         delay(1);
         contador++;
         if(iTono <= 0){
           iTono = 0;
         }
         if(contador == 1000){
           iTono--;
           contador = 0;
         }
       estado = digitalRead(pulsador);
      }
    noTone(pinBuzzer);
  }
}

int temperatura() {
int x;
unsigned long currentTime = millis(); //assigning the value of mills
unsigned long temptot = 0;
//taking 100 sample and adding
for(x=0; x<100 ; x++)
{
  temptot += analogRead(A0);
  }
float  sensorValue = temptot/100; //calculating average
float voltage = sensorValue * (1100 / 1023); //convert sensor reading into milli volt
float temp = voltage*0.1; //convert milli volt to temperature degree Celsius 
if(currentTime-previousTime_Tem >= eventTime_Tem) //Check for delay time
{
  Serial.print(temp);
  Serial.println("ºC");
  previousTime_Tem = currentTime;
}
return temp;
}