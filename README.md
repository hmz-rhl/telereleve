# telereleve

Ce projet à pour but de développer un système autonome de mesures de données environnementales pour l'audit énergétique, à l'aide d'un Raspberry et d'un ESP8266 ainsi que 3 capteur
SGP30, VEML7700, DHT11

# Installation

## Raspberry Pi
### configuration principale
il faut installer Debian sur une raspberry, ainsi que mosquitto pour la communication MQTT
Il faut aussi configurer une IP fixe pour cette raspberry qui sera le broker de nos mesures.
fichier cmdline.txt:
```
ip=X.X.X.X
```
```
sudo apt-get update 
sudo apt-get install mosquitto mosquitto-clients 
```
```
sudo nano /etc/mosquitto/conf.d/config.conf
```
![image](https://user-images.githubusercontent.com/77698738/174766929-67825cb5-fcfd-4673-9470-9b3136dc0ba6.png)

### NodeRED
installer nodeRed qui fera office de  WEB interface, et qui gérera les données qui viennes par MQTT.
```
sudo apt-get install nodered
sudo systemctl enable nodered
```
taper l'adresse ip de la raspberry suivi  de ":1880/" pour acceder a nodered.
une fois dans node roed vousallez importez un flow, il est a la racine de ce git :  	[flow_nodered_telereleve.json](https://github.com/hmz-rhl/telereleve/blob/master/flow_nodered_telereleve.json)

enfin vous aller configurer le noeud suivant pour remplacer par l'adresse ip de l'esp :
![image](https://user-images.githubusercontent.com/77698738/174768503-52f1461a-2d32-499a-bccd-b8bbc4575422.png)


vous pouver desormais ouvrir l'interface web : "http://IP_Rasp:1880/ui"

## ESP8266

### Hardware configuration
![telereleve_bb](https://user-images.githubusercontent.com/77698738/174764630-adb36bbe-234c-4ff6-8448-65f6db73b855.jpg)

### PlatfromIO
Ouvrer ce prjet à l'aide de platformIO,
il faut modifier le code pour donner le bon setup wifi, ainsi que l'adress IP du routeur ainsi que celle de la Raspberry, enfin il faudra la aussi donner une IP pour l'ESP,
lisez les commentaires du fichier main.c pour etres guider

Vous etes aussi libre de choisir les tempo de mesures en changeant le TIME_TO_SLEEP (en minutes)
vous pouver desormais build et upload le firmware(Bouton rst et flash en meme temps puis lacher rst tout en maintenant flash pour commencer l'upload, une fois l'upload lancé vous pouvez relaché)


## Erreur a éviter
le dht est énergivore, et il peut vite être mécontent si la tension n'est pas suffisante, a verifier donc !

