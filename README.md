# telereleve

Ce projet à pour but de développer un système autonome de mesures de données environnementales pour l'audit énergétique, à l'aide d'un Raspberry et d'un ESP8266 ainsi que 3 capteur
SGP30, VEML7700, DHT11

# Installation

## Raspberry Pi
il faut installer Debian sur une raspberry, ainsi que mosquitto pour la communication MQTT
Il faut aussi configurer une IP fixe pour cette raspberry qui sera le broker de nos mesures.

## ESP8266

### Hardware configuration
![telereleve_bb](https://user-images.githubusercontent.com/77698738/174764167-1ec6881c-f6f6-43f3-b67f-9d256fca1cca.png)

### PlatfromIO
Ouvrer ce prjet à l'aide de platformIO,
il faut modifier le code pour donner le bon setup wifi, ainsi que l'adress IP du routeur ainsi que celle de la Raspberry, enfin il faudra la aussi donner une IP pour l'ESP,
lisez les commentaires du fichier main.c pour etres guider

Vous etes aussi libre de choisir les tempo de mesures en changeant le TIME_TO_SLEEP (en minutes)
vous pouver desormais build et upload le firmware



