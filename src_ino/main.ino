#include <math.h>
#include <TimerOne.h>
#include <TimerThree.h>
#include <LiquidCrystal_PCF8574.h>
LiquidCrystal_PCF8574 lcd (0x27);

volatile float Ac_min = 25.0;
volatile float Ac_max = 30.0;
volatile float Ac_min_pres = 22.5;
volatile float Ac_max_pres = 40.0;
volatile float Ht_min = 15.0;
volatile float Ht_max = 20.0;
volatile float Ht_min_pres = 5.0;
volatile float Ht_max_pres = 22.0;
volatile float Ac_current_min;
volatile float Ac_current_max;
volatile float Ht_current_min;
volatile float Ht_current_max;

volatile float Ac_min_serial;
volatile float Ac_max_serial;
volatile float Ht_min_serial;
volatile float Ht_max_serial;
   

volatile bool presence_pir= false;
volatile bool presence_audio= false;
volatile bool presence_room=false;
volatile bool first_clap=false;

volatile bool green_state = false;

volatile bool temp_seriale = false;

const int HT_PIN = 6;
const int FAN_PIN = 11;
const int PIR_PIN = 7;
const int SOUND_PIN = 8;
const int GREEN = 5;

volatile unsigned long time_presence_pir = 0;

volatile float temperature;

const int TEMPERATURE_PIN=A1;
const int B=4275;
const long int R0=100000;

volatile int pwm_fan;
volatile int pwm_ht;

const int n_sound_events = 50;
const int sound_interval = 10; //in minuti
volatile int conta_sound_events = 0;
volatile bool start_sound = true;
volatile unsigned long time_presence_audio = 0;
volatile unsigned long time_presence_audio_60 = 0;
volatile unsigned long time_clap=0;




void setup() {
  Serial.begin(9600);
  pinMode(TEMPERATURE_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HT_PIN, OUTPUT );
  pinMode(PIR_PIN, INPUT);
  pinMode(SOUND_PIN, INPUT);
  //pinMode(GREEN,OUTPUT);
  lcd.begin(16,2);
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), checkPresence, RISING);
  attachInterrupt(digitalPinToInterrupt(SOUND_PIN), checkAudio, FALLING);
  attachInterrupt(digitalPinToInterrupt(GREEN), greenOn, FALLING);

  Timer1.initialize(5e06);
  Timer3.initialize(10e06);
  Timer1.attachInterrupt(lcdPrint1);
  Timer3.attachInterrupt(lcdPrint2);
  
}

void loop() {
  checkTemp(); 
  if(!start_sound){
    controlPresenceAudio();
  }
  if(presence_room){
    resetPresenceAudio60();
    presenceGone();
  }
  else{
    controlPresenceRoom();
  }
  serialChangetemperature();
  delay(10000);

}

void checkTemp() {
  getTemp();
  if(temp_seriale){
  Ac_current_min = Ac_min_serial;
  Ac_current_max = Ac_max_serial;
  Ht_current_min= Ht_min_serial;
  Ht_current_max=Ht_max_serial;
    
  }
  else{
    if(presence_room){
    Ac_current_min = Ac_min_pres;
    Ac_current_max = Ac_max_pres;
    Ht_current_min= Ht_min_pres;
    Ht_current_max=Ht_max_pres;
    }
    else{
    Ac_current_min=Ac_min;
    Ac_current_max = Ac_max;
    Ht_current_min= Ht_min;
    Ht_current_max=Ht_max;
    }
  }
  float intervallo_pwm_ac = (Ac_current_max-Ac_current_min) / 25.5;
  float intervallo_pwm_ht = (Ht_current_max-Ht_current_min) / 25.5;
    
    if(temperature< Ac_current_min){
      pwm_fan = 0;  
    }
     if(temperature> Ac_current_max){
      pwm_fan = 255;  
    }
    for(int i=0; i<10; i++){
      if(temperature > Ac_current_min+(i*intervallo_pwm_ac) && temperature < Ac_current_max -((9-i)*intervallo_pwm_ac)){
        pwm_fan = (i*25.5)+25.5;
      }
    }    
      if(temperature< Ht_current_min){
      pwm_ht = 255;  
    }
      if(temperature> Ht_current_max){
      pwm_ht = 0;  
    }
    for(int i=10; i>0; i--){
      if(temperature < Ht_max-((i-1)*intervallo_pwm_ht) && temperature > Ht_min +((10-i)*intervallo_pwm_ac)){
        pwm_fan = (i*25.5);
      }
    }
  }


void getTemp(){
  int value = analogRead(TEMPERATURE_PIN);
  float R = 1023.0/value-1.0;
  R=R0*R; // computes deltaResistance due to Temperature change
  temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15;
  
}

void checkPresence(){
presence_pir = true;
time_presence_pir = millis();
}

void presenceGone(){
  if(time_presence_pir >= 1800000){
    presence_pir = false;
    time_presence_pir = 0;
  }
}

void checkAudio(){

  conta_sound_events ++;
  if (start_sound){
    conta_sound_events = 1;
    time_presence_audio = millis();
    start_sound = false;
  } 
}
void controlPresenceAudio(){  
  if( time_presence_audio < ((time_presence_audio+sound_interval) *6e05)){
    if((conta_sound_events >= n_sound_events) && !presence_audio){
      presence_audio = true;  
      time_presence_audio_60= millis();
    }
  }
  else if(!presence_audio){
    start_sound = true;
  }
}

void resetPresenceAudio60(){
  if (time_presence_audio_60 >= (time_presence_audio_60+3600000)){
    presence_audio = false;
    start_sound = true;
  }
}

void controlPresenceRoom(){       //questa di base serve solo per proiettare sul display se ci sono persone o no
  if (presence_pir || presence_audio)
    presence_room=true;
    if (!(presence_pir && presence_audio))
    presence_room=false;

}
void lcdPrint1(){
  lcd.clear();
  lcd.print("T:");
  lcd.setCursor(0,2); 
  lcd.print(temperature,1); 
  lcd.setCursor(0,6); 
  lcd.print(" Pr:"); 
  lcd.setCursor(0,10);
  lcd.print(presence_room);
  lcd.setCursor(1,0);
  lcd.print("AC:");
  lcd.setCursor(1,3); 
  lcd.print(pwm_fan/255,0);
  lcd.setCursor(1,6); 
  lcd.print("%");
  lcd.setCursor(1,7); 
  lcd.print(" H:");
  lcd.setCursor(1,10); 
  lcd.print(pwm_ht/255,0);
  lcd.setCursor(1,12); 
  lcd.print("%");
  
}
void lcdPrint2(){
  lcd.clear();
  
  lcd.print("AC m:");
  lcd.setCursor(0,5); 
  lcd.print(Ac_current_min,1); 
  lcd.setCursor(0,9); 
  lcd.print(" M:");
  lcd.setCursor(0,12); 
  lcd.print(Ac_current_max,1);
  lcd.setCursor(1,0); 
  lcd.print("H m:"); 
  lcd.setCursor(0,4); 
  lcd.print(Ht_current_min,1);
  lcd.setCursor(0,8); 
  lcd.print(" M:");
  lcd.setCursor(0,11); 
  lcd.print(Ht_current_max,1);
}

void serialChangetemperature(){

  if (Serial.available()>0){
    
   String input=Serial.readString();
   String acm=input.substring(0,2);
   String acM=input.substring(3,5);
   String htm=input.substring(6,8);
   String htM=input.substring(9,11);
     
    
    Ac_min_serial = acm.toFloat();
    Ac_max_serial = acM.toFloat();
    Ht_min_serial = htm.toFloat();
    Ht_max_serial = htM.toFloat();
    temp_seriale = true;
  }
}
 
  void greenOn(){
    unsigned long time_in=millis();
    if (first_clap && (time_in - time_clap)<1000)
     {
      pinMode(GREEN, !green_state);
    }
    else if(!first_clap){
      first_clap=true;
      time_clap=millis();
    }
  
    else 
      first_clap= false;
    
  }