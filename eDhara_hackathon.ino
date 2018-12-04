#include <WiFi.h>
#include "DHT.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include "SPIFFS.h"
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// Uncomment one of the lines below for whatever DHT sensor type you're using!
# define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Set web server port number to 80
WiFiServer server(80);
//flag for saving data
bool shouldSaveConfig = true;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

int LED_PIN = 2;
int TEMP_LED_PIN = 19;
int HUMIDITY_LED_PIN = 18;
int MOISTURE_LED_PIN = 4;
int PHOTO_LED_PIN = 15;
int RESET_PIN = 12;
boolean solar = false;
int buttonState = 0;  

// DHT Sensor
const int DHTPin = 5;
const size_t bufferSize = 2 * JSON_ARRAY_SIZE(0) + 3 * JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(4) + 8 * JSON_OBJECT_SIZE(1) + 5 * JSON_OBJECT_SIZE(2) + 4 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 5 * JSON_OBJECT_SIZE(6);
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];
int mositure;
int photosensitivity;

// Client variables
char linebuf[80];
int charcount = 0;


// Indicates whether ESP has WiFi credentials saved from previous session
bool initialConfig = false;

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(TEMP_LED_PIN, OUTPUT);
  pinMode(HUMIDITY_LED_PIN, OUTPUT);
  pinMode(MOISTURE_LED_PIN, OUTPUT);
  pinMode(PHOTO_LED_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT);
  // initialize the DHT sensor
//  digitalWrite(RESET_PIN, HIGH);
  digitalWrite(TEMP_LED_PIN, HIGH);
  digitalWrite(HUMIDITY_LED_PIN, HIGH);
  digitalWrite(MOISTURE_LED_PIN, HIGH);
  digitalWrite(PHOTO_LED_PIN, HIGH);
  
  
  dht.begin();

  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) { ; // wait for serial port to connect. Needed for native USB port only
  }

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  display.drawPixel(10, 10, WHITE);
  // Show the display buffer on the hardware.
  // NOTE: You _must_ call display after making any drawing commands
  // to make them visible on the display hardware!
  display.display();
  delay(2000);
  display.clearDisplay();

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Booting Up the device ......");
  //display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.display();
  delay(3000);
  display.clearDisplay();

  
  Serial.println("mounting FS...");
  SPIFFS.begin(true);
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  if (WiFi.SSID()==""){
    Serial.println("We haven't got any access point credentials, so get them now");   
    initialConfig = true;
    // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("In AP MODE use below details");
  display.println("SSID : eDhara_ESP32, IP : 192.168.4.1");
    //display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.display();
  }

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("eDhara_ESP32", "G42")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.psk());
  Serial.println(); 
  server.begin();
  //WiFi.disconnect(true); //erases store credentially
  // text display 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("SSID:"+WiFi.SSID()); 
  display.println(WiFi.localIP());
  display.println("Device Ready...");
  display.display();
  
  
}

void loop() {
  buttonState = digitalRead(RESET_PIN);
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    //Serial.println("New client");
    memset(linebuf, 0, sizeof(linebuf));
    charcount = 0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        //read char by char HTTP request
        linebuf[charcount] = c;
        if (charcount < sizeof(linebuf) - 1)
          charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
          float h = dht.readHumidity();
          // Read temperature as Celsius (the default)
          float t = dht.readTemperature();
          // Read temperature as Fahrenheit (isFahrenheit = true)
          float f = dht.readTemperature(true);
          // Check if any reads failed and exit early (to try again).
          if (isnan(h) || isnan(t) || isnan(f)) {
            //Serial.println("Failed to read from DHT sensor!");
            strcpy(celsiusTemp, "Failed");
            strcpy(fahrenheitTemp, "Failed");
            strcpy(humidityTemp, "Failed");
          } else {
            // Computes temperature values in Celsius + Fahrenheit and Humidity
            float hic = dht.computeHeatIndex(t, h, false);
            dtostrf(hic, 6, 2, celsiusTemp);
            float hif = dht.computeHeatIndex(f, h);
            dtostrf(hif, 6, 2, fahrenheitTemp);
            dtostrf(h, 6, 2, humidityTemp);
                 
          }

          mositure = analogRead(39);
          mositure = map(mositure, 4095, 0, 0, 100);
                   
                
              
          //delay(1000);

          photosensitivity = analogRead(36);
          photosensitivity = map(photosensitivity, 0, 4095, 0, 100);
                       
                     
               
          //delay(1000);
          
          if (photosensitivity <= 60) {
            solar = false;
          } else if (photosensitivity > 60) {
            solar = true;
          }

          //JSON START

          DynamicJsonBuffer jsonBuffer(bufferSize);

          JsonObject & root = jsonBuffer.createObject();
          root["version"] = 1;
          root["allow_edit"] = true;
          JsonArray & plugins = root.createNestedArray("plugins");

          JsonArray & panes = root.createNestedArray("panes");

          JsonObject & panes_0 = panes.createNestedObject();
          panes_0["title"] = "Temperature";
          panes_0["width"] = 1;
          JsonObject & panes_0_row = panes_0.createNestedObject("row");
          panes_0_row["3"] = 5;
          JsonObject & panes_0_col = panes_0.createNestedObject("col");
          panes_0_col["3"] = 3;
          panes_0["col_width"] = 1;

          JsonArray & panes_0_widgets = panes_0.createNestedArray("widgets");

          JsonObject & panes_0_widgets_0 = panes_0_widgets.createNestedObject();
          panes_0_widgets_0["type"] = "text_widget";

          JsonObject & panes_0_widgets_0_settings = panes_0_widgets_0.createNestedObject("settings");
          panes_0_widgets_0_settings["size"] = "big";
          panes_0_widgets_0_settings["value"] = celsiusTemp;
          panes_0_widgets_0_settings["animate"] = true;
          panes_0_widgets_0_settings["units"] = "&deg;C";

          JsonObject & panes_0_widgets_1 = panes_0_widgets.createNestedObject();
          panes_0_widgets_1["type"] = "text_widget";

          JsonObject & panes_0_widgets_1_settings = panes_0_widgets_1.createNestedObject("settings");
          panes_0_widgets_1_settings["size"] = "small";
          panes_0_widgets_1_settings["value"] = fahrenheitTemp;
          panes_0_widgets_1_settings["animate"] = true;
          panes_0_widgets_1_settings["units"] = "&deg;F";

          JsonObject & panes_1 = panes.createNestedObject();
          panes_1["title"] = "Moisture";
          panes_1["width"] = 1;
          JsonObject & panes_1_row = panes_1.createNestedObject("row");
          panes_1_row["3"] = 1;
          JsonObject & panes_1_col = panes_1.createNestedObject("col");
          panes_1_col["3"] = 2;
          panes_1["col_width"] = 1;

          JsonArray & panes_1_widgets = panes_1.createNestedArray("widgets");

          JsonObject & panes_1_widgets_0 = panes_1_widgets.createNestedObject();
          panes_1_widgets_0["type"] = "gauge";

          JsonObject & panes_1_widgets_0_settings = panes_1_widgets_0.createNestedObject("settings");
          panes_1_widgets_0_settings["value"] = String(mositure);
          panes_1_widgets_0_settings["units"] = "%";
          panes_1_widgets_0_settings["min_value"] = 0;
          panes_1_widgets_0_settings["max_value"] = 100;

          JsonObject & panes_2 = panes.createNestedObject();
          panes_2["title"] = "Photo Sensitivity";
          panes_2["width"] = 1;
          JsonObject & panes_2_row = panes_2.createNestedObject("row");
          panes_2_row["3"] = 1;
          JsonObject & panes_2_col = panes_2.createNestedObject("col");
          panes_2_col["3"] = 3;
          panes_2["col_width"] = 1;

          JsonArray & panes_2_widgets = panes_2.createNestedArray("widgets");

          JsonObject & panes_2_widgets_0 = panes_2_widgets.createNestedObject();
          panes_2_widgets_0["type"] = "indicator";

          JsonObject & panes_2_widgets_0_settings = panes_2_widgets_0.createNestedObject("settings");
          if(solar==true){
            panes_2_widgets_0_settings["value"] = "1";
          }else{
            panes_2_widgets_0_settings["value"] = "0";
          }
          panes_2_widgets_0_settings["on_text"] = "Solar";
          panes_2_widgets_0_settings["off_text"] = "Power";
          panes_2_widgets_0_settings["PS_Value"] = String(photosensitivity);

          JsonObject & panes_3 = panes.createNestedObject();
          panes_3["title"] = "Humidity";
          panes_3["width"] = 1;
          JsonObject & panes_3_row = panes_3.createNestedObject("row");
          panes_3_row["3"] = 1;
          JsonObject & panes_3_col = panes_3.createNestedObject("col");
          panes_3_col["3"] = 1;
          panes_3["col_width"] = 1;

          JsonArray & panes_3_widgets = panes_3.createNestedArray("widgets");

          JsonObject & panes_3_widgets_0 = panes_3_widgets.createNestedObject();
          panes_3_widgets_0["type"] = "gauge";

          JsonObject & panes_3_widgets_0_settings = panes_3_widgets_0.createNestedObject("settings");
          panes_3_widgets_0_settings["title"] = "";
          panes_3_widgets_0_settings["value"] = humidityTemp;
          panes_3_widgets_0_settings["units"] = "%";
          panes_3_widgets_0_settings["min_value"] = 0;
          panes_3_widgets_0_settings["max_value"] = 100;
          JsonArray & datasources = root.createNestedArray("datasources");
          root["columns"] = 3;

          //root.printTo(Serial);

          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/JSON");
          client.println("access-control-allow-credentials:true");
          client.println("access-control-allow-headers:X-Requested-With,Origin,Content-Type");
          client.println("access-control-allow-methods:PUT,GET,POST,DELETE,OPTIONS");
          client.println("access-control-allow-origin:*");
          client.println("Connection: close"); // the connection will be closed after completion of the response
          client.print("Content-Length: ");
          client.print(root.measureLength());
          client.print("\r\n");
          client.println();
          root.printTo(client);
          //JSON END


          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf, 0, sizeof(linebuf));
          charcount = 0;
          currentLine = "";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
          currentLine += c;
        }
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_PIN, HIGH); // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_PIN, LOW); // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /bl")) {
          digitalWrite(TEMP_LED_PIN, HIGH); // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /bh")) {
          digitalWrite(TEMP_LED_PIN, LOW); // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /cl")) {
          digitalWrite(HUMIDITY_LED_PIN, HIGH); // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /ch")) {
          digitalWrite(HUMIDITY_LED_PIN, LOW); // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /dl")) {
          digitalWrite(MOISTURE_LED_PIN, HIGH); // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /dh")) {
          digitalWrite(MOISTURE_LED_PIN, LOW); // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /el")) {
          digitalWrite(PHOTO_LED_PIN, HIGH); // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /eh")) {
          digitalWrite(PHOTO_LED_PIN, LOW); // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /reset")) {
          //WiFi.disconnect(true); //erases store credentially
          //SPIFFS.format();
          //ESP.restart();
          Serial.println("resetting WIFI");
          //hard_restart();
          soft_restart(client);
        }
      }
    }
    // give the web browser time to receive the data
    //delay(1);

    // close the connection:
    client.stop();
                    
  }
  
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED on:
    Serial.println("1");
    //digitalWrite(ledPin, HIGH);
    hard_restart();
  }
}

void hard_restart() {
  WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
  WiFi.persistent(true);
  WiFi.disconnect(true,true);
  WiFi.persistent(false);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Resetting the device");
  display.display();
  ESP.getFreeHeap();
  delay(2000);
  delay(2000);
  ESP.restart();
  delay(2000);
  Serial.println("reboot complete");
}

void soft_restart(WiFiClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text");
  client.println("access-control-allow-credentials:true");
  client.println("access-control-allow-headers:X-Requested-With,Origin,Content-Type");
  client.println("access-control-allow-methods:PUT,GET,POST,DELETE,OPTIONS");
  client.println("access-control-allow-origin:*");
  client.println("Connection: close"); // the connection will be closed after completion of the response
  client.print("Content-Length:2");
  client.print("\r\n");
  client.println();
  client.println("OK");
  delay(2000);
  WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
  WiFi.persistent(true);
  WiFi.disconnect(true,true);
  WiFi.persistent(false);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Resetting the device");
  display.display();
  ESP.getFreeHeap();
  delay(2000);
  delay(2000);
  ESP.restart();
  delay(2000);
  Serial.println("reboot complete");
  }
