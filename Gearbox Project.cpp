#include <Arduino.h>
#include <LCD_I2C.h>
#include <MQTT.h>
#include <WiFi.h>

#define DC_Motor 2
const int freq = 50;
const int DC_MOtorChannel = 0;
const int resolution = 10;

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
  unsigned long last_interrupt_time3 = 0;
  for(int i=0; i<StepMove;){
    unsigned long interrupt_time3 = millis();
    if (interrupt_time3 - last_interrupt_time3 > 2){
    digitalWrite(BsteppedPin[i%4], HIGH);
    digitalWrite(BsteppedPin[(i+1)%4], LOW);
    digitalWrite(BsteppedPin[(i+2)%4], LOW);
    digitalWrite(BsteppedPin[(i+3)%4], LOW);
    i=+1;
    last_interrupt_time3 = interrupt_time3;
    }
  }
}

int GearRatio[] = {-2,-1,0,1,2,3,4};
LCD_I2C lcd(0x27, 16, 2);
void LCD_Write(){
  lcd.setCursor(2, 0);
  lcd.print("Gear Pos = ");
  lcd.print(PrevGearState-1);
  lcd.print(" Ratio = ");
  lcd.print(GearRatio[PrevGearState-1]);
  lcd.setCursor(2, 1);
  lcd.print("Rev/min = ");
  lcd.print(Rev * 60);
}

#define ON_OFF_pin_out  12
#define ON_OFF_pin_in  14
#define UpShift_pin_out  25
#define UpShift_pin_in  26
#define DownShift_pin_out  32
#define DownShift_pin_in  33

const char ssid[] = "Xiaomi12.";
const char pass[] = "09224616000";

const char mqtt_broker[]="test.mosquitto.org";
const char mqtt_topic[]="G33/command";
const char mqtt_rev_topic[]="G33/rev";
const char mqtt_gear_topic[]="G33/gear";
const char mqtt_client_id[]="arduino_group_33"; // must change this string to a unique value
int MQTT_PORT=1883;

WiFiClient net;
MQTTClient client;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect(mqtt_client_id)) {  
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe(mqtt_topic);
  client.subscribe(mqtt_rev_topic);
  client.subscribe(mqtt_gear_topic);
}

String Mqtt_input;
void messageReceived(String &topic, String &payload) {
  //Serial.println("incoming: " + topic + " - " + payload);
  Mqtt_input = payload;
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
  lcd.backlight();

  pinMode(ON_OFF_pin_out, OUTPUT);                  //on off
  pinMode(ON_OFF_pin_in, INPUT);
  attachInterrupt(ON_OFF_pin_in, &Start_Stop, RISING); //on off

  pinMode(UpShift_pin_out, OUTPUT);                 //unshift
  pinMode(UpShift_pin_in, INPUT);
  attachInterrupt(UpShift_pin_in, &Shiftup, RISING); //unshift

  pinMode(DownShift_pin_out, OUTPUT);                 //downshift
  pinMode(DownShift_pin_in, INPUT);
  attachInterrupt(DownShift_pin_in, &Shiftdown, RISING);  //downshift

  WiFi.begin(ssid, pass);
  client.begin(mqtt_broker, MQTT_PORT, net);
  client.onMessage(messageReceived);

  connect();
}

int DCPulse[] = {0,1024};
int DCmotorCount = 0;
void IRAM_ATTR Start_Stop(){
  DCmotorCount += 1;
  ledcWrite(DC_MOtorChannel, DCPulse[DCmotorCount%2]);
}

int stepmoveup[] = {1000,1000,1000,1000,1000,1000,0};
int stepmovedown[] = {0,1000,1000,1000,1000,1000,1000};
int GearState = 0;
int PrevGearState = 1;
int Gearcount = 0;
void IRAM_ATTR Shiftup(){
  Gearcount +=1;
  GearState = ((Gearcount%7)+1);
  if((PrevGearState ==  GearState) || (PrevGearState ==7)){
    //do nothing
  }else if(PrevGearState < GearState){
  unsigned long last_interrupt_time4 = 0;
  int i=0;
  while(i==0){
    unsigned long interrupt_time4 = millis();
    if (interrupt_time4 - last_interrupt_time4 <= 4000){
    ledcWrite(DC_MOtorChannel, DCPulse[1]/3);
    last_interrupt_time4 = interrupt_time4;
    }else {
      ledcWrite(DC_MOtorChannel, DCPulse[DCmotorCount%2]);
      ForwardStep(stepmoveup[PrevGearState-1]);
      PrevGearState = GearState;
      i=+1;
    }
  }
  }
}

void IRAM_ATTR Shiftdown(){
  Gearcount -=1;
  GearState = ((Gearcount%7)+1);
  if((PrevGearState ==  GearState) || (PrevGearState ==1)){
    //do nothing
  }else if(PrevGearState > GearState){
  unsigned long last_interrupt_time5 = 0;
  int i=0;
  while(i==0){
    unsigned long interrupt_time5 = millis();
    if (interrupt_time5 - last_interrupt_time5 <= 4000){
    ledcWrite(DC_MOtorChannel, DCPulse[1]/3);
    last_interrupt_time5 = interrupt_time5;
    }else {
      ledcWrite(DC_MOtorChannel, DCPulse[DCmotorCount%2]);
      BackwardStep(stepmovedown[PrevGearState-1]);
      PrevGearState = GearState;
      i=+1;
    }
  }
  }
}

void IRAM_ATTR loop() {
  Mqtt_input = "0";
  client.loop();

  if (!client.connected()){
    connect();
  }

  client.publish(mqtt_rev_topic , String(Rev * 60));
  client.publish(mqtt_gear_topic , String(GearRatio[PrevGearState-1]) + " " + String(PrevGearState-1));
  LCD_Write();


  if(Mqtt_input != "0"){
    if(Mqtt_input == "+"){
      digitalWrite(UpShift_pin_out,HIGH);
      delay(250);
      digitalWrite(UpShift_pin_out,LOW);
    }else if(Mqtt_input == "-"){
      digitalWrite(DownShift_pin_out,HIGH);
      delay(250);
      digitalWrite(DownShift_pin_out,LOW);
    }else if(Mqtt_input == "s"){
      digitalWrite(ON_OFF_pin_out,HIGH);
      delay(250);
      digitalWrite(ON_OFF_pin_out,LOW);
    }
  }

}
