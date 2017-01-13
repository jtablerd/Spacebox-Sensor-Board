#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "Adafruit_Si7021.h"
#include "ESP8266WebServer.h"
#include "Adafruit_TSL2561_U.h"
#include "ThingSpeak.h"

//Wifi info - ENTER YOUR NETWORK INFO HERE

const char* ssid     = "YOUR SSID HERE";
const char* password = "YOUR WIFI PASSWORD HERE";
WiFiClient  client;

//ThingSpeak - MUST CHANGE TO YOUR CHANNEL # AND WRITE API

unsigned long myChannelNumber = THINGSPEAK CHANNEL NUMBER;
const char * myWriteAPIKey = "THINGSPEAK WRITE API";

//Sensor Setup

//Si7021 Temp/Humidity Sensor
Adafruit_Si7021 sensor = Adafruit_Si7021();

// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

//TSL2561 Lux Sensor
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}


void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("101 ms");
  Serial.println("------------------------------------");
}


//Wifi Server Setup

// Web Server on port 80
WiFiServer server(80);



void setup() {

  ThingSpeak.begin(client);
  
  Serial.begin(115200);
  Serial.println("Wakie wakie");
  sensor.begin();
  

  // Connecting to WiFi network
  Serial.println(); 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Starting the web server
  server.begin();
  Serial.println("Web server running. Waiting for the ESP IP...");
  delay(5000);
  
  // Printing the ESP IP address
  Serial.println(WiFi.localIP());

  delay(2000);

  Serial.println("Light Sensor Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!tsl.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  /* Setup the sensor gain and integration time */
  configureSensor();
  
}


void loop() {

   sensors_event_t event;
   tsl.getEvent(&event);

            float lh = sensor.readHumidity();
            float lt = sensor.readTemperature();
            float ltf = lt * 1.8 + 32;
            float lux = event.light;
            float svp = 610.7 * (pow(10, (7.5*lt/(237.3+lt))));
            float vpd = (((100 - lh)/100)*svp)/1000;

            ThingSpeak.writeField(myChannelNumber, 1, ltf, myWriteAPIKey);            
            ThingSpeak.writeField(myChannelNumber, 2, lh, myWriteAPIKey);            
            ThingSpeak.writeField(myChannelNumber, 3, lux, myWriteAPIKey);
            ThingSpeak.writeField(myChannelNumber, 4, vpd, myWriteAPIKey);

            

  // Listenning for new clients
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("New client");
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (c == '\n' && blank_line) {
            float lh = sensor.readHumidity();
            float lt = sensor.readTemperature();
            float lux = event.light;
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
                        
// web page html

            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.println("<body bgcolor=002D64>");
            client.println("<font color = ffffff>");
            client.println("<head></head><body><h2>Spacebox</h2>");
            client.println("<h4><blink>Welcome - to the world of tomorrow!!!</blink></h4>");
            client.println("<br>");
            client.println("<h3>Box:</h3>");
            client.println(" Temp: ");
            client.println(lt * 1.8 + 32);
            client.println(" *F");
            client.println("<br>");
            client.println("Lux Value: ");
            client.println(lux);
            client.println(" lux");
            client.println("<br>");
            client.println("Humidity: ");
            client.println(lh);
            client.println(" %");
            client.println("<br>");
            client.println("Saturated Vapor Pressure: ");
            client.println(svp);
            client.println("<br>");
            client.println("VPD: ");
            client.println(vpd);
            client.println("kPa");
            client.println("</body></font></html>");     
                      
            break;

        }
        if (c == '\n') {
          // when starts reading a new line
          blank_line = true;
        }
        else if (c != '\r') {
          // when finds a character on the current line
          blank_line = false;
        }
      }
    }  
    // closing the client connection
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
  }}
