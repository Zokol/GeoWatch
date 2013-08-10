/*****

GeoWatch-project - Arduino-application
This is prototype of the Arduino-part of GeoWatch. At this state the program is not optimized and is highly unreliable.

GeoWatch is Open Hardware & Open Source project to create better handheld GPS-device for Geocaching.
The prototype-platform uses Arduino Mega equipped with 
 - 128*64 OLED-display [D0]
 - BTM-5 bluetooth shield
 - HMC5883L 3-axis Compass
 - blox NEO-6M GPS shield.

NOTICE!! All of the peripherials connected to the Arduino use 3.3V VCC, so the serial communication requires proper level shifter to handle the communication. With this prototype, transistor based bi-directional 8-channel level shifter is used. 

*****/

// Reference math-library
#include <math.h>
// Adafruit Graphics-library
#include <Adafruit_GFX.h>
// Adafruit OLED-library for SSD1306
#include <Adafruit_SSD1306.h>
// Reference the I2C Library
#include <Wire.h>
// Reference the HMC5883L Compass Library
#include <HMC5883L.h>

//#include "floatToString.h"

// Indicator led connected to PIN30
#define LED 30

// Define pins for display
#define OLED_DC 4
#define OLED_CS 3
#define OLED_CLK 52
#define OLED_MOSI 51
#define OLED_RESET 5

// Define radius of earth, used for coordinate calculations
#define R_EARTH 6371 //km

// Store display variable
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Store compass variable
HMC5883L compass;

// Record any errors that may occur in the compass.
int error = 0;

// Flag to clear display
boolean clear_dsp = false;

// Initialize coordinate and bearing-variables
float homeN = 60.46085;
float homeW = -22.2983;
float targetN = 0.0;
float targetW = 0.0;
float curr_bearing = 180.0;

// Initialize log array and set configs
#define LOG_LENGTH 6
char* cmdlog[LOG_LENGTH];
int log_index = 0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial to PC communication at 9600 bits per second:
  Serial.begin(9600);
  // initialize serial to Bluetooth at 9600 bits per second:
  Serial1.begin(38400);
  initUART();
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(500);
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  //delay(2000);
  
  // initialize the I2C-bus to compass
  Wire.begin();
  
  addLog("Const HMC5883L");
  Serial.println("Constructing new HMC5883L");
  showLog();
  // Construct a new HMC5883 compass.
  compass = HMC5883L();
  
  delay(500);
  addLog("Scale 1.3 Ga");
  Serial.println("Setting scale to +/- 1.3 Ga");
  showLog();
  // Set the scale of the compass.
  error = compass.SetScale(1.3);
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));
  delay(500);
  addLog("Set contious");
  Serial.println("Setting measurement mode to continous.");
  showLog();
  error = compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
  if(error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));
  delay(500);
  
  digitalWrite(LED, LOW);
  display.clearDisplay();

  display.setTextSize(4);
  display.setCursor(0,16);
  display.println("READY");
  display.display();
  delay(1000);
}

// the loop routine runs over and over again forever:
void loop() {
  if(Serial1.available()) {
    if(Serial1.read() == 's'){
      targetN = Serial1.parseFloat();
      targetW = Serial1.parseFloat();
      
      if(clear_dsp){
        display.clearDisplay();
        clear_dsp = false;
      }
      
      float distance = getDistance();
      float bearing = getBearing();
      
      char buffer[30];
      String distance_str = "";
      String bearing_str = "";
      
      if(distance > 0){
        distance_str = floatToString(buffer, distance, 2, 0, false);
        distance_str = distance_str + " km";
      }
      else{
        distance = distance / 100;
        distance_str = floatToString(buffer, distance, 2, 0, false);
        distance_str = distance_str + " m";
      }
      
      bearing_str = floatToString(buffer, bearing, 0, 0, false);
      
      int brng_target = (int) bearing;
      int brng_curr = (int) curr_bearing;
      
      if(brng_target > brng_curr){
        bearing_str = "   " + bearing_str + " >>";
      }
      else if(brng_target == brng_curr){
        bearing_str = " | " + bearing_str + " | ";
      }
      else{
        bearing_str = "<< " + bearing_str;
      }
      
      display.clearDisplay();
      digitalWrite(LED, HIGH);
      display.setTextColor(WHITE);
      display.setTextSize(2);
      display.setCursor(0,0);
      display.println(targetN);
      display.setCursor(0,16);
      display.println(targetW);
      display.setCursor(0,32);
      display.println(distance_str);
      display.setCursor(0,48);
      display.println(bearing_str);
      display.display();
    }
  }
  else {
    //display.display();
    digitalWrite(LED, LOW);
    clear_dsp = true;
  }
}

//Initializes UART in the BT module
void initUART() {
  Serial1.write("AT+ORGL");
  Serial1.write(0x0d);
  Serial1.write(0x0a);
}

void showLog() {
  int x = 0;
  int y = 0;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  for (int i = 0; i < LOG_LENGTH; i++){
    display.println(cmdlog[i]);
    //y = y + 10;
  }
  display.display();
}

void addLog(char* str) {
  cmdlog[log_index] = str;
  log_index = (log_index + 1) % LOG_LENGTH;
}

void clearLog() {
  log_index = 0;
  for (int i = 0; i < LOG_LENGTH; i++){
    cmdlog[i] = "";
  }
}

float getDistance(){
    float dLat = toRad(targetN - homeN);
    float dLon = toRad(targetW - homeW);
    float lat1 = toRad(homeN);
    float lat2 = toRad(targetN);
    
    float a = sin(dLat/2) * sin(dLat/2) + sin(dLon/2) * sin(dLon/2) * cos(lat1) * cos(lat2); 
    float c = 2 * atan2(sqrt(a), sqrt(1-a)); 
    float d = R_EARTH * c;
    
    return d;
}

float getBearing(){
    float dLat = toRad(targetN - homeN);
    float dLon = toRad(targetW - homeW);
    float lat1 = toRad(homeN);
    float lat2 = toRad(targetN);
    
    float y = sin(dLon) * cos(lat2);
    float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    float brng = toDeg(atan2(y, x));
    
    brng = fmod((brng + 360.0),360.0);
    return brng;
}

float toRad(float a){
    return a * 1000 / 57296;
}

float toDeg(float a){
    return a * 57296 / 1000;
}

char * floatToString(char * outstr, float value, int places, int minwidth, bool rightjustify) {
    // this is used to write a float value to string, outstr.  oustr is also the return value.
    int digit;
    float tens = 0.1;
    int tenscount = 0;
    int i;
    float tempfloat = value;
    int c = 0;
    int charcount = 1;
    int extra = 0;
    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
    // if this rounding step isn't here, the value  54.321 prints as 54.3209

    // calculate rounding term d:   0.5/pow(10,places)  
    float d = 0.5;
    if (value < 0)
        d *= -1.0;
    // divide by ten for each decimal place
    for (i = 0; i < places; i++)
        d/= 10.0;    
    // this small addition, combined with truncation will round our values properly 
    tempfloat +=  d;

    // first get value tens to be the large power of ten less than value    
    if (value < 0)
        tempfloat *= -1.0;
    while ((tens * 10.0) <= tempfloat) {
        tens *= 10.0;
        tenscount += 1;
    }

    if (tenscount > 0)
        charcount += tenscount;
    else
        charcount += 1;

    if (value < 0)
        charcount += 1;
    charcount += 1 + places;

    minwidth += 1; // both count the null final character
    if (minwidth > charcount){        
        extra = minwidth - charcount;
        charcount = minwidth;
    }

    if (extra > 0 and rightjustify) {
        for (int i = 0; i< extra; i++) {
            outstr[c++] = ' ';
        }
    }

    // write out the negative if needed
    if (value < 0)
        outstr[c++] = '-';

    if (tenscount == 0) 
        outstr[c++] = '0';

    for (i=0; i< tenscount; i++) {
        digit = (int) (tempfloat/tens);
        itoa(digit, &outstr[c++], 10);
        tempfloat = tempfloat - ((float)digit * tens);
        tens /= 10.0;
    }

    // if no places after decimal, stop now and return

    // otherwise, write the point and continue on
    if (places > 0)
    outstr[c++] = '.';


    // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
    for (i = 0; i < places; i++) {
        tempfloat *= 10.0; 
        digit = (int) tempfloat;
        itoa(digit, &outstr[c++], 10);
        // once written, subtract off that digit
        tempfloat = tempfloat - (float) digit; 
    }
    if (extra > 0 and not rightjustify) {
        for (int i = 0; i< extra; i++) {
            outstr[c++] = ' ';
        }
    }


    outstr[c++] = '\0';
    return outstr;
}
