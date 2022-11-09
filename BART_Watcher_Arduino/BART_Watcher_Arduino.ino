
#include <WiFiManager.h>              // https://github.com/tzapu/WiFiManager WiFi Configuration Magic (v. 2.0.3-alpha)
#include <ArduinoJson.h>              // https://github.com/bblanchon/ArduinoJson (v. 6.7.13)
#include <ArduinoOTA.h>               // https://github.com/jandrassy/ArduinoOTA (v. 1.0.5)
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "SSD1306Wire.h"              // https://github.com/ThingPulse/esp8266-oled-ssd1306

const int buttonPin = D5;

const char compile_date[] = __DATE__ " " __TIME__;


// Currently using the shared legacy API key, https://www.bart.gov/schedules/developers/api
// this may be revoked at any time, get your own personal key
String bartApiKey = "MW9S-E7SL-26DU-VV8V";

// Line height for each font size
int smallLineHeight = 11;
int bigLineHeight = 16;

// Global display instance 
SSD1306Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL

// Global Wifi Manager instance
WiFiManager wifiManager;

String startingStation = "EMBR";
String endingStation = "NBRK";
String directionNeeded = "North";
bool redLineOption = true;
bool redLinePreferred = true;
bool orangeLineOption = true;
bool orangeLinePreferred = true;
bool yellowLineOption = true;
bool yellowLinePreferred = false;
bool blueLineOption = false;
bool blueLinePreferred = false;
bool greenLineOption = false;
bool greenLinePreferred = false;
int notEnoughMinutes = 7;
int tooManyMinutes = 25;


void setup() {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(50);
  Serial.println('\n');
  Serial.print( "Built: " );
  Serial.println(compile_date);
  Serial.println("entering setup()");

  Serial.println("starting Wifi manager");
  String mac = WiFi.macAddress();
  mac.replace( ":", "" );
  Serial.print("Got MAC address: " );
  Serial.println(WiFi.macAddress());
  Serial.print("Got IP address: " );
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
  wifiManager.autoConnect();

  // wifiManager.setAPCallback(configodeCallback);
  // wifiManager.setSaveConfigCallback(saveConfigCallback);

  Serial.println("Initializing SSD1306");
  
  display.init();
  display.normalDisplay();
  display.flipScreenVertically();
  display.displayOn();

  // Uncomment to pull new station list for paste into code (usually one-off)
  // parseStationNames();

  pinMode(buttonPin, INPUT);

  // Load initial schedule
  refreshStationData();

  if( digitalRead(buttonPin) ){
    Serial.println("Button is pushed");
  }
  else{
    Serial.println("Button is not pushed");
  }
}

unsigned long lastButtonDown = 0;
unsigned long lastScheduleUpdate = 0;

void loop() {
  static bool isActive = true;
  static bool inMenu = false;

  if( inMenu == true ){
    if( inMenu = menuLoop() ){
      // still in menu mode
    }
    else{
      // exiting menu mode, force refresh of schedule
      display.clear();  
      display.display();        
      lastScheduleUpdate = 0;
      blinkScreen( 7, 50 );
    }
    return;
  }

  // Check for button press
  if( digitalRead(buttonPin) ){
    if( lastButtonDown != 0 ){
      // button is pressed, long enough to enter menu mode?
      if( millis() - lastButtonDown > 3000 ){
        Serial.println("Entering menu mode");
        lastButtonDown = 0;
        display.displayOn();
        isActive = true;     
        inMenu = true;
        blinkScreen( 7, 50 );
      }
    }
    else{
      lastButtonDown = millis();
    }
  }  
  else{
    // short press?
    if( ( lastButtonDown != 0 ) && ( millis() - lastButtonDown > 50 ) ){
      // switch between active / inactive
      if( isActive ){
        display.displayOff();
        isActive = false;
      }
      else{
        display.displayOn();
        isActive = true;      }
    }
    lastButtonDown = 0;
  }

  
  if( isActive ){
    // Check if time to update schedule
    if( millis() - lastScheduleUpdate > 30 * 1000 ){
      if( refreshStationData() > 0 ){
        blinkScreen( 3, 150 );
      }
      lastScheduleUpdate = millis();
    }
  }
}


bool menuLoop() {
  static int menuItem = 0;
  static int drawnMenuItem = -1;

  if( drawnMenuItem != menuItem ){
    renderMenu( menuItem );
    drawnMenuItem = menuItem; 
  }

  // Check for button press
  if( digitalRead(buttonPin) ){
    if( lastButtonDown != 0 ){
      // check for long press
      if( millis() - lastButtonDown > 3000 ){
        // Select menu item
        switch( menuItem ){
          case 0:
            setStations( "NBRK", "EMBR" );
            break;
          case 1:
            setStations( "EMBR", "NBRK" );
            break;
          case 2: 
            Serial.println("Erasing wifi config, restarting");
            wifiManager.resetSettings();
            ESP.restart();
            break;
          case 3:
            Serial.println("Exiting menu mode");
            break;
        }
        lastButtonDown = 0;
        drawnMenuItem = -1;
        return( false );
      }
    }
    else{
      lastButtonDown = millis();
    }
  }
  else{
    if( ( lastButtonDown != 0 ) && ( millis() - lastButtonDown > 50 ) ){
      // short press
      if( ++menuItem > 3 ) menuItem = 0;
    }
    lastButtonDown = 0;
  }
  
  return( true );
}

/*
 *  Draw the menu
 *  highlight = menu item to highlight, 0 = first item
 */
void renderMenu( int highlight ){
  highlight++;
  
  display.clear();  
  display.setColor( WHITE );        
  display.setFont(ArialMT_Plain_10);
  int yPos = 0; 

  display.setTextAlignment( TEXT_ALIGN_RIGHT );
  display.drawString( 128, yPos, "Menu" );
  
  // On dual-color displays, this moves schedule to second color of screen   
  yPos += smallLineHeight + 4;
  
  // Line looks fancy
  display.drawLine( 0, 12, 128, 12 );
  
  display.setTextAlignment( TEXT_ALIGN_LEFT );
  display.drawString( 0, yPos, "NBRK -> EMBR" );
  yPos += smallLineHeight;
  display.drawString( 0, yPos, "EMBR -> NBRK" );
  yPos += smallLineHeight;
  display.drawString( 0, yPos, "Reset WIFI" );
   yPos += smallLineHeight;
  display.drawString( 0, yPos, "Exit Menu" );
 
  display.setColor( INVERSE );
  display.fillRect(0, ( highlight*smallLineHeight ) + 5, 128, smallLineHeight);
  display.display();
}

/*
 *  Switch between inverted and normal screen, ideally to draw attention
 *  blinks: the nymber of normal / invert cycles
 *  msecBetween: millis between each tansition (2 per cycle)
 */
void blinkScreen( int blinks, int msecBetween ) {

  for( int i = 0; i < blinks; i++ ){
    display.invertDisplay();
    delay( msecBetween );
    display.normalDisplay();
    delay( msecBetween );
  }
}

/*
 *  Pull updated data from BART, render relevant departure infomration to screen 
 *  returns non-zero if there is a departure on a preferred line in the ideal 
 *  departure time 
 */
int refreshStationData() {
  String url = String( "http://api.bart.gov/api/etd.aspx?cmd=etd&orig=" + startingStation + "&key="  + bartApiKey + "&json=y" );
  String bartresponse = httpGETRequest( url.c_str() );
  // Serial.println(bartresponse);
  
  DynamicJsonDocument doc(20000);
  deserializeJson(doc, bartresponse);

  if ( doc.isNull() ) {
    Serial.println("Parsing JSON real time departures failed!");
    return( 0 );
  }

  int preferredTrains = 0;
  int yPos = 0;

  display.clear();  
  display.setColor( WHITE );        
  display.setFont(ArialMT_Plain_10);

  // Show start and end stations
  display.setTextAlignment( TEXT_ALIGN_RIGHT );
  display.drawString( 128, 0, endingStation );
  // annoying I can't find a character for an arrow
  int xOff = display.getStringWidth( endingStation );
  xOff += 3;
  display.fillTriangle( 128-xOff, 6, 128-xOff-5, 3, 128-xOff-5, 9 );  
  display.drawString( 128-xOff-7, 0, String( startingStation ) );
  
  // Show date & time of data pull
  display.setTextAlignment( TEXT_ALIGN_LEFT );
  String datetime = String( doc["root"]["date"] ).substring(0,5) + " " + String( doc["root"]["time"] ).substring(0,5);
  Serial.println(datetime);
  display.drawString( 0, 0, datetime );

  // Line looks fancy
  display.drawLine( 0, 12, 128, 12 );

  // On dual-color displays, this moves schedule to second color of screen   
  yPos += smallLineHeight + 3;

  display.setTextAlignment( TEXT_ALIGN_LEFT );

  for( int i=0; i < doc["root"]["station"][ 0 ]["etd"].size(); i++ ){

    JsonObject dest = doc["root"]["station"][ 0 ]["etd"][ i ];

    // Is the destination relevant and preferred?
    bool skip = false;
    bool preferred = false;

    // Okay to look at just first item as all trains w/ destination share direction / line
    JsonObject sample = dest["estimate"][ 0 ];

    // Only show trains going in the correct direction
    if( ! String( sample[ "direction" ] ).equalsIgnoreCase( directionNeeded ) ){
      skip = true; 
    }

    // See if this line is an option, and if so, is it the preferred line?
    if( String( sample[ "color" ] ).equalsIgnoreCase( "RED" ) ){
      if( ! redLineOption ) skip = true;
      if( redLinePreferred ) preferred = true;
    }
    else if( String( sample[ "color" ] ).equalsIgnoreCase( "ORANGE" ) ){
      if( ! orangeLineOption ) skip = true;
      if( orangeLinePreferred ) preferred = true;
    }
    else if( String( sample[ "color" ] ).equalsIgnoreCase( "YELLOW" ) ){
      if( ! yellowLineOption ) skip = true;
      if( yellowLinePreferred ) preferred = true;
    }
    else if( String( sample[ "color" ] ).equalsIgnoreCase( "BLUE" ) ){
      if( ! blueLineOption ) skip = true;
      if( blueLinePreferred ) preferred = true;
    }
    else if( String( sample[ "color" ] ).equalsIgnoreCase( "GREEN" ) ){
      if( ! greenLineOption ) skip = true;
      if( greenLinePreferred ) preferred = true;
    }

    if( ! skip ){
      if( preferred ){
        preferredTrains += updateDestination( dest, 0, yPos, true );
        yPos += bigLineHeight;
      }
      else{
        updateDestination( dest, 0, yPos, false );
        yPos += smallLineHeight;
      }
    }
  }
  display.display();      // Show updated information on screen
  
  return( preferredTrains );
}

/*
 *  Display a single line of a destination, stations name and minutes to departure
 *  returns: the numoer of trains that are in the ideal departure timeframe
 */
int updateDestination( JsonObject dest, int x, int y, bool large ){
  if( large ){
    display.setFont(ArialMT_Plain_16);
  }
  else{
    display.setFont(ArialMT_Plain_10);
  }
  
  display.drawString( x, y, String( dest["abbreviation"] ) );
  x += display.getStringWidth( String( dest["abbreviation"] ) + "  " );

  int idealSchedule = 0;

  JsonArray estimates = dest["estimate"];
  for( int j=0; j < estimates.size(); j++ ){
    
    String output;
    
    if( int( estimates[j]["cancelflag"] ) != 0 ){
      output = "X";
    }
    else if( String( estimates[j]["minutes"]).equalsIgnoreCase( "Leaving" ) ){
      output = "L";
    }
    else{
      int mins = int( estimates[j]["minutes"] );
      output = String( mins );
      // See if this is an ideal / sweet spot train, not too soon to catch, not too long to wait
      if( mins >= notEnoughMinutes && mins <= tooManyMinutes ){
        idealSchedule++;
        int width = display.getStringWidth( output );
        if( large ){
          display.fillRect( x-2, y+2, width+4, 17-3);
        }
        else{
          display.fillRect( x-1, y+2, width+2, 12-3);
        }
        display.setColor( BLACK );        // Draw white text
      }
    }
    display.drawString( x, y, output );
    display.setColor( WHITE );        // Draw white text

    x += display.getStringWidth( output + "  " );
    
  }  
  return( idealSchedule );
}

/*
 *  Super kluge - update parameters based on souce and destination station codes
 *  very hacky for now
 *  returns true if combination supported, false otherwise
 */
bool setStations( String starting, String ending ){

  if( ( starting == "NBRK" ) && ( ending == "EMBR" ) ){
    startingStation = "NBRK";
    endingStation = "EMBR";
    directionNeeded = "South";
    redLineOption = true;
    redLinePreferred = true;
    orangeLineOption = true;
    orangeLinePreferred = false;
    yellowLineOption = false;
    yellowLinePreferred = false;
    blueLineOption = false;
    blueLinePreferred = false;
    greenLineOption = false;
    greenLinePreferred = false;
    notEnoughMinutes = 7;
    tooManyMinutes = 15;
    return( true );
  }

  if( ( starting == "EMBR" ) && ( ending == "NBRK" ) ){
    startingStation = "EMBR";
    endingStation = "NBRK";
    directionNeeded = "North";
    redLineOption = true;
    redLinePreferred = true;
    orangeLineOption = true;
    orangeLinePreferred = true;
    yellowLineOption = true;
    yellowLinePreferred = false;
    blueLineOption = false;
    blueLinePreferred = false;
    greenLineOption = false;
    greenLinePreferred = false;
    notEnoughMinutes = 7;
    tooManyMinutes = 25;
    return( true );
  }

  return( false );
}


/*
 *  Unused for now, clumsy way to get station code from name
 */
String stationAbbreviation( String name ){
  if( name.equalsIgnoreCase( "12th St. Oakland City Center" ) ) return "12TH";
  if( name.equalsIgnoreCase( "16th St. Mission" ) ) return "16TH";
  if( name.equalsIgnoreCase( "19th St. Oakland" ) ) return "19TH";
  if( name.equalsIgnoreCase( "24th St. Mission" ) ) return "24TH";
  if( name.equalsIgnoreCase( "Antioch" ) ) return "ANTC";
  if( name.equalsIgnoreCase( "Ashby" ) ) return "ASHB";
  if( name.equalsIgnoreCase( "Balboa Park" ) ) return "BALB";
  if( name.equalsIgnoreCase( "Bay Fair" ) ) return "BAYF";
  if( name.equalsIgnoreCase( "Berryessa/North San Jose" ) ) return "BERY";
  if( name.equalsIgnoreCase( "Castro Valley" ) ) return "CAST";
  if( name.equalsIgnoreCase( "Civic Center/UN Plaza" ) ) return "CIVC";
  if( name.equalsIgnoreCase( "Coliseum" ) ) return "COLS";
  if( name.equalsIgnoreCase( "Colma" ) ) return "COLM";
  if( name.equalsIgnoreCase( "Concord" ) ) return "CONC";
  if( name.equalsIgnoreCase( "Daly City" ) ) return "DALY";
  if( name.equalsIgnoreCase( "Downtown Berkeley" ) ) return "DBRK";
  if( name.equalsIgnoreCase( "Dublin/Pleasanton" ) ) return "DUBL";
  if( name.equalsIgnoreCase( "El Cerrito del Norte" ) ) return "DELN";
  if( name.equalsIgnoreCase( "El Cerrito Plaza" ) ) return "PLZA";
  if( name.equalsIgnoreCase( "Embarcadero" ) ) return "EMBR";
  if( name.equalsIgnoreCase( "Fremont" ) ) return "FRMT";
  if( name.equalsIgnoreCase( "Fruitvale" ) ) return "FTVL";
  if( name.equalsIgnoreCase( "Glen Park" ) ) return "GLEN";
  if( name.equalsIgnoreCase( "Hayward" ) ) return "HAYW";
  if( name.equalsIgnoreCase( "Lafayette" ) ) return "LAFY";
  if( name.equalsIgnoreCase( "Lake Merritt" ) ) return "LAKE";
  if( name.equalsIgnoreCase( "MacArthur" ) ) return "MCAR";
  if( name.equalsIgnoreCase( "Millbrae" ) ) return "MLBR";
  if( name.equalsIgnoreCase( "Milpitas" ) ) return "MLPT";
  if( name.equalsIgnoreCase( "Montgomery St." ) ) return "MONT";
  if( name.equalsIgnoreCase( "North Berkeley" ) ) return "NBRK";
  if( name.equalsIgnoreCase( "North Concord/Martinez" ) ) return "NCON";
  if( name.equalsIgnoreCase( "Oakland International Airport" ) ) return "OAKL";
  if( name.equalsIgnoreCase( "Orinda" ) ) return "ORIN";
  if( name.equalsIgnoreCase( "Pittsburg/Bay Point" ) ) return "PITT";
  if( name.equalsIgnoreCase( "Pittsburg Center" ) ) return "PCTR";
  if( name.equalsIgnoreCase( "Pleasant Hill/Contra Costa Centre" ) ) return "PHIL";
  if( name.equalsIgnoreCase( "Powell St." ) ) return "POWL";
  if( name.equalsIgnoreCase( "Richmond" ) ) return "RICH";
  if( name.equalsIgnoreCase( "Rockridge" ) ) return "ROCK";
  if( name.equalsIgnoreCase( "San Bruno" ) ) return "SBRN";
  if( name.equalsIgnoreCase( "San Francisco International Airport" ) ) return "SFIA";
  if( name.equalsIgnoreCase( "San Leandro" ) ) return "SANL";
  if( name.equalsIgnoreCase( "South Hayward" ) ) return "SHAY";
  if( name.equalsIgnoreCase( "South San Francisco" ) ) return "SSAN";
  if( name.equalsIgnoreCase( "Union City" ) ) return "UCTY";
  if( name.equalsIgnoreCase( "Walnut Creek" ) ) return "WCRK";
  if( name.equalsIgnoreCase( "Warm Springs/South Fremont" ) ) return "WARM";
  if( name.equalsIgnoreCase( "West Dublin/Pleasanton" ) ) return "WDUB";
  if( name.equalsIgnoreCase( "West Oakland" ) ) return "WOAK";
  
  return( "UNKN" );
}

/*
 * Helper routine to pull names but easier to just store locally
 */
void parseStationNames(){
  String url = String( "http://api.bart.gov/api/stn.aspx?cmd=stns&key=" + bartApiKey + "&json=y" );
  String bartresponse = httpGETRequest( url.c_str() );

  DynamicJsonDocument doc(20000);
  deserializeJson(doc, bartresponse);

  if ( doc.isNull() ) {
    Serial.println("Parsing JSON list of BART stations failed!");
    return;
  }
  
  int numStations = doc["root"]["stations"]["station"].size();

  for( int i=0; i < numStations; i++ ){

    Serial.print( "if( name.equalsIgnoreCase( \"" );
    Serial.print( String( doc["root"]["stations"]["station"][ i ]["name"] ) );
    Serial.print( "\" ) ) return \"" );
    Serial.print( String( doc["root"]["stations"]["station"][ i ]["abbr"] ) );
    Serial.println( "\";" );
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  Serial.print("GET-ing web page: ");
  Serial.println(serverName);

  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}"; 

  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
