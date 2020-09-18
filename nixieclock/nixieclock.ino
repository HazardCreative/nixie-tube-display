/* ************************************
* Nixie Tube Clock - 2013.04.06
* Michael Niggel 
* (and based on various authors' open source code)
*
* 6 digits, 4 separators, 3 mode indicators
* Time, date, and timer function
* external RTC time/date source (DS1307); millis()-based timer
* Cathode poison prevention mode - hold button on startup to activate
* Inputs: one button, one rotary encoder w/button
*/

// debug flag
// turn ON for serial debug messages 
// OFF for faster execution, smaller compile size, etc.
 #define DEBUG

// Set pins for ArduiNix
// SN74141 (1)
const int outPin_c0_a = 2;
const int outPin_c0_b = 3;
const int outPin_c0_c = 4;
const int outPin_c0_d = 5;
// SN74141 (2)
const int outPin_c1_a = 6;              
const int outPin_c1_b = 7;
const int outPin_c1_c = 8;
const int outPin_c1_d = 9;
// Anode pins (Changed from other sketches that seemed backwards)
const int outPin_a_1 = 13;
const int outPin_a_2 = 12;
const int outPin_a_3 = 11;
const int outPin_a_4 = 10;

// display setup
byte NumberArray[6]={0,0,0,0,0,0}; // current display
boolean display_status = false; // which tube anode to display on now
long tubes_busy_until = 0; // multiplex delay holder
const int tube_delay = 3; // multiplexing delay; adjust for tube type
const int check_delay = 250; // RTC check interval
long check_until = 0; // RTC check delay holder

// mode changes
byte active_mode = 0;
byte active_sub_mode = 0; // setting
boolean mode_changed = true;
boolean sub_mode_changed = true;

// countdown timer
const int timer_1s_delay = 1000; // countdown 1s
long timer_delay = 0;
const int timer_001s_delay = 10; // countdown 1/10th s
long timer_delay001 = 0; 

// vars for setting and timer
int8_t set_sd = 0; // second hundredths
int8_t set_s = 0;
int8_t set_m = 0;
int8_t set_h = 0;

// temp vars for converting to BCD
int tmp_sd = 0;
int tmp_s = 0;
int tmp_m = 0;
int tmp_h = 0;

// Set pins for rotary and button
const int ENC_A = 14;
const int ENC_B = 15;
#define ENC_PORT PINC
const int ENC_BTN = 16;
const int STD_BTN = 17;

int8_t enc_data;

boolean enc_buttonstate;
boolean enc_debouncing = false;
boolean enc_btnchange = false;
boolean enc_lastbuttonreading = HIGH;
long enc_lastDebounceTime = 0;
const int enc_debounceDelay = 50;

boolean std_buttonstate;
boolean std_debouncing = false;
boolean std_btnchange = false;
boolean std_lastbuttonreading = HIGH;
long std_lastDebounceTime = 0;
const int std_debounceDelay = 50;

// Set up DS1307
#include "Wire.h"
#define DS1307_I2C_ADDRESS 0x68  // This is the I2C address
#if defined(ARDUINO) && ARDUINO >= 100   // Arduino v1.0 and newer
  #define I2C_WRITE Wire.write 
  #define I2C_READ Wire.read
#else                                   // Arduino Prior to v1.0 
  #define I2C_WRITE Wire.send 
  #define I2C_READ Wire.receive
#endif

// Global Variables
byte t_sec, t_min, t_hour, t_weekday, t_day, t_month, t_year;
byte zero;

// Set up Port Expander
#define MCP23008_I2C_ADDRESS 0x20

// Indicator lamps
byte indicatorLamps;
#define lampMode0 1 // mode indicators
#define lampMode1 2
#define lampMode2 3
#define lampLL 4 // lower left colon/period dot
#define lampLR 5
#define lampUL 6
#define lampUR 7 // upper right

// Init debug var
#ifdef DEBUG
  boolean print_tube_display_serial = false;
#endif

// cathode poison prevention mode flag
byte prevent_mode = false;

// iterator temp var
int i;

void setup() {
  // initialize ArduiNix pins
  pinMode(outPin_c0_a, OUTPUT);
  pinMode(outPin_c0_b, OUTPUT);
  pinMode(outPin_c0_c, OUTPUT);
  pinMode(outPin_c0_d, OUTPUT);

  pinMode(outPin_c1_a, OUTPUT);
  pinMode(outPin_c1_b, OUTPUT);
  pinMode(outPin_c1_c, OUTPUT);
  pinMode(outPin_c1_d, OUTPUT);

  pinMode(outPin_a_1, OUTPUT);
  pinMode(outPin_a_2, OUTPUT);
  pinMode(outPin_a_3, OUTPUT);
  pinMode(outPin_a_4, OUTPUT);
  
  // Set indicator channel always high
  // (no longer multiplexed; now controlled by I2C)
  digitalWrite(outPin_a_4, HIGH);
  
  // Set up I2C
  Wire.begin();
  
  // Set initial expander ports
  indicatorLamps = 0xff; // all lamps on
  setExpander();

  // Set up encoder pins and pull-ups
  pinMode(ENC_A, INPUT);
  digitalWrite(ENC_A, HIGH);
  pinMode(ENC_B, INPUT);
  digitalWrite(ENC_B, HIGH);
  pinMode(ENC_BTN, INPUT);
  digitalWrite(ENC_BTN, HIGH);
  pinMode(STD_BTN, INPUT);
  digitalWrite(STD_BTN, HIGH);

  #ifdef DEBUG
    Serial.begin (115200);
    Serial.println("Start");
  #endif
  
  // display test  
  timer_delay = 0;
  while (millis() < timer_1s_delay * 1.1) {
    if (millis() > timer_delay) {
      timer_delay = millis() + (timer_1s_delay / 10);

      int i = millis() / (timer_1s_delay / 10);
      NumberArray[0] = i;
      NumberArray[1] = i;
      NumberArray[2] = i;
      NumberArray[3] = i;
      NumberArray[4] = i;
      NumberArray[5] = i;
      
      #ifdef DEBUG
        print_tube_display_serial = true;
      #endif
    }
    WriteDisplay( NumberArray );
  }
  indicatorLamps = 0x00; // all lamps on
  setExpander();
  
  // wait for all init states to complete
  delay(timer_1s_delay * 0.9);
  
  // cathode poison prevention mode: hold button at end of tube test
  prevent_mode = digitalRead(STD_BTN);
  if(!prevent_mode) {
    timer_delay = 0;
    i = 0;
    #ifdef DEBUG
      Serial.println("Cathode prevention mode engaged");
    #endif
  }
}

////////////////////////////////////////////////////////////////////////
void loop() {
  if (!prevent_mode) {   
    if (millis() > timer_delay) {
      timer_delay = millis() + (timer_1s_delay / 100);

      NumberArray[0] = i;
      NumberArray[1] = i;
      NumberArray[2] = i;
      NumberArray[3] = i;
      NumberArray[4] = i;
      NumberArray[5] = i;
      
      i++;
      if (i > 9) {
        i = 0;
      }

      #ifdef DEBUG
        print_tube_display_serial = true;
      #endif
    }
    WriteDisplay( NumberArray );

  } else {
    read_inputs();
  
    switch(active_mode) { // check mode (time/date/timer)
      case 0: // time
        if (mode_changed) { // init new mode
          #ifdef DEBUG
            Serial.println("Time");
          #endif
          bitSet(indicatorLamps, lampMode0); // set mode lamp on
          bitClear(indicatorLamps, lampMode1);
          bitClear(indicatorLamps, lampMode2);
          
          bitSet(indicatorLamps, lampUL); // set separator lamps
          bitSet(indicatorLamps, lampLL);
          bitSet(indicatorLamps, lampUR);
          bitSet(indicatorLamps, lampLR);
  
          mode_changed = false; // run once per switch
        }
        switch (active_sub_mode) { // check submode (run/set s/m/h)
          case 0: // show time
            if (sub_mode_changed) {// init new submode
              #ifdef DEBUG
                Serial.println("Get Time");
              #endif
              sub_mode_changed = false; // run only once
            }
            if (millis() >= check_until) {
              check_until = millis() + check_delay;
              getDateDs1307(); // pull data from RTC
  
              // Fill in the array used to display the tubes.
              NumberArray[0] = (t_hour & 0xf0) >> 4;
              NumberArray[1] = t_hour & 0x0f;
              NumberArray[2] = (t_min & 0xf0) >> 4;
              NumberArray[3] = t_min & 0x0f;
              NumberArray[4] = (t_sec & 0xf0) >> 4;
              NumberArray[5] = t_sec & 0x0f;
              
              if (t_sec & 0x01) { // separators blink on/off each full second
                bitClear(indicatorLamps, lampUL);
                bitClear(indicatorLamps, lampLL);
                bitClear(indicatorLamps, lampUR);
                bitClear(indicatorLamps, lampLR);
              } else {
                bitSet(indicatorLamps, lampUL);
                bitSet(indicatorLamps, lampLL);
                bitSet(indicatorLamps, lampUR);
                bitSet(indicatorLamps, lampLR);
              }
              #ifdef DEBUG
                // debug shows RTC out, not tube display
                Serial.print((t_hour & 0xf0) >> 4);
                Serial.print(t_hour & 0x0f);
                Serial.print(":");
                Serial.print((t_min & 0xf0) >> 4);
                Serial.print(t_min & 0x0f);
                Serial.print(":");
                Serial.print((t_sec & 0xf0) >> 4);
                Serial.print(t_sec & 0x0f);
                Serial.println();
              #endif
            }
            break;
          case 1: // set seconds
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Time Seconds");
              #endif
  
              set_s = bcdToDec(t_sec); // convert current time from bcd for easier manipulation
              set_m = bcdToDec(t_min);
              set_h = bcdToDec(t_hour);
              
              sub_mode_changed = false;
            }
            if (enc_data) { // if the rotary encoder has moved
              set_s += enc_data; // set data from encoder input
              if (set_s < 0) set_s = 59; // limits
              if (set_s > 59) set_s = 0;
              enc_data = 0; // ready encoder for new input
              #ifdef DEBUG
                Serial.println(set_s);
              #endif            
            }
            if ((millis() / 500) & 0x01) { // blink current display at 0.5s interval
              tmp_s = decToBcd(set_s);
              NumberArray[4] = (tmp_s & 0xf0) >> 4;
              NumberArray[5] = tmp_s & 0x0f;
            } else {
              NumberArray[4] = 0xff;
              NumberArray[5] = 0xff;
            }
            break;
          case 2: // set minutes
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Time Minutes");
              #endif
  
              NumberArray[4] = (tmp_s & 0xf0) >> 4;
              NumberArray[5] = tmp_s & 0x0f;
              
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_m += enc_data;
              if (set_m < 0) set_m = 59;
              if (set_m > 59) set_m = 0;
              enc_data = 0;
              #ifdef DEBUG
                Serial.println(set_m);
              #endif          
            }
            if ((millis() / 500) & 0x01) {
              tmp_m = decToBcd(set_m);
              NumberArray[2] = (tmp_m & 0xf0) >> 4;
              NumberArray[3] = tmp_m & 0x0f;
            } else {
              NumberArray[2] = 0xff;
              NumberArray[3] = 0xff;
            }
            break;
          case 3: // set hours
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Time Hours");
              #endif
  
              NumberArray[2] = (tmp_m & 0xf0) >> 4;
              NumberArray[3] = tmp_m & 0x0f;
              
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_h += enc_data;
              if (set_h < 0) set_h = 23;
              if (set_h > 23) set_h = 0;
              enc_data = 0;
              #ifdef DEBUG
                Serial.println(set_h);
              #endif           
            }
            if ((millis() / 500) & 0x01) {
              tmp_h = decToBcd(set_h);
              NumberArray[0] = (tmp_h & 0xf0) >> 4;
              NumberArray[1] = tmp_h & 0x0f;
            } else {
              NumberArray[0] = 0xff;
              NumberArray[1] = 0xff;
            }
            break;
          default: // push time to DS1307
            getDateDs1307(); // pull fresh time/date data from RTC
            setTimeDs1307(); // push time data to DS1307
            #ifdef DEBUG
              print_tube_display_serial = true;
            #endif
            
            active_sub_mode = 0;
        }
        break;
      case 1: // date
        if (mode_changed) {
          #ifdef DEBUG
            Serial.println("Date");
          #endif
          
          bitClear(indicatorLamps, lampMode0);
          bitSet(indicatorLamps, lampMode1);
          bitClear(indicatorLamps, lampMode2);
   
          bitSet(indicatorLamps, lampLL);
          bitClear(indicatorLamps, lampUL);
          bitSet(indicatorLamps, lampLR);
          bitClear(indicatorLamps, lampUR);
  
         mode_changed = false;
        }
        switch (active_sub_mode) {
          case 0: // get date
            if (sub_mode_changed) {           
              #ifdef DEBUG
                Serial.println("Get Date");
              #endif
  
              sub_mode_changed = false;
            }
            if (millis() >= check_until) {
              check_until = millis() + check_delay;
              getDateDs1307(); // pull data from RTC
               
              // Fill in the Number array used to display the tubes.
              NumberArray[0] = (t_year & 0xf0) >> 4;
              NumberArray[1] = t_year & 0x0f;
              NumberArray[2] = (t_month & 0xf0) >> 4;
              NumberArray[3] = t_month & 0x0f;
              NumberArray[4] = (t_day & 0xf0) >> 4;
              NumberArray[5] = t_day & 0x0f;
  
              set_s = bcdToDec(t_day);
              set_m = bcdToDec(t_month);
              set_h = bcdToDec(t_year);
            }
            break;
          case 1: // set seconds
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Date Day");
              #endif
              
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_s += enc_data;
              if (set_s < 0) set_s = 31;
              if (set_s > 31) set_s = 0;
              enc_data = 0; 
              #ifdef DEBUG
                Serial.println(set_s);
              #endif            
            }
            if ((millis() / 500) & 0x01) {
              tmp_s = decToBcd(set_s);
              NumberArray[4] = (tmp_s & 0xf0) >> 4;
              NumberArray[5] = tmp_s & 0x0f;
            } else {
              NumberArray[4] = 0xff;
              NumberArray[5] = 0xff;
            }
            break;
          case 2: // set minutes
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Date Month");
              #endif
  
              NumberArray[4] = (tmp_s & 0xf0) >> 4;
              NumberArray[5] = tmp_s & 0x0f;
              
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_m += enc_data;
              if (set_m < 0) set_m = 12;
              if (set_m > 12) set_m = 0;
              enc_data = 0;
              #ifdef DEBUG
                Serial.println(set_m);
              #endif
            }
            if ((millis() / 500) & 0x01) {
              tmp_m = decToBcd(set_m);
              NumberArray[2] = (tmp_m & 0xf0) >> 4;
              NumberArray[3] = tmp_m & 0x0f;
            } else {
              NumberArray[2] = 0xff;
              NumberArray[3] = 0xff;
            }
            break;
          case 3: // set hours
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Date Year");
              #endif
  
              NumberArray[2] = (tmp_m & 0xf0) >> 4;
              NumberArray[3] = tmp_m & 0x0f;
              
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_h += enc_data;
              if (set_h < 0) set_h = 99;
              if (set_h > 99) set_h = 0;
              enc_data = 0;
              #ifdef DEBUG
                Serial.println(set_h);
              #endif
              
            }
            if ((millis() / 500) & 0x01) {
              tmp_h = decToBcd(set_h);
              NumberArray[0] = (tmp_h & 0xf0) >> 4;
              NumberArray[1] = tmp_h & 0x0f;
            } else {
              NumberArray[0] = 0xff;
              NumberArray[1] = 0xff;
            }
            break;
          default:
            getDateDs1307(); // pull fresh data from RTC
            setDateDs1307(); // push data to DS1307
            #ifdef DEBUG
              print_tube_display_serial = true;
            #endif
            
            active_sub_mode = 0;
        }
        break;
      case 2: // countdown timer
        if (mode_changed) {
          #ifdef DEBUG
            Serial.println("Timer");
          #endif
                
          set_sd = 0;
          set_s = 0;
          set_m = 0;
          set_h = 0;
          tmp_sd = decToBcd(set_sd);
          tmp_s = decToBcd(set_s);
          tmp_m = decToBcd(set_m);
          tmp_h = decToBcd(set_h);
  
          NumberArray[0] = 0xff; // blank
          NumberArray[1] = 0xff;
          NumberArray[2] = 0x00;
          NumberArray[3] = 0x00;
          NumberArray[4] = 0x00;
          NumberArray[5] = 0x00;
          
          bitClear(indicatorLamps, lampMode0);
          bitClear(indicatorLamps, lampMode1);
          bitSet(indicatorLamps, lampMode2);
         
          bitClear(indicatorLamps, lampUL);
          bitClear(indicatorLamps, lampLL);
          bitSet(indicatorLamps, lampUR);
          bitSet(indicatorLamps, lampLR);
  
          mode_changed = false;
        }
        switch (active_sub_mode) {
          case 0: // run countdown
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Run Timer");
              #endif
                          
              sub_mode_changed = false;
            }
            
            if (set_sd || set_s || set_m || set_h) {
              if (!set_m && !set_h) { // less than 1m left; show fractions of second
                if (millis() > timer_delay001) {
                  timer_delay001 = millis() + timer_001s_delay;
  
                  if (set_sd) {
                    set_sd--;
                  } else {
                    if (set_s) {
                      set_s--;
                      set_sd = 99;
                    }
                  }
    
                  tmp_sd = decToBcd(set_sd);
                  tmp_s = decToBcd(set_s);
      
                  // Fill in the Number array used to display the tubes.
                  NumberArray[0] = 0xff; // always blank left tubes
                  NumberArray[1] = 0xff;
                  NumberArray[2] = (tmp_s & 0xf0) >> 4;
                  NumberArray[3] = tmp_s & 0x0f;
                  NumberArray[4] = (tmp_sd & 0xf0) >> 4;
                  NumberArray[5] = tmp_sd & 0x0f;
    
                  bitClear(indicatorLamps, lampUL); // always blank left lamps
                  bitClear(indicatorLamps, lampLL);
                  bitClear(indicatorLamps, lampUR);
                  bitSet(indicatorLamps, lampLR); // always on lower right (decimal)
                  
                  #ifdef DEBUG
                    print_tube_display_serial = true;
                  #endif
  
                }
              } else if (millis() > timer_delay) {
                timer_delay = millis() + timer_1s_delay;
  
                set_sd = 99;
                
                if (set_s) {
                  set_s--;
                } else {
                  if (set_m) {
                    set_m--;
                    set_s = 59;
                  } else {
                    if (set_h) {
                      set_h--;
                      set_m = 59;
                    }
                  }
                }
                            
                tmp_s = decToBcd(set_s);
                tmp_m = decToBcd(set_m);
                tmp_h = decToBcd(set_h);
    
                // Fill in the Number array used to display the tubes.
                NumberArray[0] = (tmp_h & 0xf0) >> 4;
                NumberArray[1] = tmp_h & 0x0f;
                NumberArray[2] = (tmp_m & 0xf0) >> 4;
                NumberArray[3] = tmp_m & 0x0f;
                NumberArray[4] = (tmp_s & 0xf0) >> 4;
                NumberArray[5] = tmp_s & 0x0f;
  
                if (set_s & 0x01) {
                  bitClear(indicatorLamps, lampUL);
                  bitClear(indicatorLamps, lampLL);
                  bitClear(indicatorLamps, lampUR);
                  bitClear(indicatorLamps, lampLR);
                } else {
                  bitSet(indicatorLamps, lampUL);
                  bitSet(indicatorLamps, lampLL);
                  bitSet(indicatorLamps, lampUR);
                  bitSet(indicatorLamps, lampLR);
                }
                
                #ifdef DEBUG
                  print_tube_display_serial = true;
                #endif
              }
            }
                 
            break;
          case 1: // set seconds
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Timer Seconds");
              #endif
  
              bitSet(indicatorLamps, lampUL);
              bitSet(indicatorLamps, lampLL);
              NumberArray[0] = (tmp_h & 0xf0) >> 4;
              NumberArray[1] = tmp_h & 0x0f;
              NumberArray[2] = (tmp_m & 0xf0) >> 4;
              NumberArray[3] = tmp_m & 0x0f;
              NumberArray[4] = (tmp_s & 0xf0) >> 4;
              NumberArray[5] = tmp_s & 0x0f;
  
              bitSet(indicatorLamps, lampUR);
              bitSet(indicatorLamps, lampLR);
  
              set_sd = 0;
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_s += enc_data;
              if (set_s < 0) set_s = 59;
              if (set_s > 59) set_s = 0;
              enc_data = 0; 
              #ifdef DEBUG
                Serial.println(set_s);
              #endif            
            }
            if ((millis() / 500) & 0x01) {
              tmp_s = decToBcd(set_s);
              NumberArray[4] = (tmp_s & 0xf0) >> 4;
              NumberArray[5] = tmp_s & 0x0f;
            } else {
              NumberArray[4] = 0xff;
              NumberArray[5] = 0xff;
            }
            break;
          case 2: // set minutes
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Timer Minutes");
              #endif
              tmp_s = decToBcd(set_s);
              NumberArray[4] = (tmp_s & 0xf0) >> 4;
              NumberArray[5] = tmp_s & 0x0f;
             
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_m += enc_data;
              if (set_m < 0) set_m = 59;
              if (set_m > 59) set_m = 0;
              enc_data = 0;
              #ifdef DEBUG
                Serial.println(set_m);
              #endif            
            }
            if ((millis() / 500) & 0x01) {
              tmp_m = decToBcd(set_m);
              NumberArray[2] = (tmp_m & 0xf0) >> 4;
              NumberArray[3] = tmp_m & 0x0f;
            } else {
              NumberArray[2] = 0xff;
              NumberArray[3] = 0xff;
            }
            break;
          case 3: // set hours
            if (sub_mode_changed) {
              #ifdef DEBUG
                Serial.println("Set Timer Hours");
              #endif
  
              tmp_m = decToBcd(set_m);
              NumberArray[2] = (tmp_m & 0xf0) >> 4;
              NumberArray[3] = tmp_m & 0x0f;
              
              bitSet(indicatorLamps, lampUL);
              bitSet(indicatorLamps, lampLL);
  
              sub_mode_changed = false;
            }
            if (enc_data) {
              set_h += enc_data;
              if (set_h < 0) set_h = 99;
              if (set_h > 99) set_h = 0;
              enc_data = 0;
              #ifdef DEBUG
                Serial.println(set_h);
              #endif            
            }
            if ((millis() / 500) & 0x01) {
              tmp_h = decToBcd(set_h);
              NumberArray[0] = (tmp_h & 0xf0) >> 4;
              NumberArray[1] = tmp_h & 0x0f;
            } else {
              NumberArray[0] = 0xff;
              NumberArray[1] = 0xff;
            }
            break;
          default:
            active_sub_mode = 0;
        }
       break;
      default:
        #ifdef DEBUG
          Serial.println("Reset Mode");
        #endif
        
        active_mode = 0;
    }
  
    // Display.
    WriteDisplay( NumberArray );
    setExpander();
  }
}
