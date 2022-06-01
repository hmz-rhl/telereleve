
/* ESP8266 BROCHAGE

                __________________
          ADC0 | A0 o         o D0|  GPIO16   --> a connecter à RST pour DeeplSleep
NC on some  5V | VU o         o D1|  GPIO5    --> SLK
NC on some GND |  G o         o D2|  GPIO4    --> SDA
        GPIO10 | S3 o         o D3|  GPIO0
         GPIO9 | S2 o         o D4|  GPIO2    --> DHT signal
          MOSI | S1 o         o 3V|  3.3V
            CS | SC o         o G |  GND
          MISO | SO o         o D5|  GPIO14
          SCLK | SK o         o D6|  GPIO12   --> DHT +vcc allumage
           GND |  G o         o D7|  GPIO13
          3.3V | 3V o         o D8|  GPIO15   
            EN | EN o         o RX|  GPIO3
         Reset |RST o         o TX|  GPIO1
           GND |  G o         o G |  GND      
           Vin |VIN o         o 3V|  3.3V     --> interrupteur sur + batterie
               |       ____       |
               |______/====\______|



*/

#include <ESP8266Ping.h>
#include <ESP8266Ping.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h> // librairie pour configurer une liste de point d'accès (Router 1, router 2, etc.)
#include <PubSubClient.h> //Librairie pour la gestion Mqtt 

#include <Adafruit_SGP30.h>
#include <Adafruit_VEML7700.h>

#include <DHTesp.h>

#define TIME_TO_SLEEP           30 // en mn
#define DHT_PIN_VCC             12
#define DHT_PIN                 2
#define ESP32

#define ESP8266

#define DEBUGING                 1
#define MIROIR                   0

#if MIROIR == 1
  unsigned long tps=0;
#else
  float temp;
#endif

ADC_MODE(ADC_VCC); // pour renvoyer vcc sur ADC du coup on ne peut plus

// declaration des capteurs
DHTesp dht;
Adafruit_SGP30 sgp;
Adafruit_VEML7700 veml;

// variable d'erreur pour les capteurs
int8_t sgp_error = 1, veml_error = 1, dht_error = 1; 

#if MIROIR == 1

  const IPAddress remote_ip(192, 168, 137,67);//(192,168,137,161); // ip du broker a ping
  const IPAddress router_ip(192,168, 137,1); // adresse ip du ROUTEUR
  IPAddress subnet(255, 255, 255, 0);  //Subnet mask
  //IPAddress dns(8, 8, 8, 8);  //DNS
  //WIFI
  const char* ssid = "SFR_D400_EXT"; // "Pc Hamza";
  const char* password = "dmjq3fvp87csqq7zys79"; // "hamza123";
  const IPAddress staticIP(192, 168, 137, 250); //ESP demande cette IP
  //MQTT
  const char* mqtt_server = "192.168.137.67 ";// "192.168.1.67";//Adresse IP du Broker Mqtt
  const int mqttPort = 1883; //port utilisé par le Broker 

#else

  const IPAddress remote_ip(192, 168, 1,200);//(192,168,137,161); // ip du broker a ping
  const IPAddress router_ip(192,168, 1,1); // adresse ip du ROUTEUR
  IPAddress subnet(255, 255, 255, 0);  //Subnet mask
  //IPAddress dns(8, 8, 8, 8);  //DNS
  //WIFI
  const char* ssid = "SFR_D400"; // "Pc Hamza";
  const char* password = "dmjq3fvp87csqq7zys79"; // "hamza123";
  const IPAddress staticIP(192, 168, 1, 250); //ESP demande cette IP
  //MQTT
  const char* mqtt_server = "192.168.1.200 ";// "192.168.1.67";//Adresse IP du Broker Mqtt
  const int mqttPort = 1883; //port utilisé par le Broker 

#endif

// objet pouvant contenir plusieurs ssid
ESP8266WiFiMulti WiFiMulti;

WiFiClient espClient;

// declaration du client MQTT
PubSubClient client(espClient);


/*!
*
* @brief configure la connexion wifi de l'ESP
* 
* ssid et password sont définis en variable globale
*/

void setup_wifi()
{
  //connexion au wifi
  WiFi.config(staticIP, subnet, router_ip);
  WiFiMulti.addAP(ssid, password);
  #if  DEBUGING == 1
  
    Serial.print("\n\nconnection au wifi : ");
    Serial.print(ssid);
  
  #endif
  while ( WiFiMulti.run() != WL_CONNECTED ) {
    delay ( 500 );

    #if DEBUGING == 1
      Serial.print ( "." );
    #endif
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
* @brief 
*
*
*
*
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
  //client.subscribe("led");//souscription au topic led pour commander une led
}

/*!
*
* @brief configure la connexion au serveur mqtt.
* 
*   Utilise variable global description donc pas besoin de parametre, mais il faut declarer et définir mqtt_server et mqttPort.
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
  //client.setCallback(callback);//Déclaration de la fonction de souscription
  connect_mqtt();

}


/*!
* @brief 
*
*
*
*
*
*/
void sgp_test()
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
* @brief 
*
*
*
*
*
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
* @brief 
*
*
*
*
*
*/

void allumer_dht()
{
  pinMode(DHT_PIN_VCC, OUTPUT);
  digitalWrite(DHT_PIN_VCC, HIGH);
  delay(2000);
}

/*!
* @brief 
*
*
*
*
*
*/

void eteindre_dht()
{
  digitalWrite(DHT_PIN_VCC, LOW);
}

/*!
* @brief 
*
*
*
*
*
*/

void dht_test()
{
   if(dht.getStatus() == 0)
  {
    #if(DEBUGING)
      Serial.print("SUCCESS : DHT ");
      Serial.println(dht.getStatusString());
    #endif

    dht_error = 0;
  }
}




//Fonction pour publier un float sur un topic 










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
  sgp_test();

  veml_test();

  allumer_dht();
  dht.setup(DHT_PIN, DHTesp::DHT11);
  dht_test();

  //pinMode(4,INPUT); // port GPIO4 en mode Entrée
  
  setup_wifi();
  
  test_and_connexion_mqtt();

  delay(1000); // tempo

  
  
  //client.publish("test", "Hello from ESP8266");
}
//--------------------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------------
void loop() {

/*
 * si on est sur le projet miroir on évite de s'éteindre
 * 
 */

#if MIROIR == 1

    if(millis() - tps > 5)  
    {

      // si jamais le wifi se deconnecte, on reconnecte au wifi(utile que si on reste bloqué dans loop c'est a dire sans redemarré)
      if(WiFiMulti.run() != WL_CONNECTED) 
      {
        setup_wifi();
        test_and_connexion_mqtt();
      }

      if(sgp_error || veml_error || dht_error) ESP.restart();

      sgp.IAQmeasure();
      delay(2000);
      sgp.IAQmeasureRaw();
      delay(2000);
      sgp.IAQmeasure();
      delay(2000);
      sgp.IAQmeasureRaw();
      delay(2000);

      // Publication des valeurs sur les differents Topics MQTT
      mqtt_publish("esp/SGP30/CO2",sgp.eCO2);
      mqtt_publish("esp/SGP30/H2",sgp.rawH2);
      mqtt_publish("esp/SGP30/Ethanol",sgp.rawEthanol);
      
      mqtt_publish("esp/VEML/Lumiere",veml.readLux());
      mqtt_publish("esp/DHT11/Temperature", 20.5);// dht.getTemperature());
      
      mqtt_publish("esp/Battery",(float)ESP.getVcc()/1000.0); // car en mV
      
      tps = millis();
    }
#else
  
  
    delay(dht.getMinimumSamplingPeriod()); // on attend le temps necessaire pour avoir un echantillon
    connect_mqtt();
    client.loop(); 
    


    //On utilise pas un delay pour ne pas bloquer la réception des messages 
    //on envoit des donnes toutes les secondes DEBUG car après on passera au sleep mode
    //if (millis()-tps>10000)
    
      //tps=millis();
        
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
        temp = dht.getTemperature();
        while(isnan(temp)){
          #if DEBUGING == 1
            Serial.println("aghhh temp is nan !");
          #endif
          delay(100);
          temp = dht.getTemperature();

        }
        Serial.println(temp);
        #if DEBUGING == 1 
          Serial.println(" DHT Values : publication...");
        #endif
        mqtt_publish("esp/DHT11/Temperature", temp);
        temp = dht.getHumidity();
        mqtt_publish("esp/DHT11/Humidite", temp);
        #if DEBUGING == 1 
          Serial.println(" DHT Values : published !");
        #endif
        
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
      eteindre_dht();

      #if DEBUGING == 1
        Serial.print("sleeping for ");
        Serial.print(TIME_TO_SLEEP);
        Serial.println("mn");
        delay(100);
      #endif

      delay(250);
      ESP.deepSleep(TIME_TO_SLEEP*60*1000000);
#endif

     
}
//--------------------------------------------------------------------------------------------------------------------------------

