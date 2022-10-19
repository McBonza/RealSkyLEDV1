/* 
 *  Copyright (C) 2022 Jeremy Constantin Lucas - All Rights Reserved
 *  You may not use, distribute and/or modify this code without my 
 *  explicit permission, with the exception of personal use.
 *  This code is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES 
 *  OR CONDITIONS OF ANY KIND, either express or implied.
 */


#include <WiFi.h>
#include <FastLED.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include "time.h"

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
#define TFT_GREY 0x5AEB // New colour

#define LED_PIN     27
#define NUM_LEDS    300 
#define BRIGHTNESS  255
#define FRAMES_PER_SECOND 60
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
CRGB nextFrameLeds[NUM_LEDS];

const char* ssid       = "DonutA103-2.4";
const char* password   = "DoughNut";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * (-8);
const int   daylightOffset_sec = 3600 * 0;

struct tm timeinfo;

                      
const int sunriseTimes [12] [2] = { {7, 57},
                                    {7, 33},
                                    {6, 46},
                                    {5, 44},
                                    {4, 49},
                                    {4, 14},
                                    {4, 16},
                                    {4, 48},
                                    {5, 29},
                                    {6, 10},
                                    {6, 55},
                                    {7, 38} };
                                   
const int sunsetTimes [12] [2] = { {16, 29},
                                   {17, 11},
                                   {17, 56},
                                   {18, 40},
                                   {19, 23},
                                   {20, 00},
                                   {20, 10},
                                   {19, 42},
                                   {18, 47},
                                   {17, 46},
                                   {16, 49},
                                   {16, 19} };

                                   
int sunsetTime = 0;
int sunriseTime = 0;
int skyState = 0; 
int turbulance = 0;
/* 
 *0 = NOTHING
 *1 = SUN GOING UP
 *2 = SUN IN SKY
 *3 = SUN GOING DOWN
 *4 = BLUE HOUR NIGHT START
 *5 = NIGHT MOON UP
 *6 = NIGHT NO MOON
 *7 = BLUE HOUR NIGHT END
  */
  
//in minutes
const int sunGoingUpTime = 60;
const int sunGoingDownTime = 60;
const int blueHourNightStartTime = 60;
const int nightNoMoonTime = 120;
const int blueHourNightEndTime = 60;

/**** COLOR PALETTES ****/
/* [0] is color1(center) and [1] is color2(at the sides)  */
const float skyColors [7][2][3]               = { { {220, 220, 40} , {110, 150, 255} },   // sun Going Up 
                                                  { {255, 254, 100} , {120, 199, 255} },  // mid Day Sun 
                                                  { {255, 90, 0} , {255, 0, 0} },     // sun Going Down 
                                                  { {70, 48, 100} , {52, 120, 180} },     // blue HourNight Start 
                                                  { {100, 20, 80} , {0, 0, 70} },     // mid Night yes moon
                                                  { {50, 0, 50} , {0, 0, 50} },      // night No Moon 
                                                  { {200, 109, 18} , {252, 233, 174} } }; // blue Hour Night End 




              
// Returns the amount of minutes since 00:00  (01:05 will be 65)
int getSunsetTime(){
  int monthNB = timeinfo.tm_mon;
  int dayNB = timeinfo.tm_mday;
  float returnVal = 0.0;

  if(monthNB < 11) {
    returnVal = ( (((sunsetTimes[monthNB][0] * 60.0 + sunsetTimes[monthNB][1] ) * (31.0 - dayNB))   + ((sunsetTimes[monthNB+1][0] * 60.0 + sunsetTimes[monthNB+1][1] ) * dayNB ) )  / (31.0));
  }else{
    returnVal = ( (((sunsetTimes[11][0] * 60.0 + sunsetTimes[11][1] ) * (31.0 - dayNB))   + ((sunsetTimes[0][0] * 60.0 + sunsetTimes[0][1] ) * dayNB ) )  / (31.0));
  }
  
  tft.print("SunSet Time: ");
  tft.print((int)(returnVal / 60.0));
  tft.print(":");
  tft.println((int)returnVal % 60);
  
  return returnVal;
}

// Returns the amount of minutes since 00:00  (01:05 will be 65)
int getSunriseTime(){
  int monthNB = timeinfo.tm_mon;
  int dayNB = timeinfo.tm_mday;
  float returnVal = 0.0;

  if(monthNB < 11) {
    returnVal = ( (((sunriseTimes[monthNB][0] * 60.0 + sunriseTimes[monthNB][1] ) * (31.0 - dayNB))   + ((sunriseTimes[monthNB+1][0] * 60.0 + sunriseTimes[monthNB+1][1] ) * dayNB ) )  / (31.0));
  }else{
    returnVal = ( (((sunriseTimes[11][0] * 60.0 + sunriseTimes[11][1] ) * (31.0 - dayNB))    + ((sunriseTimes[0][0] * 60.0 + sunriseTimes[0][1] ) * dayNB ) )   / (31.0));
  }
  
  tft.print("SunRise Time: ");
  tft.print((int)(returnVal / 60.0));
  tft.print(":");
  tft.println((int)returnVal % 60);
  
  return returnVal;
}
 
void printLocalTime()
{
  if(!getLocalTime(&timeinfo)){
    // We can now plot text on screen using the "print" class
    tft.println("Failed to obtain time");
    return;
  }
  tft.println(&timeinfo, "%B %d %Y %H:%M:%S");
}


void colorEffect(int centerIndex, const float startColor[3], const float endColor[3], float variation){

  centerIndex = max(2, min(NUM_LEDS-2, centerIndex));

  //Color Random Variation
  float CRV1[3] = {random(0, 100)/(-100.0), random(0, 100)/(-100.0), random(0, 100)/(-100.0)};
  nextFrameLeds[centerIndex]   =  CRGB( max( ((nextFrameLeds[centerIndex][0] + startColor[0])/2 + CRV1[0]*variation), 0.0f) ,
                                        max( ((nextFrameLeds[centerIndex][1] + startColor[1])/2 + CRV1[0]*variation), 0.0f) ,
                                        max( ((nextFrameLeds[centerIndex][2] + startColor[2])/2 + CRV1[0]*variation), 0.0f) ); 
  nextFrameLeds[centerIndex+1] = nextFrameLeds[centerIndex]  ;
  

  //side 1
  for( int i = 0; i < centerIndex; ++i) {
        // transmit the color
        nextFrameLeds[i] = nextFrameLeds[i+1];
        //fade into end color slowly
        nextFrameLeds[i][0] = (nextFrameLeds[i][0]*(NUM_LEDS/2)+endColor[0]) / (NUM_LEDS/2+1) ;
        nextFrameLeds[i][1] = (nextFrameLeds[i][1]*(NUM_LEDS/2)+endColor[1]) / (NUM_LEDS/2+1) ;
        nextFrameLeds[i][2] = (nextFrameLeds[i][2]*(NUM_LEDS/2)+endColor[2]) / (NUM_LEDS/2+1) ;
  }

  //side 2
  for( int i = NUM_LEDS-1; i > centerIndex+1; --i) {
        // transmit the color
        nextFrameLeds[i] = nextFrameLeds[i-1];
        //fade into end color slowly
        nextFrameLeds[i][0] = (nextFrameLeds[i][0]*(NUM_LEDS/2)+endColor[0]) / (NUM_LEDS/2+1) ;
        nextFrameLeds[i][1] = (nextFrameLeds[i][1]*(NUM_LEDS/2)+endColor[1]) / (NUM_LEDS/2+1) ;
        nextFrameLeds[i][2] = (nextFrameLeds[i][2]*(NUM_LEDS/2)+endColor[2]) / (NUM_LEDS/2+1) ;
  }
  
  //WRITE DOWN NEW COLORS TO THE ACTUAL ARRAY
  for(int i = 0 ; i < NUM_LEDS ; i++)
    leds[i] = nextFrameLeds[i];
}

void displaySatellite(int satellitePos, int satelliteSize, int satelliteColor[3]){

  if(satelliteSize < 1) return;
  
  if(satellitePos >= NUM_LEDS)
    satellitePos = NUM_LEDS-1;
    
  if(satellitePos < 0)
    satellitePos = 0;
  
  leds[satellitePos][0] = satelliteColor[0];
  leds[satellitePos][1] = satelliteColor[1];
  leds[satellitePos][2] = satelliteColor[2];

  int aheadIndex = satellitePos+1;
  int behindIndex = satellitePos-1;
  
  for(int sizeRemaining = satelliteSize-1; sizeRemaining > 0 ; sizeRemaining--){
    if(aheadIndex < NUM_LEDS){
      leds[aheadIndex][0] = satelliteColor[0];
      leds[aheadIndex][1] = satelliteColor[1];
      leds[aheadIndex][2] = satelliteColor[2];
      aheadIndex++;
      sizeRemaining--;
    }
    
    if(behindIndex >= 0){
      leds[behindIndex][0] = satelliteColor[0];
      leds[behindIndex][1] = satelliteColor[1];
      leds[behindIndex][2] = satelliteColor[2];
      behindIndex--;
      sizeRemaining--;
    }
  }


}


 
void setup()
{
  //setup OLED Screen
  tft.init();
  tft.setRotation(1);
  
  Serial.begin(115200);
  
  tft.fillScreen(TFT_GREY);
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  
  tft.setTextSize(1);
  
  //connect to WiFi
  tft.printf("Connecting to %s ", ssid);
  tft.println(" ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      tft.print(".");
  }
  tft.println("..CONNECTED");

  tft.println("  ");
  tft.println(" INITIALIZING LEDS...");
  delay( 1000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
 
  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  //refresh screen 
  tft.fillScreen(TFT_BLACK);
  
  delay(1000);
}
 
void loop()
{
  
  
//PROCESS SCREEN AND TIME

  //CURSOR RESET
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_ORANGE,TFT_BLACK);  tft.setTextSize(1.2);

  //PRINT ON SCREEN
  printLocalTime();
  
  sunsetTime = getSunsetTime();
  sunriseTime = getSunriseTime();
//



  

//PROCESS LEDs

  executeLightBehavior();      


//DISPLAY    
  FastLED.show(); // display this frame
 // FastLED.delay(1000 / FRAMES_PER_SECOND);
  
}

   
// decide how light pattern moves
void executeLightBehavior(){

  /*********************** DECIDE SKY STATE ***********************/
  //RECORD TIMES SINCE AND TILL SUNSET 
  int timeTillSunset = 999999;
  int timeTillSunrise = 999999;
  int timeSinceSunset = 999999;
  int timeSinceSunrise = 999999;

  // check if sunset or sunrise is closer then decide skyState
  // if during daytime, sunset is closer
  if(timeinfo.tm_hour*60 + timeinfo.tm_min > sunriseTime && timeinfo.tm_hour*60 + timeinfo.tm_min < sunsetTime ){
     timeTillSunset = sunsetTime - timeinfo.tm_hour*60 - timeinfo.tm_min;
     timeSinceSunrise = timeinfo.tm_hour*60 + timeinfo.tm_min - sunriseTime;
  }
  // else sunrise is closer, but check if we are before or after midnight
  else{
    // if it isnt past midnight yet
    if(timeinfo.tm_hour*60 + timeinfo.tm_min > sunriseTime){
      timeTillSunrise = sunriseTime + (24*60) - (timeinfo.tm_hour*60 + timeinfo.tm_min);
      timeSinceSunset = timeinfo.tm_hour*60 + timeinfo.tm_min - sunsetTime;
    }
    //it is past midnight
    else{ 
      timeTillSunrise = sunriseTime - timeinfo.tm_hour*60 - timeinfo.tm_min;
      timeSinceSunset = timeinfo.tm_hour*60 + timeinfo.tm_min + (24*60) - sunsetTime;
    }
  }

  // SUNSET/SUNRISE TIMES DEBUG PRINTS
  if(timeSinceSunrise < timeSinceSunset){
    tft.print("Time Since Sunrise: ");  
    tft.print((int)(timeSinceSunrise / 60.0));
    tft.print(":");
    tft.println((int)timeSinceSunrise % 60);
  }else{
    tft.print("Time Since Sunset: ");
    tft.print((int)(timeSinceSunset / 60.0));
    tft.print(":");
    tft.println((int)timeSinceSunset % 60);
  }
  if(timeTillSunrise < timeTillSunset){
    tft.print("Time Till Sunrise: ");
    tft.print((int)(timeTillSunrise / 60.0));
    tft.print(":");
    tft.println((int)timeTillSunrise % 60);
  }else{
    tft.print("Time Till Sunset: ");
    tft.print((int)(timeTillSunset / 60.0));
    tft.print(":");
    tft.println((int)timeTillSunset % 60);
  }   

   // DECIDE THE STATE
   /* 
   *0 = NOTHING
   *1 = SUN GOING UP
   *2 = SUN IN SKY
   *3 = SUN GOING DOWN
   *4 = BLUE HOUR NIGHT START
   *5 = NIGHT MOON UP
   *6 = NIGHT NO MOON
   *7 = BLUE HOUR NIGHT END
   */
   if(timeSinceSunrise < timeSinceSunset){
      if(timeSinceSunrise < sunGoingUpTime)
        skyState = 1;
      else if(timeTillSunset < sunGoingDownTime)
        skyState = 3;
      else
        skyState = 2;
   }
   else{
     if(timeSinceSunset < blueHourNightStartTime)
        skyState = 4;
      else if(timeTillSunrise < blueHourNightEndTime)
        skyState = 7;
      else if(timeTillSunrise < nightNoMoonTime + blueHourNightEndTime)
        skyState = 6;
      else
        skyState = 5;
   }
   
  tft.print("Sky State: ");
  tft.println(skyState);

   
  /*********************** DECIDE COLOR PALETTE ***********************/

    
  /* 
   *0 = NOTHING
   *1 = SUN GOING UP
   *2 = SUN IN SKY
   *3 = SUN GOING DOWN
   *4 = BLUE HOUR NIGHT START
   *5 = NIGHT MOON UP
   *6 = NIGHT NO MOON
   *7 = BLUE HOUR NIGHT END
   *
    const int sunGoingUpTime = 60;
    const int sunGoingDownTime = 60;
    const int blueHourNightStartTime = 60;
    const int nightNoMoonTime = 120;
    const int blueHourNightEndTime = 60;
   */
   
  float mixVal = 0;
  int colorIndex1 = 0;
  int colorIndex2 = 0;
  int satellitePos = (NUM_LEDS/2);
  int satelliteSize = 0;
  int satelliteColor[3] = {0,0,0};
  
  switch(skyState){
    
    case 1:
      mixVal = (((float)sunGoingUpTime - (float)timeSinceSunrise) / (float)sunGoingUpTime); 
      colorIndex1 = 6;
      colorIndex2 = 0;
      turbulance = 150 - (1.0-mixVal) *100;
      
      if(timeSinceSunrise >= sunGoingUpTime / 10)
        satelliteSize = (2 + (timeSinceSunrise*10 / sunGoingUpTime) * 12) * (NUM_LEDS / 300.0) ;
      else
        satelliteSize = (14 - ((timeSinceSunrise - (sunGoingUpTime / 10)) / sunGoingUpTime)) * (NUM_LEDS / 300.0);

      satelliteColor[0] = 255;
      satelliteColor[1] = 50 + (float)timeSinceSunrise;
      satelliteColor[2] = 0 + (float)timeSinceSunrise;
      
      satellitePos = (NUM_LEDS/2) + (NUM_LEDS/2) * (1.0-mixVal);
      break;
      
    case 2:
      mixVal = max(0.0,((float)sunGoingUpTime*5.0 - timeSinceSunrise) / ((float)sunGoingUpTime*4.0)); 
      colorIndex1 = 0;
      colorIndex2 = 1;
      turbulance =  50;
      
      satelliteSize = 8 * (NUM_LEDS / 300.0);
      
      satelliteColor[0] = 255;
      satelliteColor[1] = min(50 + (float)timeSinceSunrise, 230.0f); 
      satelliteColor[2] = min(0 + (float)timeSinceSunrise, 200.0f);
      
      satellitePos = (NUM_LEDS) -  (NUM_LEDS * (1.0-mixVal));
      break;
      
    case 3:
      mixVal = ((float)timeTillSunset / (float)sunGoingDownTime); 
      colorIndex1 = 1;
      colorIndex2 = 2;
      satelliteSize = 1 + (14 - ((timeTillSunset - (sunGoingDownTime / 10)) / sunGoingDownTime)) * (NUM_LEDS / 300.0);
      turbulance = 150 - mixVal *100;
      
      satelliteColor[0] = 255;
      satelliteColor[1] = 230 - ((1.0-mixVal) * 180); 
      satelliteColor[2] = 200 - ((1.0-mixVal) * 200);
      satellitePos = (NUM_LEDS/2 * (1.0-mixVal));
      break;
      
    case 4:
      mixVal = (((float)blueHourNightStartTime - (float)timeSinceSunset) / (float)blueHourNightStartTime); 
      colorIndex1 = 2;
      colorIndex2 = 3;
      turbulance = 120 - (1.0-mixVal) *90;
      satelliteSize = 0 * (NUM_LEDS / 300.0);
      satellitePos = (NUM_LEDS/2* mixVal);
      break;
      
    case 5:
      mixVal = min(1.0 , max(0.0,((float)blueHourNightStartTime*5.0 - timeSinceSunset) / ((float)blueHourNightStartTime*4.0))); 
      colorIndex1 = 3;
      colorIndex2 = 4;
      satelliteSize = 10 * (NUM_LEDS / 300.0);
      turbulance = 30;
      
      satelliteColor[0] = 230;
      satelliteColor[1] = 255 - (mixVal * 50); 
      satelliteColor[2] = 255 - (mixVal * 100);
                          
      satellitePos = NUM_LEDS * (1.0f - mixVal);
      break;
      
    case 6:
      mixVal = ((float)timeTillSunrise / ((float)nightNoMoonTime + (float)blueHourNightEndTime)); 
      mixVal = mixVal*mixVal;
      colorIndex1 = 4;
      colorIndex2 = 5;
      turbulance = 10;
      satelliteSize = 0 * (NUM_LEDS / 300.0);
      satellitePos = (NUM_LEDS/2) + (NUM_LEDS/2) * (mixVal);
      break;
      
    case 7:
      mixVal = ((float)timeTillSunrise / (float)blueHourNightEndTime); 
      colorIndex1 = 5;
      colorIndex2 = 6;
      turbulance =  150 - mixVal *140;
      satelliteSize = 0 * (NUM_LEDS / 300.0);
      satellitePos = (NUM_LEDS/2);
      break;
    
    default:
      break;
  }

  Serial.println(mixVal);

  float skyColorPalette [2][3] = { 
    { (skyColors[colorIndex1][0][0]*mixVal + skyColors[colorIndex2][0][0]*(1.0 - mixVal)), 
      (skyColors[colorIndex1][0][1]*mixVal + skyColors[colorIndex2][0][1]*(1.0 - mixVal)), 
      (skyColors[colorIndex1][0][2]*mixVal + skyColors[colorIndex2][0][2]*(1.0 - mixVal))} 
    , 
    { (skyColors[colorIndex1][1][0]*mixVal + skyColors[colorIndex2][1][0]*(1.0 - mixVal)), 
      (skyColors[colorIndex1][1][1]*mixVal + skyColors[colorIndex2][1][1]*(1.0 - mixVal)), 
      (skyColors[colorIndex1][1][2]*mixVal + skyColors[colorIndex2][1][2]*(1.0 - mixVal))}   }; 

  //debug prints
  tft.print("Start Color: ("); tft.print(skyColorPalette[0][0]); tft.print(", "); tft.print(skyColorPalette[0][1]); tft.print(", ");  tft.print(skyColorPalette[0][2]); tft.println(")");  
  tft.print("End Color: (");  tft.print(skyColorPalette[1][0]); tft.print(", ");  tft.print(skyColorPalette[1][1]); tft.print(", "); tft.print(skyColorPalette[1][2]);  tft.println(")");

  //apply color effect
  colorEffect(satellitePos, skyColorPalette[0], skyColorPalette[1], turbulance);

  //paint in the satellite (sun or moon) if there are any
  displaySatellite(satellitePos, satelliteSize, satelliteColor);
  
}
