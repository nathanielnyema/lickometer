#include <EEPROM.h>


#define low_thresh 10
#define pump 2
#define spout1 A0
#define spout2 A3
#define inf_thresh 20 // thresh
#define buff_size 10
#define sess_dur 1800 //duration of session in seconds
#define debug false

float thresh = 0.05; // threshold voltage change for lick detection
int pump_dur = 3000; //in milliseconds
bool l_lick = false;
bool r_lick = false;
int l_count = 0;
int r_count = 0;
unsigned long t_init;
unsigned long t;

bool finished=false;
int lv_buffer[buff_size];
int rv_buffer[buff_size];
int l_diff;
int r_diff;

int l_buff=0;
int r_buff=0;

bool has_pumped=false;
bool pumping=false;

int add;
int add2;

void setup() {
  Serial.begin(9600);
  thresh=thresh*(1023.0/5.0); // convert threshold voltage to analogRead units

  // setup the pins
  pinMode(spout1,INPUT);
  pinMode(spout2,INPUT);
  pinMode(pump,OUTPUT);

  // initialize the buffer with zeros
  for (int i=0;i<buff_size;i++){
    lv_buffer[i]=0;
    rv_buffer[i]=0;
  }
}

void loop() {
  if (!finished){
    count_licks();
    fixed_inf();
    if (millis()/1000>=sess_dur){
      finished=true;
      Serial.println("stop");
    }
  }
  check_pump(&pump_dur);
}

void count_licks () {

  // throw away the first couple reads to avoid cross talk
  // see https://speeduino.com/forum/viewtopic.php?t=984
  
  analogRead(spout1);
  analogRead(spout1);

  // shift the values in the buffer for the left spout
  for (int i=0; i<buff_size-1; i++) { lv_buffer[i] = lv_buffer[i+1]; }

  // add new reading to the buffer
  lv_buffer[ buff_size-1 ]=analogRead(spout1);

  // get the change in voltage
  l_diff=last_half_mean(lv_buffer)-first_half_mean(lv_buffer);
  
  analogRead(spout2);
  analogRead(spout2);

  // shift the values in the buffer for the right spout
  for (int i=0; i<buff_size-1; i++) { rv_buffer[i] = rv_buffer[i+1]; }

  // add new reading to the buffer
  rv_buffer[ buff_size-1 ]=analogRead(spout2);

  // get the change in voltage
  r_diff=last_half_mean(rv_buffer)-first_half_mean(rv_buffer);

  if(debug){
    Serial.print("Left Spout Voltage Change:");
    Serial.print(l_diff * (5.0 / 1023.0));
    Serial.print(",");

    Serial.print("Right Spout Voltage Change:");
    Serial.print(r_diff * (5.0 / 1023.0));
    Serial.print(",");
  } else if(!(l_lick||r_lick)){ // if we're not already in the middle of a lick
    if ((l_diff>thresh) || (r_diff>thresh)){ // check if we've crossed the threshold for lick detection on either spout
      if (r_diff>l_diff){ // if so, if the right spout has a greater difference register the lick on the right spout
        r_count++;
        r_lick=true; // raise a flag to say we are currently in the middle of a lick
        String buff=String(millis()) + "," + String(0) +  "," + String(1);
        Serial.println(buff);
      } else{ // otherwise register a left lick
        l_count++;
        l_lick=true;
        String buff=String(millis()) +  "," + String(1) +  "," + String(0);
        Serial.println(buff);
      }

    }
  } else if(r_lick && (r_diff<=low_thresh)){
    // if we've dropped below the threshold for ending lick detection lower the flag
    r_lick=false;
  } else if(l_lick && (l_diff<=low_thresh)){
    l_lick=false;
  }
}

void check_pump (int *pump_dur){
  // function to check if we should stop pumping
  if (pumping){
    if ( (millis()-t_init)>=*pump_dur ) {
      pumping=false;
      digitalWrite(pump,LOW);
    }
  }
}

void fixed_inf(){
  if (!has_pumped && (r_count==inf_thresh || l_count==inf_thresh) ){
    has_pumped=true;
    pumping=true;
    digitalWrite(pump,HIGH);
    t_init=millis();
  }
}

int first_half_mean( int arr[buff_size] ){
  int val=0;
  for (int i=0;i<buff_size/2;i++){
    val+=arr[i];
  }
  return val/(buff_size/2);
}

int last_half_mean( int arr[buff_size] ){
  int val=0;
  for (int i=buff_size/2;i<buff_size;i++){
    val+=arr[i];
  }
  return val/(buff_size/2);
}
