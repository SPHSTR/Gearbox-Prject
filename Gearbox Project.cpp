#include <Arduino.h>
#include <LCD_I2C.h>
#include <MQTT.h>
#include <WiFi.h>

#define DC_Motor 2
const int freq = 50;
const int DC_MOtorChannel = 0;
const int resolution = 10;
//ledcWrite(DC_MOtorChannel, 1023);    Fullspeed
//ledcWrite(DC_MOtorChannel, 1023*(1/2));    Halfspeed
//ledcWrite(DC_MOtorChannel, 0);    Stop

#define Hall_Sensor 27
double Rev;
double Count = 0;
int OldCount = 0;
void IRAM_ATTR RevCounter(){
  Count+=1;
}

hw_timer_t *My_timer = NULL;
int calTimeMicroS = 1000000;
void IRAM_ATTR Revcal(){
  /*int OldCount = Count;
  unsigned long last_interrupt_time1 = 0;
  for(int i=0;i=1;){
  unsigned long interrupt_time1 = millis();
  if (interrupt_time1 - last_interrupt_time1 > calTime){
    Rev = (Count - OldCount)/calTime ;
    last_interrupt_time1 = interrupt_time1;
    i+=1;
    }
  }*/
  Rev = (Count - OldCount)/calTimeMicroS;
  OldCount = Count;
}


#define Step1  19
#define Step2  18
#define Step3  5
#define Step4  17
int FsteppedPin[] = {Step1,Step2,Step3,Step4};
int BsteppedPin[] = {Step4,Step3,Step2,Step1};
void ForwardStep(int StepMove){
     unsigned long last_interrupt_time2 = 0;
  for(int i=0; i<StepMove;){
    unsigned long interrupt_time2 = millis();
    if (interrupt_time2 - last_interrupt_time2 > 2){
    digitalWrite(FsteppedPin[i%4], HIGH);
    digitalWrite(FsteppedPin[(i+1)%4], LOW);
    digitalWrite(FsteppedPin[(i+2)%4], LOW);
    digitalWrite(FsteppedPin[(i+3)%4], LOW);
    i=+1;
    last_interrupt_time2 = interrupt_time2;
    }
  }
}

void BackwardStep(int StepMove){
  unsigned long last_interrupt_time = 0;
  for(int i=0; i<StepMove;){
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 2){
    digitalWrite(BsteppedPin[i%4], HIGH);
    digitalWrite(BsteppedPin[(i+1)%4], LOW);
    digitalWrite(BsteppedPin[(i+2)%4], LOW);
    digitalWrite(BsteppedPin[(i+3)%4], LOW);
    i=+1;
    last_interrupt_time = interrupt_time;
    }
  }
}


LCD_I2C lcd(0x27, 16, 2);
void LCD_Write(){

}

void setup() {
  ledcAttachPin(DC_Motor, DC_MOtorChannel);           //dc motor control
  ledcSetup(DC_MOtorChannel, freq, resolution);       //dc motor control

  pinMode(Hall_Sensor, INPUT);                              //rev meter
  attachInterrupt(Hall_Sensor, &RevCounter, RISING);  
  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &Revcal, true);
  timerAlarmWrite(My_timer, calTimeMicroS, true);
  timerAlarmEnable(My_timer);                               //rev meter

  pinMode(Step1, OUTPUT);     //stepped motor 
  pinMode(Step2, OUTPUT);
  pinMode(Step3, OUTPUT);
  pinMode(Step4, OUTPUT);     //stepped motor 

  lcd.begin();    //LCD
}

void Start_Stop(){

}

void Shiftup(){

}


void loop() {
  
}
