#include <SPI.h>                                        // Librairie de communication SPI
#include <SD.h>                                         // Librairie pour la carte SD
#include <Adafruit_BME280.h>                            // Librairie pour le capteur BME280 d'Adafruit
#include "RTClib.h"                                     // Librairie pour l'horloge RTC
#include "config.h"                                     // Fichier de configuration (non fourni dans le code)

Adafruit_BME280 bme;                                    // Initialisation du capteur BME280
const int cardSelect = 15;                              // Sélection du numéro de broche pour la carte SD
RTC_DS3231 rtc;                                         // Initialisation de l'objet RTC                  

// Constantes à initialiser
#define adrI2CduBME280                0x76          // Adresse I2C du capteur BME280
#define pressionMer      1016.95                        // Pression atmosphérique au niveau de la mer en HPa à 1013.25, qui dépend de l'altitude où l'on se situe
#define delaiAffichage    1500                          // Temps de rafraîchissement de l'affichage de nouvelles données (en ms)
unsigned long myTime;
int val[100];
int valeurcapt = 0;                                     //initialisation de l'état initial du capteur

int delai1 = 0 ;                                        // Délai pour la première exécution
int delai2 = 0;                                         // Délai pour la deuxième exécution

// Initialisation des feeds pour Adafruit IO
AdafruitIO_Feed *Temperature = io.feed("Temperature");
AdafruitIO_Feed *Humidite = io.feed("Humidite");
AdafruitIO_Feed *Altitude = io.feed("Altitude");
AdafruitIO_Feed *Pression = io.feed("Pression");
AdafruitIO_Feed *Iefficace = io.feed("Intensite efficace");

// Constantes à créer
float Vefficace = 0;
float tensionSortieConditionneur = 0;
float Ieff = 0;
float max_v = 0;
float VmaxD = 0;
float VeffD = 0;
float Imax = 0;

//initialisations nécessaires au programme
void setup() {
  Serial.begin(9600);                                   // Initialisation de la communication série

  #ifndef ESP8266
  while (!Serial);                                      // Attente de la connexion du port série. Nécessaire pour les cartes avec USB.
  #endif

  if (! rtc.begin()) {
    Serial.println("Pas de RTC détecté");               // Message d'erreur si capteur RTC non détecté
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {                  
    Serial.println("RTC n'est plus alimenté,  !");        //Vérifie si l'horloge RTC a perdu son alimentation ; Si l'alimentation a été perdue, le programme affiche le message d'erreur                                                       
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));       //Configuration de l'heure de l'horloge RTC.
  }
  while(!Serial);
  Serial.println("Programme de test du BME280");
  Serial.println("===========================");
  Serial.println();                                       //Attend que la communication série soit établie avant de poursuivre l'exécution du programme. 

  // Initialisation du BME280
  Serial.print(F("Initialisation du BME280, à l'adresse [0x")); //Ajoute "[" au début de la ligne. 
  Serial.print(adrI2CduBME280, HEX);                      //Affiche la valeur de la variable adrI2CduBME280 en format hexadécimal 
  Serial.println(F("]"));                                 //Ajoute "]" à la fin de la ligne.
  
  if(!bme.begin(adrI2CduBME280)) {                        //Initialisation à l'adresse I2C spécifiée 
    Serial.println(F("--> ÉCHEC…"));                      //Si l'initialisation échoue, ce message est affiché sur le port série
    while(1);                             
  } else {
    Serial.println(F("--> RÉUSSIE !"));                   //Si l'initialisation est réussie, ce message est affiché sur le port série pour indiquer que l'initialisation du capteur BME280 s'est bien déroulée.

  }
  Serial.println();

  while (!Serial) {                                       //Suspend l'exécution du programme jusqu'à ce que la connexion du port série soit établie.
  }

  //Initialisation de la Carte SD
  Serial.print("Initializing SD card...");                //Affiche le message "Initializing SD card..." sur le port série pour indiquer que l'initialisation de la carte SD est en cours.

  if (!SD.begin(cardSelect)) {                            //Tente d'initialiser la carte SD en utilisant le numéro de broche cardSelect
    Serial.println("Carte non détectée");                 //Si l'initialisation de la carte SD échoue, ce message est affiché sur le port série

    while (1);
  }
  Serial.println("Carte initialisée correctement.");      //Si l'initialisation de la carte SD réussit, ce message est affiché sur le port série

  while(! Serial);                                        //Suspend l'exécution du programme jusqu'à ce que la connexion du port série soit établie.

  Serial.print("Connexion à Adafruit IO");              //Affiche le message sur le port série pour indiquer que le programme est en train de se connecter à Adafruit IO. 
  io.connect();                                         //Appelle la fonction connect() de l'objet io, qui permet d'établir une connexion à Adafruit IO.

  while(io.status() < AIO_CONNECTED) {                  //La constante AIO_CONNECTED est utilisée pour représenter l'état de connexion établie.
    Serial.print(".");
    delay(1000);                                        //Introduit un délai de 1000 millisecondes  entre chaque itération de la boucle. Cela permet de ne pas surcharger la tentative de connexion et de laisser un peu de temps pour que la connexion soit établie.
  }

pinMode(A0, INPUT);                                     //Configure la broche A0 en mode d'entrée, ce qui signifie que la broche sera utilisée pour lire des valeurs analogiques
}

//Cette fonction est appelée en boucle indéfiniment après l'exécution de la fonction setup()
void loop() {
  float valanalog = analogRead(A0);   //Lecture la valeur analogique de la broche A0
  float tension = valanalog /1023;    //Convertit la valeur analogique lue en une tension en divisant par 1023.
  Ieff = tension*15;                  //Calcule une valeur d'intensité efficace

//Création d'une chaine de caractères pour trier les données relevées
  DateTime now = rtc.now();           //L'objet DateTime stocke la date et l'heure actuelles.
  String chaineCarac = "";             //Cette ligne initialise une variable de type chaîne de caractères
  chaineCarac += String(now.unixtime());//Cette ligne ajoute à la chaîne chaineCarac la représentation sous forme de chaîne de caractères du temps UNIX actuel
  chaineCarac += ";";

  chaineCarac += String(bme.readTemperature());//Ajoute à la chaîne chaineCarac la valeur de la température mesurée par le capteur BME280 en °C.
  chaineCarac += ";";                          //Ajoute un point-virgule à la chaîne chaineCarac comme séparateur.

  chaineCarac += String(bme.readHumidity());   //Ajoute à la chaîne chaineCarac la valeur de l'humidité mesurée par le capteur BME280 en %.
  chaineCarac += ";";

  chaineCarac += String(bme.readPressure() / 100.0F);//Ajoute à la chaîne chaineCarac la valeur de la pression atmosphérique mesurée par le capteur BME280, en Pa (division par 100). 
  chaineCarac += ";";

  chaineCarac += String(bme.readAltitude(pressionMer));//Ajoute à la chaîne chaineCarac la valeur de l'altitude calculée à partir de la pression atmosphérique mesurée par le capteur BME280, en m. 
  chaineCarac+=";";

  chaineCarac += Ieff;                                 //Ajoute à la chaîne chaineCarac la valeur de l'intensité efficace, en A.
  chaineCarac += ";";

  if(millis()-delai2 >= 2000){                         //Vérifie si le temps écoulé depuis la dernière opération d'écriture dans le fichier est supérieur ou égal à 2000 millisecondes. Cela permet d'effectuer l'enregistrement toutes les 2 secondes.
    File FichierSD = SD.open("datalogGroupe13.txt", FILE_WRITE); //Ouvre le fichier sur la carte SD en mode écriture.

    if (FichierSD) {
      FichierSD.println(chaineCarac);                            //Si le fichier est ouvert avec succès, la chaîne de caractères chaineCarac, qui contient les données, est écrite dans le fichier, suivie d'un saut de ligne.
      FichierSD.close();                                         //Fermeture du fichier après avoir écrit les données.
      Serial.println(chaineCarac);                               //La chaîne de caractères est également imprimée sur le port série  pour le suivi ou le débogage.
    }
    //Si le fichier ne s'est pas ouvert
    else {
      Serial.println("Erreur d'ouverture pour datalog.txt");              //Un message d'erreur est imprimé sur le port série pour indiquer qu'il y a eu un problème lors de l'ouverture du fichier.
    }
    delai2 = millis();                                                    // Enregistre le temps actuel (en millisecondes) dans la variable delai2, 
  }  

  if(millis()-delai1 >= 10000){    //Vérifie si le temps écoulé depuis la dernière opération d'envoi de données à Adafruit IO (delai1) est supérieur ou égal à 10000 millisecondes. Cela permet d'effectuer l'envoi toutes les 10 secondes.
      io.run();                    //Cela permet de traiter les opérations réseau et de maintenir la connexion à la plateforme.

      Temperature->save(bme.readTemperature());      //Enregistre la lecture actuelle de la température provenant du capteur BME280 dans le feed 'Temperature' défini sur Adafruit IO.
      Humidite->save(bme.readHumidity());            //Enregistre la lecture actuelle de la température provenant du capteur BME280 dans le feed 'Humidité' défini sur Adafruit IO.
      Altitude->save(bme.readAltitude(pressionMer));//Enregistre la lecture actuelle de la température provenant du capteur BME280 dans le feed 'Altitude' défini sur Adafruit IO.
      Pression->save(bme.readPressure() / 100.0F);  //Enregistre la lecture actuelle de la température provenant du capteur BME280 dans le feed 'Pression' défini sur Adafruit IO.
      Iefficace ->save(Ieff);                       //Enregistre la lecture actuelle de la température provenant du capteur BME280 dans le feed 'Intensité efficace' défini sur Adafruit IO.
      delai1 = millis();                            //Enregistre le temps actuel (en millisecondes) dans la variable delai1, marquant ainsi le moment de la dernière opération d'envoi de données à Adafruit IO.
  }
}