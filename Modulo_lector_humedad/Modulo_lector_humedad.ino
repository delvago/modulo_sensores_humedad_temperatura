//Código Modulo Lector de Humedad
//Creadores Luis Felipe Duarte, David Vallejo Gómez, Jorge Andres Cardona

//Librerias
#include <WiFi.h>
#include <HTTPClient.h>

//Credenciales de conexión
const char* ssid = "INVERNADERO";
const char* password = "Invernad_2023+";
const char* aeronicDevice = "t71VA2kTJWPDpn4";

/*Pines definidos*/
#define RXD2 16//Pin de recepción del protocolo Serial2
#define TXD2 17//Pin de transmisión del protocolo Serial2

/*********************************************************
 * Variables Globales
 ********************************************************/
char inChar;
String string = "";
String Tem_hum[3];//Variable que recibe las lecturas del Sensor de Teros

//Variables para temporizador
unsigned long tiempo_inicio_serial;
unsigned long tiempo_actual;

/*********************************************************
 * PERIODO DE ENVIO DE DATOS A AERONIC EN MILISEGUNDOS
 * periodo envio = 1*n*1000 : n equivale al numero de segundos
 ********************************************************/
long int periodo_envio = 1*30*1000;
//Se recomienda un tiempo superior a una hora


/*********************************************************
 * Lectura de sensor de humedad del suelo
 ********************************************************/
void sensor() {
   while (Serial2.available()) {
    inChar = Serial2.read();
    if (inChar == '\n') {
      Tem_hum[0] = getValue(string, '+', 1);//para ech20
      Tem_hum[1] = getValue(string, '+', 3);//para ech20
      //Tem_hum[0] = getValue(string, '+', 1);//Para teros12
      //Tem_hum[1] = getValue(string, '+', 2);//Para teros12
      string = "";
    } else {
      string += inChar;
    }
  }
}

/*********************************************************
 * Lectura de comunicación Serial entre Arduino y ESP32
 ********************************************************/
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "-1";
}

//Funcion para envio de datos a Aeronic
void postDataToAeronic(float humedad, float temperatura) {
  // Crea la instancia de HTTPClient
  HTTPClient http;

  // URL de la API de Aeronic
  String url = "https://aeronic.herokuapp.com/api/telemetry/" + String(aeronicDevice);

  // Agrega los encabezados requeridos
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Crea el cuerpo del JSON
  String jsonData = "{\"humedad_modulo\":" + String(humedad) + ", \"temperatura_modulo\":" + String(temperatura) + "}";
  Serial.print("JSON enviado: ");
  Serial.println(jsonData);

  // Realiza la solicitud HTTP POST
  int httpResponseCode = http.POST(jsonData);

  // Verifica la respuesta del servidor
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("HTTP Request failed. Error code: ");
    Serial.println(httpResponseCode);
  }

  // Libera los recursos
  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.print("-----Inicio Modulo Lectura UPB----- "); 
  tiempo_inicio_serial = millis();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectado al WiFi...");
  }
  Serial.println("WiFi conectado"); 
}

void loop() {
  float lectura_humedad;
  float lectura_temperatura;
  float promedio_humedad;
  float promedio_temperatura;
  
  sensor();
  
  //Periodo de sensado
  tiempo_actual = millis();
  if(tiempo_actual - tiempo_inicio_serial >= periodo_envio){
    promedio_humedad = 0;
    promedio_temperatura = 0;
    
    //Promedio de las lecturas
    for (int i=0; i<10; i++) {
    float humedad = atof(Tem_hum[0].c_str());
    lectura_humedad=((4.3e-6*pow(humedad,3))-(5.5e-4*pow(humedad,2))+(2.92e-2*humedad)-(5.3e-2))*100;// para ech20
    //lectura_humedad=((3.879e-4*humedad)-0.6956)*100;//para teros12: ecucación para obtener humedad del suelo
    float lectura_temperatura = atof(Tem_hum[1].c_str());
    
    promedio_humedad = promedio_humedad + lectura_humedad;
    promedio_temperatura = promedio_temperatura + lectura_temperatura;

    tiempo_inicio_serial = tiempo_actual;
  }
  
  //Promedio de las lecturas
  promedio_humedad=promedio_humedad/10;
  promedio_temperatura=promedio_temperatura/10;

  //Envio de datos a la nube
  postDataToAeronic(promedio_humedad, promedio_temperatura);

  //Envio de datos al serial
  Serial.print("Humedad Suelo: ");
  Serial.print(promedio_humedad);
  Serial.print(" %, ");
  Serial.print("Temperatura Suelo: ");
  Serial.print(promedio_temperatura);
  Serial.println(" C");
  
  }
}
