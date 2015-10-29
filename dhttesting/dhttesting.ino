/***************************************************
 
  Ambiance 1 - Adafruit HUZZAH ESP8266-12 DHT22/Photocell sensor Module

  subscribing to pacer is returning null? need to switch to REST api?  sigh....

  Adapted from sketch in Adafruit.com
  
  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino
    
  Works great with Adafruit's Huzzah ESP board:
  ----> https://www.adafruit.com/product/2471

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution

 ****************************************************/

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

#define IOSEND          true
#define ECHO            true

// pace of publications, but send only if a sensor has a new value compared to last reading
#define IOCYCLE_SECS    300

// BUILTIN_LED is an ESP defined pin for GPIO 0; instead of LED 13 in Arduino UNO
// Connect GPIO 16 to GPIO RST to enable deep sleep wakes, board comes up and the sketch runs from the top

// Photocell
#define LUXPIN          A0

// DHT22 sensor for humidity and temperature
#define DHTPIN          5
#define DHTTYPE         DHT22

// WiFi parameters
#define WLAN_SSID       "roughashlar"
#define WLAN_PASS       "r0mans12:1"

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "jmpantelis"
#define AIO_KEY         "553c3d5043bda3fe355ca4ba0a529dbd0e2ef1a4"

// Change this AIO_CLIENT ID for different ESP units.
// Marcos: AMB1
// Robert: AMB2
#define AIO_CLIENT      "AMB2"

// Initialize DHT22 sensor
DHT dht(DHTPIN, DHTTYPE, 15);

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, client ID, username, and password in flash memory.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;

// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = AIO_USERNAME AIO_CLIENT __DATE__;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/
/*  Marcos: tmp, hum, lux, pacer; Group: amb1; Dashboard: Ambiance 1
 *  
 *  Robert: tmp2, hum2, lux2, pacer2; Group: amb2; Dashboard: Ambiance 2 
 */


const char TMP_FEED[] PROGMEM = AIO_USERNAME "/feeds/tmp2";
Adafruit_MQTT_Publish tmp = Adafruit_MQTT_Publish(&mqtt, TMP_FEED);

const char HUM_FEED[] PROGMEM = AIO_USERNAME "/feeds/hum2";
Adafruit_MQTT_Publish hum = Adafruit_MQTT_Publish(&mqtt, HUM_FEED);

const char LUX_FEED[] PROGMEM = AIO_USERNAME "/feeds/lux2";
Adafruit_MQTT_Publish lux = Adafruit_MQTT_Publish(&mqtt, LUX_FEED);

// Subscription is not working at this time.  10/23/15
// Setup a feed called 'pacer' for subscribing to sensor reading rate changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//const char PACER_FEED[] PROGMEM = AIO_USERNAME "/feeds/pacer";
//Adafruit_MQTT_Subscribe pacer = Adafruit_MQTT_Subscribe(&mqtt, PACER_FEED);

// These controls to limit resends of same values work if pace<60secs
float last_tmp = -1;
float last_hum = -1;
int last_lux = -1;

void setup() {

  pinMode(LUXPIN, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  
  dht.begin();

  if(ECHO) {
    Serial.begin(115200);
    delay(10);
    Serial.println(F("Ambiance 1 - Adafruit IO Test"));
    Serial.print(F("\nConnecting to ")); Serial.println(WLAN_SSID);
  }

  // Connect to WiFi access point.
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    //delay(500);
    if(ECHO) Serial.print(F("."));
    blinking( 1, 250 );
  }
  
  if(ECHO) {
    Serial.println();
    Serial.println(F("WiFi connected"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
  }

  //mqtt.subscribe(&pacer);
  
  // connect to adafruit io
  connect();
}

void loop() {
  
  // Subscription is not working at this time.  10/23/15
  //Adafruit_MQTT_Subscribe *subscription;
  
  int pace= IOCYCLE_SECS;
  
  // check if still connected to adafruit io; else reconnect
  if( !mqtt.ping(3) ) {
     if( !mqtt.connected() ) {
        connect();
     }
   }


  // Subscription is not working at this time.  10/23/15
  /* this is our 'wait for incoming subscription packets' busy subloop
  while (subscription = mqtt.readSubscription(1000)) {

    if(ECHO) Serial.println(F("Attempting to read pacer value..."));
 
    // we only care about the lamp events
    if (subscription == &pacer) {
 
      // convert mqtt ascii payload to int
      char *avalue = (char *)pacer.lastread;
      if(ECHO){ Serial.print(F("Pacer value received: ")); Serial.println(avalue); }
      
      //pace = atoi(avalue); // convert to integer
    }
 
  } */

  // Keeping readings as integers for now.  AIO is not displaying floats quite well yet.

  float hum_data = dht.readHumidity();
  float tmp_data = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(hum_data) || isnan(tmp_data)) {
    if(ECHO) Serial.println(F("Failed to read from DHT sensor!"));
    
    blinking( 30, 500 );
    
    return;
  }

  // read the photocell level from the HUZZAH ESP8266 analog in pin.
  // analog read level is 10 bit 0-1023 (0V-1.8V).
  // our 10K fixed resistor and variable photocell resistor (1K-10K) voltage 
  // divider takes the ESP voltage of 3.3V and drops it to 0.3 min and 1.65 max.
  // this means our min analog read value should be 170 (0.3)
  // and the max analog read value should be 938 (1.65V).  Not really working this way, why?
  int lux_data;
  for( int i = 0; i < 3; i++) { lux_data = analogRead(LUXPIN); delay(40); }

  lux_data = map(lux_data, 0, 1024, 0, 100);
  lux_data = 100 - lux_data;

  if(IOSEND) {
        
    if(tmp_data != last_tmp) {  // Only re-publish if there is a change
      if ( !tmp.publish(tmp_data)) {
        if(ECHO) Serial.println(F("Failed to publish temperature"));
      }
      else {
        if(ECHO) Serial.println(F("Temperature published!"));
        last_tmp = tmp_data;
      }
    }

    if(hum_data != last_hum) {  // Only republish if there is a change
      if ( !hum.publish(hum_data)) { 
        if(ECHO) Serial.println(F("Failed to publish humidity"));
      }
      else {
        if(ECHO) Serial.println(F("Humidity published!"));
        last_hum = hum_data;
      }
    }

    if(lux_data != last_lux) {  // Only republish if there is a change
      if( !lux.publish(lux_data)) {
        if(ECHO) Serial.println(F("Failed to publish luminosity"));
      }
      else {
        if(ECHO) Serial.println(F("Luminosity published!"));
        last_lux = lux_data;
      }
    }
  }
  
  if(ECHO) { // This is setup to see sensors output when on ECHO mode
    Serial.print(F("\nhum val (%): ")); Serial.println(hum_data);
    Serial.print(F("   last_hum val: "));  Serial.println(last_hum);
    Serial.print(F("tmp val (F): "));   Serial.println(tmp_data);
    Serial.print(F("   last_tmp val: "));  Serial.println(last_tmp);
    Serial.print(F("lux val (V): "));   Serial.println(lux_data);
    Serial.print(F("   last_lux val: ")); Serial.println(last_lux);
    Serial.print(F("next read in (secs): ")); 
    Serial.println(pace);
  }

  //if( pace >= 60 ) {
      ESP.deepSleep(100*1000000, WAKE_RF_DEFAULT);
      delay(200);
 // } 
  //else {
  //  delay( pace * 1000 );  
  //}
}

// connect to adafruit io via MQTT
void connect() {

  if(ECHO) Serial.print(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    if(ECHO) {
      switch (ret) {
        case 1: Serial.println(F("Wrong protocol")); break;
        case 2: Serial.println(F("ID rejected")); break;
        case 3: Serial.println(F("Server unavail")); break;
        case 4: Serial.println(F("Bad user/pass")); break;
        case 5: Serial.println(F("Not authed")); break;
        case 6: Serial.println(F("Failed to subscribe")); break;
        default: Serial.println(F("Connection failed")); break;
      }
    }

    if(ret >= 0)
      mqtt.disconnect();

    if(ECHO) Serial.println(F("Retrying connection in 5 seconds..."));
  }

  if(ECHO) Serial.println(F("Adafruit IO Connected!"));
}

void blinking( int times, int blinktime ) {
  int i;
  int led= 0; // start off
  
  for (i=0;i<times*2;i++) // loop twice to blink 
  {
   digitalWrite(BUILTIN_LED,led);
   led=led^1; // swap state
   delay(blinktime);
  }
}



// END
 
