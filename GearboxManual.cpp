#include <Arduino.h>
#include <LCD_I2C.h>
#include <MQTT.h>
#include <WiFi.h>
#include <math.h>


#define DC_Motor 2
const int freq = 50;
const int DC_MOtorChannel = 0;
const int resolution = 10;

#define Hall_Sensor 27
double Rev;
int Count = 0;
int OldCount = 0;
void IRAM_ATTR RevCounter(){
  Count+=1;
}

hw_timer_t *My_timer = NULL;
hw_timer_t *My_timer2 = NULL;
hw_timer_t *My_timer3 = NULL;
hw_timer_t *My_timer4 = NULL;
int calTimeMicroS = 1000000;
void IRAM_ATTR Revcal(){
  Rev = (Count - OldCount)/(calTimeMicroS/1000000);
  OldCount = Count;
}

int DCPulse[] = {0,1024};
int DCmotorCount = 0;

//#define ON_OFF_pin_out  12
//#define ON_OFF_pin_in  14
#define Slow_pin_in  26
//#define DownShift_pin_out  32
#define ON_OFF_pin_in  33

const char ssid[] = "Xiaomi12.";
const char pass[] = "09224616000";

const char mqtt_broker[]="test.mosquitto.org";
const char mqtt_topic[]="G1_8/command";
const char mqtt_rev_topic[]="G1_8_rev/command";
const char mqtt_gear_topic[]="G1_8_gear/command";
const char mqtt_client_id[]="arduino_G1_8"; // must change this string to a unique value
int MQTT_PORT=1883;

WiFiClient net;
MQTTClient client;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  
  Serial.print("\nconnecting...");
  while (!client.connect(mqtt_client_id)) {  
    Serial.print(".");
  }
  Serial.println("\nconnected!");

  client.subscribe(mqtt_topic);
  //client.subscribe(mqtt_rev_topic);
  //client.subscribe(mqtt_gear_topic);
}

String Mqtt_input;
void messageReceived(String &topic, String &payload) {
  //Serial.println("incoming: " + topic + " - " + payload);
  if(topic == mqtt_topic){
  Mqtt_input = payload;
  Serial.println(payload);
  }
}

bool doStartStop = false;
void IRAM_ATTR Start_Stop(){
  if(doStartStop){
  DCmotorCount += 1;
  Serial.println(DCmotorCount);
  ledcWrite(DC_MOtorChannel, DCPulse[DCmotorCount%2]);
  Serial.print("DCPulse");
  Serial.println(DCPulse[DCmotorCount%2]);
  doStartStop = false;
  }
}

int pausecount = 0 ;
void IRAM_ATTR MqttMessage(){
  if(Mqtt_input != "0"){
    if(Mqtt_input == "+"){
      //Serial.println(Gearcount);
      pausecount += 1;
    }else if(Mqtt_input == "s"){
      doStartStop = true;
    }
    Mqtt_input = "0";
  }
}

LCD_I2C lcd(0x27, 16, 2);
void LCD_Write(){
  //Serial.print("aaaaaa");
  //lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Gear Pos = ");
  //lcd.print(", I/O = ");
  //lcd.print(GearRatio[PrevGearState-1]);
  lcd.setCursor(1, 1);
  lcd.print("Rev/min = ");
  lcd.print(Rev * 15);
}

void setup() {
  Serial.begin(9600);
  ledcAttachPin(DC_Motor, DC_MOtorChannel);           //dc motor control
  ledcSetup(DC_MOtorChannel, freq, resolution);       //dc motor control

  pinMode(Hall_Sensor, INPUT_PULLDOWN);                              //rev meter
  attachInterrupt(Hall_Sensor, &RevCounter, RISING);  
  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &Revcal, true);
  timerAlarmWrite(My_timer, calTimeMicroS, true);
  timerAlarmEnable(My_timer);                               //rev meter

  My_timer2 = timerBegin(1, 80, true);
  timerAlarmWrite(My_timer2, 10000, true);
  timerAttachInterrupt(My_timer2, &Start_Stop, true);
  timerAlarmEnable(My_timer2);    

  My_timer3 = timerBegin(2, 80, true);
  timerAlarmWrite(My_timer3, 10000, true);
  timerAttachInterrupt(My_timer3, &MqttMessage, true);
  timerAlarmEnable(My_timer3);      

  lcd.begin();    //LCD
  lcd.backlight();

  pinMode(ON_OFF_pin_in, INPUT_PULLDOWN);//on off           
  pinMode(Slow_pin_in, INPUT_PULLDOWN);//upshift
  
  WiFi.begin(ssid, pass);
  client.begin(mqtt_broker, MQTT_PORT, net);
  client.onMessage(messageReceived);

  connect();

  ledcWrite(DC_MOtorChannel, 0);
}


void loop() {
  
  client.loop();

  if (!client.connected()){
    connect();
  }

  client.publish(mqtt_rev_topic , String(Rev * 15));
  LCD_Write();

  if(digitalRead(ON_OFF_pin_in)==HIGH){
    while (digitalRead(ON_OFF_pin_in)==HIGH){
      //do nothing
    }
    doStartStop = true;
  }

  if(digitalRead(Slow_pin_in)==HIGH){
    while (digitalRead(Slow_pin_in)==HIGH){
      //do nothing
    }
    pausecount += 1;
  }

  if((pausecount%2)==1){
    ledcWrite(DC_MOtorChannel, 180);
  }else ledcWrite(DC_MOtorChannel, DCPulse[DCmotorCount%2]);
}
