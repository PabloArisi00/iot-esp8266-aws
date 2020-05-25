/* Basado en: ESP8266 AWS IoT example by Evandro Luis Copercini
   Public Domain - 2017
   It connects to AWS IoT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h> //Get it from here: https://github.com/knolleary/pubsubclient
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include "ArduinoJson.h"

 
// CONNECTIONS:
// DS3231 SDA --> SDA
// DS3231 SCL --> SCL
// DS3231 VCC --> 3.3v or 5v
// DS3231 GND --> GND
/* convert and replace with your keys
  $ openssl x509 -in aaaaaaaaa-certificate.pem.crt -out certificates/cert.der -outform DER
  $ openssl rsa -in aaaaaaaaaa-private.pem.key -out certificates/private.der -outform DER
  $ xxd -i certificates
*/
unsigned char arduinoesp_3048_key[] = {...
  0x48, 0x71, 0x7f, 0x8e, 0x42, 0x33, 0x34, 0x72,
  0xfe, 0xea, 0xe3, 0x5c, 0x42, 0x29, 0x0a, 0x02, 0x94, 0x8c, 0x9a, 0xcd,
  0xed, 0x4a, 0x63, 0x5f, 0x92, 0x67, 0xda, 0x7f, 0xbe, 0xc7, 0x2a, 0x50,
  0x50, 0x86, 0x0c
};
unsigned int arduinoesp_3048_key_len = 1191;

unsigned char arduinoesp_3048_der[] = {
  ...0x47, 0xeb, 0xc7, 0xdd, 0xa2, 0x0b, 0xaf, 0x44, 0x1b, 0xa0, 0xce, 0x61,
  0x69, 0xda, 0xe3, 0x2c, 0x5f, 0x61, 0x4f, 0xd2, 0x76, 0xc0, 0xdb, 0xbb,
  0x26, 0x91, 0x60, 0x7e, 0x33, 0xe8, 0x25, 0xa4, 0xeb, 0x79, 0x44, 0xad,
  0xa7, 0x79, 0xda, 0x0b, 0x28, 0xc9, 0x51, 0xbb, 0xc6, 0x08, 0xb4, 0x81,
  0xb1, 0x54, 0x42, 0x6f, 0x34, 0xa9, 0xea, 0x65, 0x4a
};
unsigned int arduinoesp_3048_der_len = 861;

unsigned char aws_root_ca_der[] = {
  ...xa3, 0xfb, 0x9f, 0x40, 0x95, 0x6d, 0x31, 0x54, 0xfc, 0x42, 0xd3, 0xc7,
  0x46, 0x1f, 0x23, 0xad, 0xd9, 0x0f, 0x48, 0x70, 0x9a, 0xd9, 0x75, 0x78,
  0x71, 0xd1, 0x72, 0x43, 0x34, 0x75, 0x6e, 0x57, 0x59, 0xc2, 0x02, 0x5c,
  0x26, 0x60, 0x29, 0xcf, 0x23, 0x19, 0x16, 0x8e, 0x88, 0x43, 0xa5, 0xd4,
  0xe4, 0xcb, 0x08, 0xfb, 0x23, 0x11, 0x43, 0xe8, 0x43, 0x29, 0x72, 0x62,
  0xa1, 0xa9, 0x5d, 0x5e, 0x08, 0xd4, 0x90, 0xae, 0xb8, 0xd8, 0xce, 0x14,
  0xc2, 0xd0, 0x55, 0xf2, 0x86, 0xf6, 0xc4, 0x93, 0x43, 0x77, 0x66, 0x61,
  0xc0, 0xb9, 0xe8, 0x41, 0xd7, 0x97, 0x78, 0x60, 0x03, 0x6e, 0x4a, 0x72,
  0xae, 0xa5, 0xd1, 0x7d, 0xba, 0x10, 0x9e, 0x86, 0x6c, 0x1b, 0x8a, 0xb9,
  0x59, 0x33, 0xf8, 0xeb, 0xc4, 0x90, 0xbe, 0xf1, 0xb9
};
unsigned int aws_root_ca_der_len = 837;

const char* AWS_endpoint = "a3ndp1z76itiwu-ats.iot.us-east-1.amazonaws.com"; //MQTT broker ip
const char* ssid = "Wifi@Home";
const char* password = "33056630";
String ThingID = "";
#define MSG_BUFFER_SIZE  (160)
int iPeriod = 60000;
String sVersion = "v1.3";


// Objetos WIFI y NTPClient   ---------------------------------------------------------------------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiClientSecure espClient;
RtcDS3231<TwoWire> Rtc(Wire);

// Funciones  ---------------------------------------------------------------------

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
    Serial.print((char)payload[i]);

// Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
  Serial.println();
}

PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set  MQTT port number to 8883
long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];
int value = 0;

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
 //   Create a random client ID
    String clientId = "ESP8266Client-id-";
    clientId += String(random(0xffff), HEX);
 
    Serial.print("Client ID: ");
    Serial.println(clientId);
    ThingID = clientId; 
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("inTopic");
      Serial.println("Suscribe in Topic: InTopic");

    
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin (115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  espClient.setCertificate(arduinoesp_3048_der, arduinoesp_3048_der_len);
  espClient.setPrivateKey(arduinoesp_3048_key, arduinoesp_3048_key_len);
  espClient.setCACert(aws_root_ca_der, aws_root_ca_der_len);

  timeClient.begin();
   while(!timeClient.update()){
     timeClient.forceUpdate();
   }
  espClient.setX509Time(timeClient.getEpochTime());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Rtc.Begin();
  /*RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    if (!Rtc.IsDateTimeValid()) 
    {
        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }
  */
    Rtc.SetDateTime(timeClient.getEpochTime());
    RtcDateTime now = Rtc.GetDateTime();

    Serial.print(" Tiempo de NTP:"); Serial.println(timeClient.getEpochTime());
    Serial.print(" Tiempo de Clock DS3231:"); 
    String hora = (String)now.Hour()+":"+(String)now.Minute()+":"+(String)now.Second();
    String dia = (String)now.Year()+"/"+(String)now.Month()+"/"+(String)now.Day();
    Serial.print(dia); Serial.println(hora);
    Serial.println("-----------------------------------------------------------------------------------------------------------");
    
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
      
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > iPeriod) {
    lastMsg = now;
    ++value;
  
    RtcDateTime now = Rtc.GetDateTime();
      String dia = (String)now.Year()+"/"+(String)now.Month()+"/"+(String)now.Day();
      String hora = (String)now.Hour()+":"+(String)now.Minute()+":"+(String)now.Second();
      Serial.print("Time: "); Serial.print(dia); Serial.print(hora);
    
  RtcTemperature  temp = Rtc.GetTemperature();
      Serial.print(" Temperature:"); Serial.println(temp.AsFloatDegC());
      float temperatura = temp.AsFloatDegC();

  //Serial.println("*****************************************************************");Serial.print("BUFFER ->>>> ");Serial.print( MSG_BUFFER_SIZE);

  
  StaticJsonDocument<MSG_BUFFER_SIZE> JSONMessage;
    JSONMessage["thing"] = ThingID;
    JSONMessage["sensor"] = "DS3231";
    JSONMessage["trx"] = value;
    JSONMessage["date"] = dia;
    JSONMessage["time"] = hora;
    JSONMessage["temperature"] = temperatura;
   // JSONMessage["ver"] = sVersion;
       
    char json_string[MSG_BUFFER_SIZE];
    serializeJson(JSONMessage, json_string);
    
    snprintf (msg, MSG_BUFFER_SIZE, json_string );
    Serial.println("Publish message:");
    Serial.println(msg);
    client.publish("outTopic" , msg);
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
    Serial.println("--------------------------------------------------------------------------------");
  }
}
