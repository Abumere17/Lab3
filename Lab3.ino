#include <IRremote.h>
#include <Wire.h>
#include <RTClib.h>
#include <arduinoFFT.h>
#include <LiquidCrystal.h>

/*-------LCD & Pins---------*/
LiquidCrystal LCD(12, 11, 5, 4, 3, 2);
/*-------Motor Pins-----------*/
const int ENABLE = 44; /*Enable Pin*/ /*PWM*/
const int DIRA = 23; /*Logic A Pin*/
const int DIRB = 24; /*Logic B Pin*/
/*--------Button Pin-----------*/
const int BUTTON = 22; /*Button Pin*/

/*--------RTC Clock-----------*/
String RTC_Time = "00:00:00";
RTC_DS1307 RTC;
/*--------IR Receiver-----------*/
IRrecv IR(26); /*IR Receiver is on PIN 26*/

/*------Fan Variables------*/
bool fanOn = false;
bool speedChanged = false;
int speedLevel = 0; 
const int Speeds[4] = {0, 120, 190, 255};
int direction = 0; // 0 is cw, 1 is ccw   

/*-----Time Variables*----*/
bool timeEntry = false;
int TimeEntry[6] = {0};
int entryPos = 0;

/*-------Sound Sensor Pins-----------*/
const int MICROPHONE = A0;
/*-------FFT------------*/
#define SAMPLES 128               
#define SAMPLING_FREQUENCY 2048
double vReal[SAMPLES];            
double vImag[SAMPLES]; 
unsigned int samplingPeriod;
unsigned long microSeconds;

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

void setup() {

    /*-------Setup LED------*/
    LCD.begin(16, 2);
    /*----Setup Motor Pins-----*/
    pinMode(ENABLE, OUTPUT);
    pinMode(DIRA, OUTPUT);
    pinMode(DIRB, OUTPUT);
     /*----Setup Button Pin-----*/
    pinMode(BUTTON, INPUT); 

    Serial.begin(9600);
    while (!Serial); 

     /*----Setup IR Receiver-----*/
    IR.enableIRIn();
    Serial.println(F("Ready to receive IR signals"));

     /*----Setup RTC-----*/
    RTC.begin();
    Serial.println(F("RTC ready"));

   /*---Setup Microphone----*/
    pinMode(MICROPHONE, INPUT); 
    samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void increaseFanSpeed() {
   if(speedLevel < 3) {speedLevel++;}
}

void decreaseFanSpeed() {
  if(speedLevel > 0) {speedLevel--;}
}

void loop() {
    /*-----Read IR data-------*/
    if (IR.decode()) { 
        translateIR(IR.decodedIRData.decodedRawData);
        delay(100);
        IR.resume();
    }

    /*-----Manage Fan*-----*/
    if(fanOn) {
        if(direction == 0) {
            digitalWrite(DIRA, HIGH);
            digitalWrite(DIRB, LOW);
        }
        else if (direction == 1) {
            digitalWrite(DIRA, LOW);
            digitalWrite(DIRB, HIGH);
        }
        analogWrite(ENABLE, Speeds[speedLevel]);
    } else {
         analogWrite(ENABLE, Speeds[0]);
    }


    /*------Button reading-----*/
    if (digitalRead(BUTTON) == HIGH) {
        delay(200); // debounce
        direction = (direction == 0) ? 1 : 0;
    }

    /*-----------Audio Sampling---------------*/
    for (int i = 0; i < SAMPLES; i++) {
        microSeconds = micros();
        vReal[i] = analogRead(MICROPHONE);
        vImag[i] = 0;              // Set imaginary part to 0
        delayMicroseconds(samplingPeriod); // Ensure proper sampling rate
    }

    FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Apply Hamming window
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);                 // Compute FFT
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);    

    double peakFrequency = FFT.majorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY) - 195;
    Serial.println(peakFrequency);

    if (peakFrequency >= 170 && peakFrequency <= 180) {  // C4 (262Hz ± 2%)
      Serial.println("C4 detected");
      increaseFanSpeed();
    } 
    else if (peakFrequency >= 430 && peakFrequency <= 450) {  // A4 (440Hz ± 2%)
      Serial.println("A4 detected");
      decreaseFanSpeed();
    }

    /*----RTC-----*/
    DateTime Current = RTC.now();
    String Hour = (Current.hour() < 10) ? ("0" + String(Current.hour())) : String(Current.hour());
    String Minute = (Current.minute() < 10) ? ("0" + String(Current.minute())) : String(Current.minute());
    String Second = (Current.second() < 10) ? ("0" + String(Current.second())) : String(Current.second());
    RTC_Time = (Hour + ":" + Minute + ":" + Second);


    /***********---LCD PRINTING----***********/
    String OnOff = (fanOn) ? "ON" : "OFF";
    String Dir = (direction == 0) ? "C" : "CC";
    String Speed = getSpeedString(speedLevel);
    String Line1 =  RTC_Time;
    String Line2 = OnOff + " | " + Dir + " | " + Speed;
    LCD.setCursor(0, 0);
    LCD.print(Line1);
    LCD.setCursor(0, 1);
    LCD.print(Line2);
    LCD.print("  ");

}

void translateIR(unsigned long value) {  
  if(!timeEntry) {
    switch (value) {
        case 0xBF40FF00: // Pause/Play = fan on/off
            fanOn = !fanOn;
            Serial.println((fanOn) ? "Fan On" : "Fan Off");
            break;

        case 0xBC43FF00: // Fast Forward = Increase speed
            increaseFanSpeed();
             Serial.println("Speed Increase");
            break;

        case 0xBB44FF00: // Rewind = Decrease speed
            decreaseFanSpeed();
            Serial.println("Speed Decrease");
            break;

        case 0xB847FF00: // FUNC/STOP = time update
            Serial.println("Time Entry:");
            timeEntry = true;
        break;
    }
  } 
  else {
     int digit = getDigitFromIR(value);
     if(digit >= 0) {
          Serial.println(digit);
          TimeEntry[entryPos++] = digit;
          if(entryPos == 6) {

              DateTime Current = RTC.now();
              int Hour = (String(TimeEntry[0]) + String(TimeEntry[1])).toInt();
              int Minute = (String(TimeEntry[2]) + String(TimeEntry[3])).toInt();
              int Second = (String(TimeEntry[4]) + String(TimeEntry[5])).toInt();

              RTC.adjust(DateTime(Current.year(), Current.month(), Current.day(), Hour, Minute, Second));

              entryPos = 0;
              timeEntry = false;
          }
     }
  }
}

int getDigitFromIR(unsigned long value) {
    switch (value) {
        case 0xF30CFF00: return 1;
        case 0xE718FF00: return 2;
        case 0xA15EFF00: return 3;
        case 0xF708FF00: return 4;
        case 0xE31CFF00: return 5;
        case 0xA55AFF00: return 6;
        case 0xBD42FF00: return 7;
        case 0xAD52FF00: return 8;
        case 0xB54AFF00: return 9;
        case 0xE916FF00: return 0;
        default: return -1; // Invalid IR code
    }
}

String getSpeedString(int speed) {
    switch (speed) {
        case 0: return "0";
        case 1: return "1/2";
        case 2: return "3/4";
        case 3: return "Full";
        default: return "";
    }
}
