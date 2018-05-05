/*
 * Rui Santos 
 * Complete Project Details http://randomnerdtutorials.com
*/

#include <WiFi.h>
#include "DHT.h"

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Replace with your network credentials
const char* ssid     = "one2";
const char* password = "12345678";
int LED_PIN = 2;
int TEMP_LED_PIN = 19;
int HUMIDITY_LED_PIN = 21;
int MOISTURE_LED_PIN = 22;
int PHOTO_LED_PIN = 23;
boolean solar = false;
WiFiServer server(80);

// DHT Sensor
const int DHTPin = 5;
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
int charcount=0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(TEMP_LED_PIN, OUTPUT);
  pinMode(HUMIDITY_LED_PIN, OUTPUT);
  pinMode(MOISTURE_LED_PIN, OUTPUT);
  pinMode(PHOTO_LED_PIN, OUTPUT);
  // initialize the DHT sensor
  digitalWrite(TEMP_LED_PIN, HIGH);
  digitalWrite(HUMIDITY_LED_PIN, HIGH);
  digitalWrite(MOISTURE_LED_PIN, HIGH);
  digitalWrite(PHOTO_LED_PIN, HIGH);
  dht.begin();
  
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // attempt to connect to Wifi network:
  while(WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available(); 
  if (client) {
    Serial.println("New client");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
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
              Serial.println("Failed to read from DHT sensor!");
              strcpy(celsiusTemp,"Failed");
              strcpy(fahrenheitTemp, "Failed");
              strcpy(humidityTemp, "Failed");         
            }
            else{
              // Computes temperature values in Celsius + Fahrenheit and Humidity
              float hic = dht.computeHeatIndex(t, h, false);       
              dtostrf(hic, 6, 2, celsiusTemp);             
              float hif = dht.computeHeatIndex(f, h);
              dtostrf(hif, 6, 2, fahrenheitTemp);         
              dtostrf(h, 6, 2, humidityTemp);
              // You can delete the following Serial.print's, it's just for debugging purposes
              Serial.print("Humidity: ");
              Serial.print(h);
              Serial.print(" %\n Temperature: ");
              Serial.print(t);
              Serial.print(" *C ");
              Serial.print(f);
              Serial.print(" *F\t Heat index: ");
              Serial.print(hic);
              Serial.print(" *C ");
              Serial.print(hif);
              Serial.print(" *F");
              Serial.print("Humidity: ");
              Serial.print(h);
              Serial.print(" %\n Temperature: ");
              Serial.print(t);
              Serial.print(" *C ");
              Serial.print(f);
              Serial.print(" *F\t Heat index: ");
              Serial.print(hic);
              Serial.print(" *C ");
              Serial.print(hif);
              Serial.println(" *F");
          }
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/JSON");
          client.println("access-control-allow-credentials:true");
          client.println("access-control-allow-headers:X-Requested-With,Origin,Content-Type");
          client.println("access-control-allow-methods:PUT,GET,POST,DELETE,OPTIONS");
          client.println("access-control-allow-origin:*");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println(); 

          mositure= analogRead(39);
          mositure = map(mositure,4095, 0, 0, 100);
          Serial.print("Moisture : ");
          Serial.print(mositure);
          Serial.println("%");
          delay(1000);
          
          photosensitivity = analogRead(36);
          photosensitivity = map(photosensitivity, 0, 4095, 0, 100);
          Serial.print("photosensitivity : ");
          Serial.print(photosensitivity);
          Serial.println("%");
          delay(1000);
          if(photosensitivity <= 80){
            solar = false;
          }
          else if(photosensitivity > 80){
            solar = true;
          }

//JSON START

client.println("{");
  client.println("\"version\": 1,");
  client.println("\"allow_edit\": true,");
  client.println("\"plugins\": [],");
  client.println("\"panes\": [");
    client.println("{");
      client.println("\"title\": \"Temperature\",");
      client.println("\"width\": 1,");
      client.println("\"row\": {");
        client.println("\"3\": 5");
      client.println("},");
      client.println("\"col\": {");
        client.println("\"3\": 3");
      client.println("},");
      client.println("\"col_width\": 1,");
      client.println("\"widgets\": [");
        client.println("{");
          client.println("\"type\": \"text_widget\",");
          client.println("\"settings\": {");
            client.println("\"size\": \"big\",");
            client.print("\"value\": \"");client.print(celsiusTemp);client.println("\",");
            client.println("\"animate\": true,");
            client.println("\"units\": \"&deg;C\"");
          client.println("}");
        client.println("},");
        client.println("{");
          client.println("\"type\": \"text_widget\",");
          client.println("\"settings\": {");
            client.println("\"size\": \"small\",");
            client.print("\"value\": \"");client.print(fahrenheitTemp);client.println("\",");
            client.println("\"animate\": true,");
            client.println("\"units\": \"&deg;F\"");
          client.println("}");
        client.println("}");
      client.println("]");
    client.println("},");
    client.println("{");
      client.println("\"title\": \"Moisture\",");
      client.println("\"width\": 1,");
      client.println("\"row\": {");
        client.println("\"3\": 1");
      client.println("},");
      client.println("\"col\": {");
        client.println("\"3\": 2");
      client.println("},");
      client.println("\"col_width\": 1,");
      client.println("\"widgets\": [");
        client.println("{");
          client.println("\"type\": \"gauge\",");
          client.println("\"settings\": {");
            client.print("\"value\": \"");client.print(mositure);client.println("\",");
            client.println("\"units\": \"%\",");
            client.println("\"min_value\": 0,");
            client.println("\"max_value\": 100");
          client.println("}");
        client.println("}");
      client.println("]");
    client.println("},");
    client.println("{");
      client.println("\"title\": \"Photo Sensitivity\",");
      client.println("\"width\": 1,");
      client.println("\"row\": {");
        client.println("\"3\": 1");
      client.println("},");
      client.println("\"col\": {");
        client.println("\"3\": 3");
      client.println("},");
      client.println("\"col_width\": 1,");
      client.println("\"widgets\": [");
        client.println("{");
          client.println("\"type\": \"indicator\",");
          client.println("\"settings\": {");
            client.print("\"value\": \"");client.print(solar);client.println("\",");
            client.println("\"on_text\": \"Solar\",");
            client.println("\"off_text\": \"Power\"");
          client.println("}");
        client.println("}");
      client.println("]");
    client.println("},");
    client.println("{");
      client.println("\"title\": \"Humidity\",");
      client.println("\"width\": 1,");
      client.println("\"row\": {");
        client.println("\"3\": 1");
      client.println("},");
      client.println("\"col\": {");
        client.println("\"3\": 1");
      client.println("},");
      client.println("\"col_width\": 1,");
      client.println("\"widgets\": [");
        client.println("{");
          client.println("\"type\": \"gauge\",");
          client.println("\"settings\": {");
            client.println("\"title\": \"\",");
            client.print("\"value\": \"");client.print(humidityTemp);client.println("\",");
            client.println("\"units\": \"%\",");
            client.println("\"min_value\": 0,");
            client.println("\"max_value\": 100");
          client.println("}");
        client.println("}");
      client.println("]");
    client.println("}");
  client.println("],");
  client.println("\"datasources\": [],");
  client.println("\"columns\": 3");
client.println("}");

//JSON END



          
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
          currentLine = "";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
          currentLine += c;
        }
         if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_PIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_PIN, LOW);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /bl")) {
          digitalWrite(TEMP_LED_PIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /bh")) {
          digitalWrite(TEMP_LED_PIN, LOW);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /cl")) {
          digitalWrite(HUMIDITY_LED_PIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /ch")) {
          digitalWrite(HUMIDITY_LED_PIN, LOW);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /dl")) {
          digitalWrite(MOISTURE_LED_PIN, HIGH);                // GET /L turns the LED off
        }
         if (currentLine.endsWith("GET /dh")) {
          digitalWrite(MOISTURE_LED_PIN, LOW);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /el")) {
          digitalWrite(PHOTO_LED_PIN, HIGH);                // GET /L turns the LED off
        }
         if (currentLine.endsWith("GET /eh")) {
          digitalWrite(PHOTO_LED_PIN, LOW);                // GET /L turns the LED off
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
