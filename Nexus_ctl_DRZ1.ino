/*
   * ======================================= *
        BlkDem`s MediaStation Controller
        Nexus 2012 3G Based     2016 (c) 
        Using DAC Clarion HX-D2
   * ======================================= *
*/

/*
  A0 - CubiePwrRelay //OUT
  A1 - AddzestModeRelay //OUT
  A2, A3, A4, A5 - Addzest buttons control //OUT
  A6 - Acc In //IN
  A7 - Wheel Buttons //IN
  
  D0, D1 - reserved by FT232
  D2, D3, D4, D5, D9, D12 - lcd
  D13 - play/pause indicator //OUT
  D6 - SuperTOD control //OUT
  D7 - free
  D8 - speaker //OUT
  D10, D11 - BT-Serial (software serial)  
*/

#include <EEPROM.h>
#include <LiquidCrystal.h>
//#include <MsTimer2.h>
//#include <SoftwareSerial.h>
#include <avr/wdt.h>

/*const int PIN_2H = 6;
const int PIN_4H = 7;*/
const int PIN_TOD = 6;
const int PIN_VOLTAGE = 6; //ADC6
const int PIN_BTN = 7; //ADC7
const int PIN_SPEAKER = 13;
const int PIN_CONTRAST = 9;

const int button_gis = 30;

/*const int button_vup = 124;
const int button_vdown = 167;
const int button_preset = 235;
const int button_seek = 25;
const int button_mode = 67;*/

//for 2k2 pullup resistor
const int button_vup = 694;
const int button_vdown = 775;
const int button_preset = 850;
const int button_seek = 167;
const int button_mode = 502;

const int button_tod = 30;

#define _ModeAuto {digitalWrite(PIN_TOD, HIGH); delay(60); digitalWrite(PIN_TOD, LOW);}
//#define _Mode2H {digitalWrite(PIN_2H, HIGH); digitalWrite(PIN_4H, LOW);}
#define _Mode2H {_ModeAuto;}
//#define _Mode4H {digitalWrite(PIN_2H, HIGH); digitalWrite(PIN_4H, HIGH);}
#define _Mode4H {digitalWrite(PIN_TOD, HIGH); delay(620); wdt_reset(); digitalWrite(PIN_TOD, LOW);}

#define _MediaNext {PORTC |= B00000100; delay(50); PORTC &= B11111011; delay(200);} //press HU button Next
#define _MediaPrev {PORTC |= B00001000; delay(50); PORTC &= B11110111;} //press HU button Previous
#define _MediaPlay {PORTC |= B00010000; delay(50); PORTC &= B11101111;} //press HU button Play/Pause
#define _PwrBtn {PORTC |= B00100000; delay(100); PORTC &= B11011111;} //press Power TABLET button
//#define _PwrBtnLong {PORTC |= B00100000; delay(1000); wdt_reset(); delay(1000); wdt_reset(); delay(1000); wdt_reset(); PORTC &= B11011111; _PowerOff; MsTimer2::stop(); _MediaPlay; _AddzestModeCD; AddzestModeAUX=false;}
//#define _AddzestModeAUX {PORTC |= B00000001;} //switch to AUX mode
//#define _AddzestModeCD {PORTC &= B11111110;} //switch to CD mode 
#define _ClarionSource {PORTC |= B00000001; delay(200); PORTC &= B11111110; delay(200);} //switch HX-D2 source


#define _PowerOff {PORTC &= B11111101;} //power relay off
#define _PowerOn {PORTC |= B00000010;} //power relay on
#define _beep {for(byte i=0; i<50; i++) {digitalWrite(PIN_SPEAKER, HIGH); delayMicroseconds(220); digitalWrite(PIN_SPEAKER, LOW); delayMicroseconds(220);} digitalWrite(PIN_SPEAKER, LOW);}
#define _beep_long {for(byte i=0; i<200; i++) {digitalWrite(PIN_SPEAKER, HIGH); delayMicroseconds(220); digitalWrite(PIN_SPEAKER, LOW); delayMicroseconds(220);} digitalWrite(PIN_SPEAKER, LOW);}
#define _beep_double {_beep; delay(50); _beep;} //double beep

const int ENC_CCW = 11; //bt
const int ENC_CW = 10; //bt

#define _volume_up {digitalWrite(ENC_CW, HIGH); delay(50); digitalWrite(ENC_CW, LOW); delay(50);}
#define _volume_down {digitalWrite(ENC_CCW, HIGH); delay(50); digitalWrite(ENC_CCW, LOW); delay(50);}

int CurrentVoltage = 118; //abstact value 
boolean ACCOn = false;
LiquidCrystal lcd(12, 8, 5, 4, 3, 2); //
boolean PlayPause=false;
//boolean SetupMode=false;
//volatile boolean IsShutDown = false;
volatile boolean AutoPowerControlAllow = false;
volatile int PowerCtlDelay = 0;
const int PowerCtlDelayMax = 10;

volatile int ShutDownCounter = 10; 
const int ShutDownCounterMax = 10;
//volatile int Update5sCounter = 0;
volatile int timerCounter=0;

void _mode()
{
  int tmp = analogRead(PIN_BTN);    
  if ((tmp >= (button_vup - button_gis)) && (tmp < (button_vup + button_gis))) 
  { 
    delay(30); 
    tmp = analogRead(PIN_BTN);    
    if ((tmp >= (button_vup - button_gis)) && (tmp < (button_vup + button_gis))) //volume up button
    {
      /*_MediaNext;
      _beep;
      delay(200);*/
      _volume_up;
    }
    
  }
  else
  if ((tmp >= (button_vdown - button_gis)) && (tmp < (button_vdown + button_gis))) 
  { 
    delay(30); 
    tmp = analogRead(PIN_BTN);    
    if ((tmp >= (button_vdown - button_gis)) && (tmp < (button_vdown + button_gis))/* && (AddzestModeAUX)*/) //volume down button 
    {
      /*_MediaPrev;
      _beep;
      delay(200);*/
      _volume_down;
    }
  }
  else
  if ((tmp >= (button_seek - button_gis)) && (tmp < (button_seek + button_gis))) 
  { 
    delay(30); 
    tmp = analogRead(PIN_BTN);    
    if ((tmp >= (button_seek - button_gis)) && (tmp < (button_seek + button_gis))/* && (AddzestModeAUX)*/) //seek button (play/pause)
    {
      _beep;
      _MediaPlay;
      delay(500);
    }
  }
  else
  if ((tmp >= (button_mode - button_gis)) && (tmp < (button_mode + button_gis))) 
  { 
    delay(30); 
    tmp = analogRead(PIN_BTN);    
    if ((tmp >= (button_mode - button_gis)) && (tmp < (button_mode + button_gis))) //mode button 
    {      
        delay(500);
        wdt_reset();
        tmp = analogRead(PIN_BTN);    
        if ((tmp >= (button_mode - button_gis)) && (tmp < (button_mode + button_gis)))
        {
          _beep;
          AutoPowerControlAllow = !AutoPowerControlAllow;
          if (!AutoPowerControlAllow)
          {
            _PowerOff;
            delay(100);
            _beep;
            
          }
          delay(900);
          wdt_reset();
        }
        else
        {
          _ClarionSource;
          delay(500);
          wdt_reset();
        }
        
    }    
  }
  else
  if ((tmp >= (button_preset - button_gis)) && (tmp < (button_preset + button_gis))) 
  { 
    delay(50); 
    tmp = analogRead(PIN_BTN);    
    if ((tmp >= (button_preset - button_gis)) && (tmp < (button_preset + button_gis)))  //preset button 
    {      
      _MediaNext;
      _beep;
    }
  }

}

boolean CheckACCOn()
{
  if (Voltage() > 100)
  {
    delay(100);
    wdt_reset();
    if (Voltage() > 100)
    {
      return true;
    }
    else
    {
      return false;
    }
  }  
  return false;
}

int Voltage()
{
  CurrentVoltage = analogRead(PIN_VOLTAGE)/4;
  return CurrentVoltage;
}

void PowerControl()
{
  if (!AutoPowerControlAllow)
  {
    return;
  }
  if (CheckACCOn())
  {
      _PowerOn;
  }
  else
  {
    if (!CheckACCOn())
    {
      for (int y=0; y<5; y++)
      {
        delay(1000);
        wdt_reset();
        if (CheckACCOn())
        {
          return;
        }
      }
      _PowerOff;
    }
  }
}


void Banner()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HoldX Station");
  lcd.setCursor(0, 1);
  lcd.print("BlkDem (c)2013");
}

void setup()
{
  pinMode(PIN_TOD, OUTPUT);
  //digitalWrite(PIN_TOD, HIGH);
  
  pinMode(PIN_SPEAKER, OUTPUT);
  analogWrite(PIN_CONTRAST, 15);
  
  DDRC = B00111111; //sets A0-A5 to OUTPUT
  
  

  wdt_enable(WDTO_2S); //start watchdog
  //_beep;
  
  //Serial.begin(115200);
  lcd.begin(24, 2);
  Banner();
  delay(2);
  //MsTimer2::set(1, MyTimer);
  //MsTimer2::start();
  if (CheckACCOn()) {_PowerOn; _beep;} 
}

void loop()
{
  wdt_reset();
/*  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(analogRead(6)/4);
  delay(50);
  return;*/
  PowerControl();
  _mode();
  //lcd.setCursor(0,0);
  //lcd.print(Voltage());
  //if (ShutDownCounter<2) {MsTimer2::stop();}
  //bt();
  //return;
}

