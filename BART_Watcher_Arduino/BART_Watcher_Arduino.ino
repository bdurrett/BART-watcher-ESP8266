
#include <WiFiManager.h>              // https://github.com/tzapu/WiFiManager WiFi Configuration Magic (v. 2.0.3-alpha)
#include <ArduinoJson.h>              // https://github.com/bblanchon/ArduinoJson (v. 6.7.13)
#include <ArduinoOTA.h>               // https://github.com/jandrassy/ArduinoOTA (v. 1.0.5)
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "SSD1306Wire.h"              // https://github.com/ThingPulse/esp8266-oled-ssd1306


SSD1306Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL

const char compile_date[] = __DATE__ " " __TIME__;

// Currently using the shared legacy API key, https://www.bart.gov/schedules/developers/api
// this may be revoked at any time, get your own personal key
String bartApiKey = "MW9S-E7SL-26DU-VV8V";

// Line height for each font size
int smallLineHeight = 11;
int bigLineHeight = 16;

/*
String startingStation = "NBRK";
String directionNeeded = "South";
bool redLineOption = true;
bool redLinePreferred = true;
bool orangeLineOption = true;
bool orangeLinePreferred = false;
bool yellowLineOption = false;
bool yellowLinePreferred = false;
bool blueLineOption = false;
bool blueLinePreferred = false;
bool greenLineOption = false;
bool greenLinePreferred = false;
int notEnoughMinutes = 7;
int tooManyMinutes = 15;
*/

String startingStation = "EMBR";
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
  WiFiManager wifiManager;
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
}

void loop() {
  refreshStationData();
  delay( 30 * 1000 );
}

void refreshStationData() {
  String url = String( "http://api.bart.gov/api/etd.aspx?cmd=etd&orig=" + startingStation + "&key="  + bartApiKey + "&json=y" );
  String bartresponse = httpGETRequest( url.c_str() );
  // Serial.println(bartresponse);
  
  DynamicJsonDocument doc(8192);
  deserializeJson(doc, bartresponse);

  if ( doc.isNull() ) {
    Serial.println("Parsing input failed!");
    return;
  }
  
  int yPos = 0;

  display.clear();  
  display.setColor( WHITE );        

  String datetime = String( doc["root"]["date"] ).substring(0,5) + " " + String( doc["root"]["time"] ).substring(0,8);
  Serial.println(datetime);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment( TEXT_ALIGN_RIGHT );
  display.drawString( 128, 0, String( startingStation + "-->" ).c_str() );
  display.setTextAlignment( TEXT_ALIGN_LEFT );
  display.drawString( 0, 0, datetime.c_str() );
  yPos += smallLineHeight + 2;

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
        updateDestination( dest, 0, yPos, true );
        yPos += bigLineHeight;
      }
      else{
        updateDestination( dest, 0, yPos, false );
        yPos += smallLineHeight;
      }
    }
  }
  display.display();      // Show updated information on screen
}


void updateDestination( JsonObject dest, int x, int y, bool large ){
  if( large ){
    display.setFont(ArialMT_Plain_16);
  }
  else{
    display.setFont(ArialMT_Plain_10);
  }
  
  display.drawString( x, y, String( dest["abbreviation"] ) );
  x += display.getStringWidth( String( dest["abbreviation"] ) + "  " );

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
      if( mins >= notEnoughMinutes && mins <= tooManyMinutes ){
        int width = display.getStringWidth( output );
        if( large ){
          display.fillRect( x-2, y+2, width+4, 17-2);
        }
        else{
          display.fillRect( x-1, y+2, width+2, 12-2);
        }
        display.setColor( BLACK );        // Draw white text
      }
    }
    display.drawString( x, y, output );
    display.setColor( WHITE );        // Draw white text

    x += display.getStringWidth( output + "  " );
    
  }  
}


String stationAbbreviation( String name ){
  Serial.println( "Lookup station: " + name );

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
  
  return( "UNKN" );
}

/*
 * Helper routine to pull names but easier to just store locally
 */
void parseStationNames(){
  String url = String( "http://api.bart.gov/api/stn.aspx?cmd=stns&key=" + bartApiKey + "&json=y" );
  String bartresponse = httpGETRequest( url.c_str() );

  DynamicJsonDocument doc(8192);
  deserializeJson(doc, bartresponse);

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