#include <math.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include "floatToString.h"

#define OLED_DC 4
#define OLED_CS 3
#define OLED_CLK 52
#define OLED_MOSI 51
#define OLED_RESET 5
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
#define R_EARTH 6371 //km

int led = 30;
boolean clear_dsp = false;
float homeN = 60.46085;
float homeW = -22.2983;
float targetN = 0.0;
float targetW = 0.0;
float curr_bearing = 180.0;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial to PC communication at 9600 bits per second:
  Serial.begin(9600);
  // initialize serial to Bluetooth at 9600 bits per second:
  Serial1.begin(38400);
  initUART();
  
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  delay(500);
  digitalWrite(led, LOW);
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(6);
  display.setCursor(10,20);
  display.println("GPS");
  display.display();
  delay(2000);
  display.clearDisplay();
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
      
      digitalWrite(led, HIGH);
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
    display.display();
    digitalWrite(led, LOW);
    clear_dsp = true;
  }
}

//Initializes UART in the BT module
void initUART() {
  Serial1.write("AT+ORGL");
  Serial1.write(0x0d);
  Serial1.write(0x0a);
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
