
/* ESP8266 BROCHAGE

                      ______________________
                ADC0 | A0 o            [o D0|  GPIO16   --> a connecter avec RST pour DeeplSleep
VCC des capteurs  5V | VU o           / o D1|  GPIO5    --> i2c SLK
                 GND |  G o          /  o D2|  GPIO4    --> i2c SDA
              GPIO10 | S3 o         /   o D3|  GPIO0
               GPIO9 | S2 o        /    o D4|  GPIO2    --> DHT pin S
                MOSI | S1 o       /     o 3V|  3.3V
                  CS | SC o      /      o G |  GND      --> GND des capteurs
                MISO | SO o     /       o D5|  GPIO14
                SCLK | SK o    /        o D6|  GPIO12   
                 GND |  G o   /         o D7|  GPIO13
                3.3V | 3V o  /          o D8|  GPIO15   
                  EN | EN o /           o RX|  GPIO3
               Reset |RST o]            o TX|  GPIO1
                 GND |  G o             o G |  GND      
                 Vin |VIN o             o 3V|  3.3V    
                     |         ____         |
                     |________/====\________|



*/

#include <ESP8266Ping.h>
#include <ESP8266Ping.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // librairie pour configurer une liste de point d'accès (Router 1, router 2, etc.)
#include <PubSubClient.h> //Librairie pour la gestion Mqtt 

#include <Adafruit_SGP30.h>
#include <Adafruit_VEML7700.h>

#include <DHTesp.h>

/**
 * Temps a fixer pour le deepsleep, ne pas oublier de connecter la broche D0 a RST
 */
#define TIME_TO_SLEEP           20 // en mn


/**
 * GPIO du SIGNAL du DHT
 */
#define DHT_PIN                 2

#define ESP8266

/**
 * si mis a 1, le Serial affichera des message pour debugger
 */
#define DEBUGING                 1


/**
 * variable globale pour stocker la mesur durant le setup, c'est mieux de demander une mesure avant de commencer une connexion wifi
 * car il draine beaucoup de courant au risque d'en patir sur le DHT
 */
float temp;
float hum;


/**
 * renvoyer la tension VCC sur l'adc pour pouvoir la mesurer, cela prive l'utilisation de l'adc !
 */
ADC_MODE(ADC_VCC);

/**
 * déclaration des objets des capteurs
 */
DHTesp dht;
Adafruit_SGP30 sgp;
Adafruit_VEML7700 veml;

/**
 * variable d'erreur pour les capteurs
 */
int8_t sgp_error = 1, veml_error = 1, dht_error = 1; 


/**
 * Addresse du broker (Raspberry), pour le ping qui permet de savoir si le broker est connecté au réseau
 */
  const IPAddress remote_ip(192, 168, 1,200);

/**
 * Addresse du router(point d'accès)
 */
  const IPAddress router_ip(192,168, 1,1);

/**
 * masque de sous réseau
 */
  IPAddress subnet(255, 255, 255, 0);

/**
 * Configuration WIFI
 */
  const char* ssid = "SFR_D400";

  const char* password = "dmjq3fvp87csqq7zys79";

/**
 * L'adresse IP que demandera l'esp du router
 */
  const IPAddress staticIP(192, 168, 1, 250);

/**
 * Configuration du MQTT
 */
  const char* mqtt_server = "192.168.1.200 "; //Adresse IP du Broker Mqtt (Raspberry)
  const int mqttPort = 1883;                  //port utilisé par le Broker 


// objet pouvant contenir plusieurs ssid
ESP8266WiFiMulti WiFiMulti;

/**
 * déclaration du client WIFI
 */
WiFiClient espClient;

/**
 * déclaration du client MQTT
 */
PubSubClient client(espClient);


/*!
*
* @brief configure la connexion wifi de l'ESP
* 
* ssid et password sont définis en variable globale
*/

void setup_wifi()
{
  int tentatives = 0;

  WiFi.config(staticIP, subnet, router_ip);
  // cette ligne permet d'ajouter d'autre SSID, si jamais le premier n'est pas trouver ainsi de suite, vous pouvez en ajouter autant que besoin
  WiFiMulti.addAP(ssid, password);
  // WiFiMulti.addAP("Pc Hamza", "hamza123");
  #if  DEBUGING == 1
  
    Serial.print("\n\nconnection au wifi : ");
    Serial.print(ssid);
  
  #endif
  while ( WiFiMulti.run() != WL_CONNECTED && tentatives < 20) {
    delay ( 1000 );
    tentatives++;
    #if DEBUGING == 1
      Serial.print ( "." );
    #endif
  }
  if(tentatives >= 20){
    //on dort pour 10mn
    ESP.deepSleep(10*60*1000000);
    
  }
  #if DEBUGING == 1
  
    Serial.println("");
    Serial.println("WiFi connecté");
    Serial.print("MAC : ");
    Serial.println(WiFi.macAddress());
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());
  #endif

}


/*!
* @brief se connecte au broker MQTT
*
*/
void connect_mqtt(){
  while (!client.connected()) {
    #if(DEBUGING)
      Serial.println("Connection au serveur MQTT ...");
    #endif
    if (client.connect("ESP32Client",NULL,NULL,0,2,0,0,1)) {
      #if(DEBUGING)
        Serial.println("MQTT connecté");
      #endif
    }
    else {
      #if(DEBUGING)
        Serial.print("echec, code erreur= ");
        Serial.println(client.state());
        Serial.println("nouvel essai dans 2s");
      #endif
    delay(2000);
    }
  }
}

/*!
*
* @brief configure la connexion au serveur mqtt.
* 
*   Utilise variable global description donc pas besoin de parametre, mais il faut declarer et définir en vriable globale mqtt_server et mqttPort.
*
*/

void test_and_connexion_mqtt()
{
  /*
  * Parfois ca ne marche pas car le router bloque les pings entre devices par sécurité c'est le cas avec le partage de connexion wifi
  */
 #if DEBUGING == 1
 
    Serial.print("ping ");
    Serial.print(router_ip);
 
 #endif


  while(!Ping.ping(router_ip)) {
    if(DEBUGING) Serial.print(".");
    //delay(500);
  }
  #if DEBUGING == 1
  {
    Serial.println();
    Serial.print(router_ip);
    Serial.println(" has responded Successfully!");
    Serial.print("ping Mqtt server...");
  }
  #endif
  while(!Ping.ping(remote_ip)) {
    
    Serial.print('.');
  }
  #if DEBUGING == 1
  
    Serial.println("Success!");
  #endif
  client.setServer(mqtt_server, mqttPort);

  connect_mqtt();

}


/*!
* @brief test et initialise le capteur SGP30
*
*/
void sgp_testAndInit()
{
  if (!sgp.begin())
  {// test de l'existence du capteur sur la liaision i2c
    sgp_error = 1;
    #if(DEBUGING) 
      Serial.println("fail : sgp not found");
    #endif

  }
  else
  {
    #if(DEBUGING) 
      Serial.println("Success : sgp found");
    #endif
    //sgp.setHumidity(getAbsoluteHumidity(dht.getTemperature(),dht.getHumidity()));
    delay(1000);
    sgp.IAQinit();
    delay(15*1000);
        
    if(! sgp.IAQinit())// commande le capteur pour commencer l'algorithme Init_Air_Quality
    {
      #if(DEBUGING) 
        Serial.println("fail : IAQ not initialized");
      #endif
      sgp_error = 2;
    }
    else
    {
      #if(DEBUGING) 
        Serial.println("Success : sgp IAQ initialized");
      #endif
      sgp.IAQmeasure();
      delay(2000);
      sgp.IAQmeasureRaw();
      delay(2000);
      sgp_error = 0;
          
    }

  }
}

/*!
* @brief test et configure le capteur VEML7700
*/

void veml_test()
{
  if(veml.begin())
  {
    #if(DEBUGING) 
      Serial.println("Success : VEML found");
    #endif
    veml.powerSaveEnable(false);
    delay(1);
    veml.setLowThreshold(10000);
    veml.setHighThreshold(20000);
    veml_error = 0;
  }
}


/*!
* @brief test et mesure l'humidité et la temperature
*
*/

void dht_testAndMeasure()
{
   if(dht.getStatus() == 0)
  {
    #if(DEBUGING)
      Serial.print("SUCCESS : DHT ");
      Serial.println(dht.getStatusString());
    #endif

    dht_error = 0;
    temp = dht.getTemperature();
    hum = dht.getHumidity();
  }
}



/*!
*
* @brief publie un flottant en mqtt sur le topic
*
* @param  topic
* topic mqtt
* @param  value
* valeur flottante a envoyer
* 
*/
void mqtt_publish(String topic, float value){
  client.loop(); 

  char top[topic.length()+1];

  topic.toCharArray(top,topic.length()+1);
  char val[100];

  String t_str = String(value);

  t_str.toCharArray(val, t_str.length() + 1);
  #if(DEBUGING)
    Serial.println(topic);
  #endif
  client.publish(top,val);
  
}

/*! 
* @brief return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

//--------------------------------------------------------------------------------------------------------------------------------
void setup() {



  #if DEBUGING == 1

  {
    Serial.begin(9600); // pour le debugging
    Serial.println(); //saut de ligne
  }

  #endif
  sgp_testAndInit();

  veml_test();

  dht.setup(DHT_PIN, DHTesp::DHT11);
  dht_testAndMeasure();

  
  setup_wifi();
  
  test_and_connexion_mqtt();

  delay(1000); // tempo

}
//--------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------
void loop() {

/*
 * si on est sur le projet miroir on évite de s'éteindre
 * 
 */

    delay(dht.getMinimumSamplingPeriod()); // on attend le temps necessaire pour avoir un echantillon chez le DHT
    connect_mqtt();
    client.loop(); 
    


      mqtt_publish("esp/Battery",(float)ESP.getVcc()/1000.0); // car en mV

      // partie traitement des informations SGP30
      if(! sgp_error)
      {
          sgp.IAQmeasure();
          delay(2000);
          sgp.IAQmeasureRaw();
          delay(2000);
          sgp.IAQmeasure();
          delay(2000);
          sgp.IAQmeasureRaw();
          delay(2000);
          
          #if(DEBUGING)
            Serial.println(" SGP values : Publication...");
          #endif
          mqtt_publish("esp/SGP30/CO2",sgp.eCO2);
          mqtt_publish("esp/SGP30/H2",sgp.rawH2);
          mqtt_publish("esp/SGP30/Ethanol",sgp.rawEthanol);
          #if(DEBUGING)
            Serial.println("Values Published !");
          #endif
      }
      else
      {
        if(sgp_error == 1)
        {
          //client.loop(); 
          #if(DEBUGING)
            Serial.println(" SGP Error : publication...");
          #endif
          client.publish("esp/Erreur","FAIL : SGP30 Not Found");
          #if(DEBUGING)
            Serial.println(" SGP Error : published !");
          #endif

        }

        else
        {
          #if(DEBUGING)
            Serial.println(" SGP Error : publication...");
          #endif
          client.publish("esp/Erreur","FAIL : IAQ not initialized");
          #if(DEBUGING)
            Serial.println(" SGP Error : published !");
          #endif
        }
      }

      if(! veml_error)
      {
          delay(0.5*1000);
          #if(DEBUGING) 
            Serial.println(" VEML Values : publication...");
          #endif
          mqtt_publish("esp/VEML/Lumiere",veml.readLux());
          #if(DEBUGING)
           Serial.println(" VEML Values : published !");
          #endif
      }
      else
      {
        client.loop();
        #if(DEBUGING)
          Serial.println(" VEML Error : publication...");
        #endif
        client.publish("esp/Erreur","FAIL : VEML Not Found");
        #if(DEBUGING)
          Serial.println(" VEML Error : published !");
        #endif
      }


      if (! dht_error)
      {
        // les valeurs ont été récuperés au moment du setup
        mqtt_publish("esp/DHT11/Temperature", temp);
        mqtt_publish("esp/DHT11/Humidite", hum);
        
      }
      else
      {
        #if DEBUGING == 1 
         Serial.println(dht.getStatusString());
        #endif

        //client.loop(); 
        #if(DEBUGING) 
          Serial.println(" DHT Error : publication...");
        #endif
        client.publish("esp/Erreur", dht.getStatusString());
        #if(DEBUGING) 
          Serial.println(" DHT Error : publication...");
        #endif
      }
      #if DEBUGING
       Serial.println("soft resetting sgp for sleeping");
      #endif
      sgp.softReset(); // on soft reset pour plonger le capteur dans le sleep mode
      // Serial.println("power saving veml");
      // veml.powerSaveEnable(true); // on disable pour plonger le capteur dans le sleep mode
      
      #if DEBUGING == 1
        Serial.println("disconnecting from the broker");
      #endif

      client.disconnect();

      #if DEBUGING == 1
        Serial.println("power off dht");
      #endif

      #if DEBUGING == 1
        Serial.print("sleeping for ");
        Serial.print(TIME_TO_SLEEP);
        Serial.println("mn");
        delay(100);
      #endif

      delay(250);
      ESP.deepSleep(TIME_TO_SLEEP*60*1000000);


     
}
//--------------------------------------------------------------------------------------------------------------------------------

