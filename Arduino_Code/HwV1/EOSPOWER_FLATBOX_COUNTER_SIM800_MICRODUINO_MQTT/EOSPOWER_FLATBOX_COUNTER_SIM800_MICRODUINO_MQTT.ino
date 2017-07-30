/*
 Basic MQTT example 
 
  - connects to an MQTT server
  - publishes "hello world" to the topic "test/out"
  - subscribes to the topic "test/in"
*/
#include <Time.h>
#include <sim800Client.h>
#include <GSMPubSubClient.h>
#include <ArduinoJson.h>                                              //https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7
#include <TimeAlarms.h>
#include "settings.h"
//----------------------------------------------------------------------Variables de verificacion de fallas de capa de conexion con servicio
int failed, sent, published;
//----------------------------------------------------------------------Declaracion de Variables Globales (procuar que sean las minimas requeridas.
String ISO8601;                                                       //Variable para almacenar la marca del timepo (timestamp) de acuerdo al formtao ISO8601

//----------------------------------------------------------------------Variables Para casignacion de pines para los led RGB
                                                                      //NOTA PARA EL MODULO ESP8266 NO se podra asignar el PIN 16 debido a que este resete el modulo
const int rojo = 10;                                                  //Asignacion D10 Para el color Rojo del LeD RGB
const int verde = 9;                                                 //Asignacion D9 Para el color Verde del LeD RGB
const int azul = 8;                                                  //Asignacion D8 Para el color Azul del LeD RGB
//----------------------------------------------------------------------Variables Para casignacion de pines para los led RGB
int DeviceState = 0;
unsigned long lastNResetMillis;                                       //Variable para llevar conteo del tiempo desde la ultima publicacion 
int hora = 0;
const char* cLat = "14.598805";
const char* cLong = "-90.511142";
String Lat = "14.598805";
String Long = "-90.511142";
/*
Measuring AC Current Using ACS712
*/
const int RedPinIn = A0;
const int GreenPinIn = A1;
const int YellowPinIn = A2;
String LightColor = "";
String OldLightColor = "";
boolean RED = false;
boolean GREEN = false;
boolean YELLOW = false;

double Voltage = 0;

sim800Client s800;
char imeicode[16];

// Update these with values suitable for your network.
//byte server[] = { 192, 168, 1, 199 };
char server[] = "counter.flatbox.io";

//----------------------------------------------------------------------denifinir el parpadeo de coloers del led RGB ---- Puertos D8 (rojo) d7 (verde) D6 (Azul)
void flashBlue(){
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, HIGH);
  delay(200);
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);
}

void BlueLight(){
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, HIGH);
}

void flashRed(){
  digitalWrite(rojo, HIGH);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);
  delay(200);
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);
}

void RedLight(){
  digitalWrite(rojo, HIGH);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);  
}

void flashGreen(){
  digitalWrite(rojo, LOW);
  digitalWrite(verde, HIGH);
  digitalWrite(azul, LOW);
  delay(200);
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);
}

void GreenLight(){
  digitalWrite(rojo, LOW);
  digitalWrite(verde, HIGH);
  digitalWrite(azul, LOW);
}
void flashPurple(){
  digitalWrite(rojo, HIGH);
  digitalWrite(verde, LOW);
  digitalWrite(azul, HIGH);
  delay(200);
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);
}

void Purple(){
  digitalWrite(rojo, HIGH);
  digitalWrite(verde, LOW);
  digitalWrite(azul, HIGH);
}

void flashWhite(){
  digitalWrite(rojo, HIGH);
  digitalWrite(verde, HIGH);
  digitalWrite(azul, HIGH);
  delay(200);
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);
}

void WhiteLight(){
  digitalWrite(rojo, HIGH);
  digitalWrite(verde, HIGH);
  digitalWrite(azul, HIGH);
}
void lightsOff(){
  digitalWrite(rojo, LOW);
  digitalWrite(verde, LOW);
  digitalWrite(azul, LOW);
}
//----------------------------------------------------------------------Funcion remota para administrar las actulizaciones remotas de las variables configurables desde IBMbluemix
void handleUpdate(byte* payload) {                                    //La Funcion recibe lo que obtenga Payload de la Funcion Callback que vigila el Topico de subcripcion (Subscribe TOPIC)
  StaticJsonBuffer<300> jsonBuffer;                                  //Se establece un Buffer de 1o suficientemente gande para almacenar los menasajes JSON
  JsonObject& root = jsonBuffer.parseObject((char*)payload);          //Se busca la raiz del mensaje Json convirtiendo los Bytes del Payload a Caracteres en el buffer
  if (!root.success()) {                                              //Si no se encuentra el objeto Raiz del Json
    Serial.println(F("ERROR en la Letura del JSON Entrante"));        //Se imprime un mensaje de Error en la lectura del JSON
    return;                                                           //Nos salimos de la funcion
    }                                                                 //se cierra el condicional
  Serial.println(F("handleUpdate payload:"));                         //si se pudo encontrar la raiz del objeto JSON se imprime u mensje
  root.prettyPrintTo(Serial);                                         //y se imprime el mensaje recibido al Serial  
  Serial.println();                                                   //dejamos una linea de pormedio para continuar con los mensajes de debugging
  JsonObject& d = root["d"];                                          //Se define el objeto "d" como  la raiz del mensaje JSON
  JsonArray& fields = d["fields"];                                    //se define el arreglo "fields" del JSON
  for(JsonArray::iterator it=fields.begin();                          //se daclara una rutina para buscar campos dentro del arreglo 
      it!=fields.end();                                               //si no se encuentra lo que se busca se termina la busqueda
      ++it) {                                                         //se busca el siguiente campo
        JsonObject& field = *it;                                      //se asigna lo que tenga el iterador de campos field
        const char* fieldName = field["field"];                       //se crea l avariable nombre de campo
        if (strcmp (fieldName, "metadata") == 0) {                    //Se confirma valida si el campo contiene "metadata"
          JsonObject& fieldValue = field["value"];                    //Se asigna el valor de campo a el objeto de JSON
          if (fieldValue.containsKey("UInterval")) {                  //Si el Valor del campo contiene la LLave "publishInterval"
            UInterval = fieldValue["UInterval"];                      //asignar ese valor a la variable global "publishInterval"
            Serial.print(F("UInterval:"));                            //se imprime un mensaje con ka variable que acaba de modificarse remotamente
            Serial.println(UInterval);                                //se imprime el nuevo valor de la variable actualizada
          }
        }
        if (strcmp (fieldName, "deviceInfo") == 0){                   //Se confirma valida si el campo contiene "deviceInfo"                  
          JsonObject& fieldValue = field["value"];                    //Se asigna el valor de campo a el objeto de JSON
          if (fieldValue.containsKey("Lat")) {                  //Si el Valor del campo contiene la LLave "fwVersion"
            cLat = fieldValue["Lat"];                      //asignar ese valor a la variable global "FWVERSION"
            Serial.print(F("Lat:"));                            //se imprime un mensaje con ka variable que acaba de modificarse remotamente
            Serial.println(Lat);                                //se imprime el nuevo valor de la variable actualizada
          }
          if (fieldValue.containsKey("Long")) {                  //Si el Valor del campo contiene la LLave "server"
            cLong = fieldValue["Long"];                      //asignar ese valor a la variable global "server"
            Serial.print(F("Long:"));                            //se imprime un mensaje con ka variable que acaba de modificarse remotamente
            Serial.println(Long);                                //se imprime el nuevo valor de la variable actualizada
          }
        }
      }
}

//----------------------------------------------------------------------Funcion remota para mandar a dormir el esp despues de enviar un RFID
void handleResponse (byte* payloadrsp) {
  StaticJsonBuffer<200> jsonBuffer;                                   //Se establece un Buffer de 1o suficientemente gande para almacenar los menasajes JSON
  JsonObject& root = jsonBuffer.parseObject((char*)payloadrsp);       //Se busca la raiz del mensaje Json convirtiendo los Bytes del Payload a Caracteres en el buffer
  if (!root.success()) {                                              //Si no se encuentra el objeto Raiz del Json
    Serial.println(F("ERROR en la Letura del JSON Entrante"));        //Se imprime un mensaje de Error en la lectura del JSON
    return;                                                           //Nos salimos de la funcion
  }                                                                   //se cierra el condicional
  
  Serial.println(F("handleResponse payload:"));                       //si se pudo encontrar la raiz del objeto JSON se imprime u mensje
  root.printTo(Serial);                                         //y se imprime el mensaje recibido al Serial  
  Serial.println();                                                   //dejamos una linea de pormedio para continuar con los mensajes de debugging
  
  JsonObject& d = root["d"];                                          //Se define el objeto "d" como  la raiz del mensaje JSON
  JsonArray& fields = d["fields"];                                    //se define el arreglo "fields" del JSON
  for(JsonArray::iterator it=fields.begin();                          //se daclara una rutina para buscar campos dentro del arreglo 
      it!=fields.end();                                               //si no se encuentra lo que se busca se termina la busqueda
      ++it) {                                                         //se busca el siguiente campo
        JsonObject& field = *it;                                      //se asigna lo que tenga el iterador de campos field
        const char* fieldName = field["field"];                       //se crea l avariable nombre de campo
        if (strcmp (fieldName, "metadata") == 0) {                    //Se confirma valida si el campo contiene "metadata"
          JsonObject& fieldValue = field["value"];                    //Se asigna el valor de campo a el objeto de JSON
          if (fieldValue.containsKey("State")) {                      //Si el Valor del campo contiene la LLave "DeviceState"
            DeviceState = fieldValue["State"];                    //asignar ese valor a la variable global "DeviceState"
            Serial.print(F("DeviceState:"));                                 //se imprime un mensaje con ka variable que acaba de modificarse remotamente
            Serial.println(DeviceState);                                    //se imprime el nuevo valor de la variable actualizada
            if (DeviceState == 0){
              Serial.println(F("lights off"));
              lightsOff();
              delay(100);
            }            
          }
        }
      }
}

//----------------------------------------------------------------------Funcion de vigilancia sobre mensajeria remota desde el servicion de IBM bluemix
void callback(char* topic, byte* payload, unsigned int payloadLength){//Esta Funcion vigila los mensajes que se reciben por medio de los Topicos de respuesta;
  Serial.print(F("callback invoked for topic: "));                    //Imprimir un mensaje señalando sobre que topico se recibio un mensaje
  Serial.println(topic);                                              //Imprimir el Topico
  
  if (strcmp (responseTopic, topic) == 0) {                            //verificar si el topico conicide con el Topico responseTopic[] definido en el archivo settings.h local
    handleResponse(payload);
    //return; // just print of response for now                         //Hacer algo si conicide (o en este caso hacer nada)
  }
  
  if (strcmp (rebootTopic, topic) == 0) {                             //verificar si el topico conicide con el Topico rebootTopic[] definido en el archivo settings.h local
    Serial.println(F("Rebooting..."));                                //imprimir mensaje de Aviso sobre reinicio remoto de unidad.
    asm volatile ("  jmp 0");                                                   //Emitir comando de reinicio para ESP8266
  }
  
  if (strcmp (updateTopic, topic) == 0) {                             //verificar si el topico conicide con el Topico updateTopic[] definido en el archivo settings.h local
    handleUpdate(payload);                                            //enviar a la funcion handleUpdate el contenido del mensaje para su parseo.
  } 
  /*char mypl[48];
  Serial.println(length);
  memcpy(mypl,payload,length);
  mypl[length]=char(0);
  Serial.print(F("receive: "));
  Serial.print(topic);
  Serial.print(F("->"));
  Serial.println(mypl);*/
}

PubSubClient client(server, 1883, callback, s800);

void SetTime(){
   #define RESULTAT 100
   #define RESULTT 41
  char result[RESULTAT];
  char Tresult[RESULTT];
  String ATstamp ="";
  s800.ATcommand("+CGSN",result); 
  delay(300);
  s800.ATcommand("+SAPBR=3,1,\"Contype\",\"GPRS\"",result);
  Serial.println(result);
  delay(300);
  s800.ATcommand("+SAPBR=3,1,\"APN\",\"broadband.tigo.gt\"",result);
  Serial.println(result);
  delay(300);
  s800.ATcommand("+SAPBR=1,1",result);
  Serial.println(result);
  delay(300);
  s800.ATcommand("+CNTPCID=1",result);
  Serial.println(result);
  delay(300); 
  s800.ATcommand("+CNTP=\"129.6.15.30\",-24",result); 
  Serial.println(result);
  delay(300);
  s800.ATcommand("+CNTP",result); 
  Serial.println(result);
  delay(300);
  s800.ATcommand("+CCLK?",Tresult); 
  Serial.println(Tresult);
  delay(300);
  for(int k=0; k<RESULTT; k++){
      ATstamp += String(Tresult[k]);
  }
  int firstindex = ATstamp.indexOf('"');
  int secondindex = ATstamp.indexOf('"', firstindex + 1);
  String command = ATstamp.substring(0, firstindex);
  String Stamp = ATstamp.substring(firstindex + 1, secondindex);
  ISO8601 = Stamp;
  Serial.print(F("ISO8601:"));
  Serial.println(ISO8601);
  delay(300);
}
  
void setup(){
  pinMode(rojo, OUTPUT);
  pinMode(verde, OUTPUT);
  pinMode(azul, OUTPUT);
  Serial.begin(115200);
  WhiteLight();
  Serial.println("SIM800 Shield testing.");
  Purple();
  for (int i=0; i<10; i++){
    delay(5000);
    Serial.println("try to init sim800");
    #ifdef HARDWARESERIAL
    if (s800.init( 7, 6)) break;
    #else
    if (s800.init(&Serial1 , 7, 6)) break;
    #endif
  }
  Serial.println("try to setup sim800");
  BlueLight();
  s800.setup();  
  s800.stop();
  s800.TCPstop();
  s800.getIMEI(imeicode);
  Serial.print("IMEI: ");
  Serial.println(imeicode);
  flashWhite();
  SetTime();
  while (!s800.TCPstart(GSMAPN,GSMUSER,GSMPASSWORD)) {
    Serial.println("TCPstart failed");
    s800.TCPstop();
    delay(1000);
  }
  Serial.println("TCPstart started");
  flashBlue();
  while (!client.connect(imeicode,"flatbox","FBx_admin2012")) {
    Serial.println("connect failed");
    delay(1000);
  }
  flashGreen();
  Serial.println("connected");
  client.publish("iotdevice-1/mgmt/manage/eospower","Iniciando Prueba de Semaforo");
  client.subscribe("iotdm-1/device/update/eospower");
  lightsOff();  
}

//----------------------------------------------------------------------Funcion de REConexion a Servicio de MQTT
void reconnect() {
  int retry = 0;
  // Loop until we're reconnected
  while (!client.connected()) {    
    Serial.print(F("Attempting MQTT connection..."));
    if (client.connect(imeicode,"flatbox","FBx_admin2012")) {
      Serial.println(F("connected"));
      } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.print(F(" try again in 3 seconds,"));
      Serial.print(F(" retry #:"));
      Serial.println(retry);
      if (retry > 10){
        retry=0;
        asm volatile ("  jmp 0");        
      }
      retry++;
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}
//----------------------------------------- Declaracion de sensor de luz

boolean GetRedStatus(){
   boolean RedStatus;
   Voltage = getVPP(RedPinIn);
   if(Voltage *1000 >=100){
    RedStatus = true;
    LightColor = "RED";
    RedLight();
   }else{
    RedStatus = false;
   }
   return RedStatus;   
}

boolean GetGreenStaus(){
   boolean GreenStaus;
   Voltage = getVPP(GreenPinIn);
   if(Voltage * 1000 >=100){
    GreenStaus = true;
    LightColor = "Green";
    GreenLight();
   }else{
    GreenStaus = false;
   }
    return GreenStaus;
}

boolean GetYellowStaus(){
   boolean YellowStaus;
   Voltage = getVPP(YellowPinIn);
   if(Voltage * 1000 >= 100){
    YellowStaus = true;
    LightColor = "Yellow";
    WhiteLight();
    }else{
    YellowStaus = false;
   }
   return YellowStaus;   
}

//---------------------------------------------------------------------------funcion de enviode Datos Boton RF_Boton.-----------------------
void publishLightColor(String IDModulo, String LColor,String Latitude,String Longitude, String Tstamp) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& Lightdata = d.createNestedObject("Lightdata");
  Lightdata["IMEI"] = IDModulo;
  Lightdata["Semaforo"] = LColor;
  Lightdata["Lat"] = Latitude;
  Lightdata["Long"] = Longitude;
  Lightdata["Tstamp"] = Tstamp;
  char MqttLightdata[500];
  root.printTo(MqttLightdata, sizeof(MqttLightdata));
  Serial.println(F("publishing device publishTopic metadata:")); 
  Serial.println(MqttLightdata);
  sent ++;
  if (client.publish(publishTopic, MqttLightdata)){
    Serial.println(F("enviado data de semaforo: OK"));
    published ++;
    failed = 0; 
  }else {
    Serial.println(F("enviado data de semaforo: FAILED"));
    failed ++;
  }
}

void loop(){
  RED = GetRedStatus();
  GREEN = GetGreenStaus();
  YELLOW = GetYellowStaus();
  if ( (RED == 0) && (GREEN == 0) && (YELLOW == 0) ){
    LightColor = "OFF";
    lightsOff();
  }
  if (LightColor != OldLightColor){
    OldLightColor = LightColor;
    Serial.print(F("Light is: "));
    Serial.println(LightColor);
    publishLightColor(imeicode, LightColor,cLat,cLong,ISO8601);  // publishRF_Boton(String IDModulo, String EventID, String Tstamp)
  }
  
  NormalReset();
 
  if ( millis() - RetardoLectura > 30*60* UInterval){
  client.publish("iotdevice-1/mgmt/manage/eospower","Mensaje de control");
  RetardoLectura = millis(); //Actulizar la ultima hora de envio
 }

 // VERIFICAMOS CUANTAS VECES NO SE HAN ENVIOADO PAQUETES (ERRORES)
   if (failed >= FAILTRESHOLD){
    failed =0;
    published =0;
    sent=0;    
    asm volatile ("  jmp 0");
  }
  
 //verificar que el cliente de Conexion al servicio se encuentre conectado
 if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
//--------------------------------------------------------------------------Funcion de Reinicio automatico cada 24h (prevenir buffer overflow.!!!------------------------------------------------------------------------------

void NormalReset(){
  if (millis()- lastNResetMillis > 60 * 60 * UInterval){
    hora++;
    if (hora > 24){
      String msg = ("24h NReset");  
      String Msg = ( imeicode + msg + "@:" + ISO8601);
      client.publish("iotdevice-1/mgmt/manage/eospower","Mensaje de control Reser24N");
      Serial.println(Msg);
      void disconnect ();
      hora = 0;
      asm volatile ("  jmp 0");
    }
     lastNResetMillis = millis(); //Actulizar la ultima hora de envio
  }
}

float getVPP(int Pin){
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here
  uint32_t start_time = millis();
  while((millis()-start_time) < 250){ //sample for 1/4 Sec
    readValue = analogRead(Pin); // see if you have a new maxValue
       if (readValue > maxValue){
        /*record the maximum sensor value*/
        maxValue = readValue;
       }
       if (readValue < minValue){
        /*record the maximum sensor value*/
        minValue = readValue;
       }
  }
  // Subtract min from max
  result = ((maxValue - minValue) * 5.0)/1024.0;
  return result;
}
