/*****
 
 All the resources for this project:
 http://randomnerdtutorials.com/
 
*****/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "passinfo.h"
#include "DHT.h"

#define ZONE "zone1"

// Uncomment one of the lines bellow for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "192.168.1.109";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
String nombreini = "ESP8266Client_";
WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor - GPIO 5 = D1 on ESP-12E NodeMCU board
const int DHTPin = 5;

// Lamp - LED - GPIO 4 = D2 on ESP-12E NodeMCU board
const int lamp = 4;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// int BAT= A0;              //Analog channel A0 as used to measure battery voltage
// float RatioFactor=1;  //Resistors Ration Factor

//const int AirValue = 790;   //you need to replace this value with Value_1
//const int WaterValue = 390;  //you need to replace this value with Value_2
//const int SensorPin = A0;
//int soilMoistureValue = 0;
//int soilmoisturepercent=0;
const float AirValue = 747.0;   //you need to replace this value with Value_1 (orig: 790.0)
const float WaterValue = 316.0;  //you need to replace this value with Value_2 (orig: 390.0)
const int SensorPin = A0;
float soilMoistureValue = 0.0;
int soilmoisturepercent=0;

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic garden/zona1/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="garden/" ZONE "/lamp"){
      Serial.print("Changing zone lamp to ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  String strcli = nombreini + WiFi.localIP().toString();
  const char* nombrecliente = strcli.c_str();
  Serial.print("nombrecliente: ");
  Serial.println(nombrecliente);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect(nombrecliente)) {
      Serial.println("connected client");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("garden/" ZONE "/lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  pinMode(lamp, OUTPUT);
  
  dht.begin();
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop()) {
    const char* nombrecliente = (nombreini + WiFi.localIP().toString()).c_str();
    client.connect(nombrecliente);
  }

//  float Tvoltage=0.0;
//  float Vvalue=0.0,Rvalue=0.0;

  now = millis();
  // Publishes new temperature and humidity every 30 seconds
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    
    // Uncomment to compute temperature values in Fahrenheit 
    // float hif = dht.computeHeatIndex(f, h);
    // static char temperatureTemp[7];
    // dtostrf(hif, 6, 2, temperatureTemp);
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    soilMoistureValue=0.0;
    for(unsigned int i=0;i<10;i++){
      soilMoistureValue=soilMoistureValue+analogRead(SensorPin);         //Read analog Voltage
      delay(5);                              //ADC stable
    }
    // soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
    soilMoistureValue=(float)soilMoistureValue/10.0;
    soilmoisturepercent = map((int)soilMoistureValue, AirValue, WaterValue, 0, 100);
    Serial.print("soilMoistureValue: ");
    Serial.print(soilMoistureValue);
    Serial.print(" --> Soil moisture Percentage: ");
    Serial.print(soilmoisturepercent);
    Serial.println("%");
    if(soilmoisturepercent > 100)
    {
      Serial.println(" --> setting to 100 %");
      soilmoisturepercent = 100;
    } else if(soilmoisturepercent <0)
    {
      Serial.println(" --> setting to 0 %");
    }
    static char moistureStr[7];
    dtostrf(soilmoisturepercent, 6, 2, moistureStr);

    // Publishes input voltage
    client.publish("garden/" ZONE "/moisturepercent", moistureStr);

    /////////////////////////////////////Battery Voltage//////////////////////////////////  
//    for(unsigned int i=0;i<10;i++){
//      Vvalue=Vvalue+analogRead(BAT);         //Read analog Voltage
//      delay(5);                              //ADC stable
//    }
//    Vvalue=(float)Vvalue/10.0;            //Find average of 10 values
//    Rvalue=(float)(Vvalue/1024.0)*5;      //Convert Voltage in 5v factor
//    Tvoltage=Rvalue*RatioFactor;          //Find original voltage by multiplying with factor
//    static char TvoltageStr[7];
//    dtostrf(Tvoltage, 6, 2, TvoltageStr);
    /////////////////////////////////////Battery Voltage//////////////////////////////////
    // Publishes input voltage
//    client.publish("esp8266/" ZONE "/voltage", TvoltageStr);

    // Publishes Temperature and Humidity values
    client.publish("garden/" ZONE "/temperature", temperatureTemp);
    client.publish("garden/" ZONE "/humidity", humidityTemp);
    
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");
    // Serial.print(hif);
    // Serial.println(" *F");

//    Serial.print("\t Voltage: ");
//    Serial.print(Tvoltage);
//    Serial.println(" volts ");

//    if(Tvoltage<=2){
//      Serial.print("Battery dead OR disconnected");
//    }
//    else if(Tvoltage>2 && Tvoltage<=4){
//      Serial.print("Need Imediate recharge");
//    }
//    else if(Tvoltage>4 && Tvoltage<=5){
//      Serial.print("Recharge");
//    }
//    else{
//      Serial.print("Battery Full");
//    }

  }
} 
