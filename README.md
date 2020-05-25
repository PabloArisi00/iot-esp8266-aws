# Repository of IOT with ESP8266 
##v1.3 Arduino ESP8266 with IOT MQTT + AWS IoT .


## AWS IoT And Arduino ESP8266
###Librerias: 
- #include <ESP8266WiFi.h>
- #include <PubSubClient.h> //Get it from here: https://github.com/knolleary/pubsubclient
- #include <NTPClient.h>
- #include <WiFiUdp.h>
- #include <Wire.h>
- #include <RtcDS3231.h>
- #include "ArduinoJson.h"

###Funciones:
- NTP: Obtiene fecha
- DS3231: Graba en el clock el horario provisto de NTP
          Consulta Horario, Fecha y temperatura
- MQTT: Transforma mensaje en Json
       Suscribe a un Tópico de AWS IoT
       Publica en Topico de IoT el Mensaje 
       Valida si el msg suscripto inicia es diferente de 1 y activa led interno, si es 1 apaga led.
      
      
##TODO:
### 25-5-2020
1 Conectar un display para ver el msg a enviar ( temperatura y hora )
2 Otro Projecto: Conectar Tags de NFC para activar funciones( gestión de wifi y control de acceso internet )
3 Gestión de energía eficiente usando el clock del DS3231 para activar al ESP8266 y usarlo con baterias externas
4 Probar el funcionamiento de Shadow de AWS IoT
5 Usar Kinetics para cargar un ElasticSearch y analizar los datos en tiempo real
