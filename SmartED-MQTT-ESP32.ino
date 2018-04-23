// SmartED Can DATA Sniffer to MQTT over WIFI on ESP32

/*
 * MCP2515  = WEMOS ESP32
 * VCC      =       5V
 * GND      =      GND
 * CS       =       P5
 * SO       =      P19
 * SI       =      P23
 * SCK      =      P18
 * INT      =      P22
 */

//bool DEBUG = true;
bool DEBUG = false;

#include <WiFi.h>
#include <PubSubClient.h>
#include <mcp_can.h>
#include <SPI.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];

#define CS 5
MCP_CAN CAN0(CS);                               

const char* ssid = "Mobile-WIFI-Hotspot";
const char* password = "SuperSecurePass";

#define mqtt_server "my.mqtt.server"
#define owner "SmartED"
#define mqtt_user "MyUsername"
#define mqtt_password "MyPassword"
#define status_topic "status/SmartED"
#define data_topic "data/SmartED"

int INTERVALL = 10;    // alle n Sek Werte an den MQTT Server übertragen


WiFiClient espClient;
PubSubClient client(espClient);

float               SPEED = 0;             // Speed
long unsigned int   KILOMETERSTAND = 0;    // km
unsigned int        RESTREICHWEITE = 0;    // km
unsigned int        POWERLEVEL = 0;        // 33, 66, 99
float               HVA = 0;               // HV-Akku-Strom
float               HVV = 0;               // HV-Akku-Spannung
long signed int     HVP = 0;               // HV-Akku-Leistung
float               SOC = 0;               // SOC State of Charge
float               rSOC = 0;              // realSOC State of Charge
float               ECOall = 0;            // ECO-Wert
float               ECObre = 0;            // ECO-Wert Bremsen
float               ECOrol = 0;            // ECO-Wert Rollen
float               ECOacc = 0;            // ECO-Wert Beschleunigen
float               TEMPERATUR = 0;        // TEMPERATUR

int TIMER = millis() - 1000 * INTERVALL/2;

void mqttconnect() {   // Loop until reconnected 
  while (!client.connected()) {
    Serial.print("MQTT connecting ...");
    String clientId = "SmartED";

    /* connect now */
    if (client.connect( owner, mqtt_user, mqtt_password, status_topic, 2, true, "offline")) {
      client.publish(status_topic, "online", true);
      Serial.println("connected");
    } else {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");  // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  
  pinMode(CS, OUTPUT);  
  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");
  
  CAN0.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.

  pinMode(22, INPUT);                            // Setting pin 22 for /INT input
  
  Serial.println("Wemos-ESP32 - MCP2515-CAN - MQTT");
}

void loop()
{
    /* if MQTT client was disconnected then try to reconnect again */
    if (!client.connected()) {
          Serial.print("MQTT reconnecting ...........................");
      mqttconnect();
    } 


    if(!digitalRead(22))                        // If pin 22 is low, read receive buffer
    {
      CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)

      switch (rxId) {
        
      case (0x200):
      SPEED = ((rxBuf[2] * 256 ) + (rxBuf[3])) / 18;
      if ( DEBUG ) { Serial.print("Speed               - "); Serial.print(SPEED); Serial.print("   "); }
      break;
        
      case (0x412):
      KILOMETERSTAND = (rxBuf[2] * 65536 ) + (rxBuf[3] * 256 ) + rxBuf[4];
      if ( DEBUG ) { Serial.print("Kilometerstand      - "); Serial.print(KILOMETERSTAND); Serial.print("   "); }
      break;
      
      case (0x318):
      RESTREICHWEITE = rxBuf[7];
      POWERLEVEL = rxBuf[5];
      if ( DEBUG ) { Serial.print("Reichweite Powerbar - "); Serial.print(RESTREICHWEITE); Serial.print(" "); Serial.print(POWERLEVEL); Serial.print("   "); }
      break;
      
      case (0x3F2):
      ECOall = rxBuf[3]*0.5;
      ECObre = rxBuf[2]*0.5;
      ECOrol = rxBuf[1]*0.5;
      ECOacc = rxBuf[0]*0.5;
      if ( DEBUG ) { Serial.print("ECO Werte           - "); Serial.print(ECOall); Serial.print(" "); Serial.print(ECObre); Serial.print(" "); Serial.print(ECOrol); Serial.print(" "); Serial.print(ECOacc); Serial.print("   "); }
      break;
      
      case (0x448):
      if ( rxBuf[0] == 0x0F ){
      HVV = (rxBuf[6] * 256 + rxBuf[7]) * 0.1;
      if ( DEBUG ) { Serial.print("HV Spannung         - "); Serial.print(HVP); Serial.print("   "); Serial.print(HVV); Serial.print("   "); }
      } else {
      if ( DEBUG ) { Serial.print("HV Spannung         - "); Serial.print("ungültiger Wert"); Serial.print("   "); }
      }
      break;
      
      case (0x508):
      HVA = (((rxBuf[2] & 0x3F) * 256 + rxBuf[3]) * 0.1) - 819.2; 
      if ( DEBUG ) { Serial.print("HV Strom            - "); Serial.print(HVA); Serial.print("   "); }
      break;
      
      case (0x518):
      SOC = rxBuf[7]*0.5;
      if ( DEBUG ) { Serial.print("SOC                 - "); Serial.print(SOC); Serial.print("   "); }
      break;
      
      case (0x2D5):
      rSOC = ((rxBuf[4] & 0x0F) * 256 + rxBuf[5])*0.1;      
      if ( DEBUG ) { Serial.print("realSOC             - "); Serial.print(rSOC); Serial.print("   "); }
      break;

      case (0x408):
      TEMPERATUR = ((rxBuf[4] - 50 ) - 32 ) / 1.8;      
      if ( DEBUG ) { Serial.print("TEMPERATUR          - "); Serial.print(TEMPERATUR); Serial.print("   "); }
      break;
          
      }
      
      HVP = HVV * HVA;
      
      if ( DEBUG ) {
      Serial.print("ID: ");                     // Print the message ID.
      Serial.print(rxId, HEX);

      Serial.print("  Data: ");
      for(int i = 0; i<len; i++)                // Print each byte of the data.
      {
        if(rxBuf[i] < 0x10)                     // If data byte is less than 0x10, add a leading zero.
        {
          Serial.print("0");
        }
        Serial.print(rxBuf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      }

    }

     if ( millis() - TIMER > 1000 * INTERVALL ){

      if ( DEBUG ) {
      Serial.println("Werte werden übertragen");
      Serial.printf("Speed             : %7.1f km/h\n", SPEED );
      Serial.printf("Kilometerstand    : %7.1u km\n", KILOMETERSTAND );
      Serial.printf("Reichweite        : %7.1u km\n", RESTREICHWEITE );
      Serial.printf("Powerlevel        : %7.1u %%\n", POWERLEVEL );
      Serial.printf("Akku Strom        : %7.1i A\n", HVA );
      Serial.printf("Akku Spannung     : %7.1f V\n", HVV );
      Serial.printf("Akku Leistung     : %7.1s W\n", HVP );
      Serial.printf("SOC               : %7.1f %%\n", SOC );
      Serial.printf("rSOC              : %7.1f %%\n", rSOC );
      Serial.printf("ECO               : %7.1f %%\n", ECOall );
      Serial.printf("ECO Bremsen       : %7.1f %%\n", ECObre );
      Serial.printf("ECO Rollen        : %7.1f %%\n", ECOrol );
      Serial.printf("ECO Beschleunigen : %7.1f %%\n", ECOacc );
      Serial.printf("Temperatur        : %7.1f C\n", TEMPERATUR );
      Serial.println("--");
      }

      String DATENSATZ;
      DATENSATZ += "*";
      DATENSATZ += String(KILOMETERSTAND).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(RESTREICHWEITE).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(POWERLEVEL).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(HVA).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(HVV).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(HVP).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(SOC).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(rSOC).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(ECOall).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(ECObre).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(ECOrol).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(ECOacc).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(SPEED).c_str();
      DATENSATZ += "*";
      DATENSATZ += String(TEMPERATUR).c_str();
      DATENSATZ += "*";
      
      client.publish(data_topic, String(DATENSATZ).c_str(), true); // Werte zum MQTT-Server senden

      TIMER = millis();
     }
   
}



