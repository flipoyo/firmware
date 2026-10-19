#include <Arduino.h>
#include <SD.h>
#include <queue>
#include "ArduinoLowPower.h"
#include <MKRWAN.h>

#include "LoRaWan_Molonari.hpp"
#include "LoRa_Molonari.hpp"
#include "Waiter.hpp"
#include "Reader.hpp"
#include "Time.hpp"
#include "Writer.hpp"

#include "main.hpp"


LoRaModem modem;

LoraWANCommunication loraWAN;
std::queue<String> sendingQueue;
Writer logger(Storage::RECORDS_FILE);

GeneralConfig res;

long lastLoraSend = 0;
long lastAttempt = 0;// mémorise la dernière tentative de réception (en millisecondes)

String appEui;
String devEui;
String appKey;
LoraCommunication lora(LoRaConfig::FREQUENCY_Hz, devEui, appEui, RoleType::MASTER);
bool modif = false;

Waiter waiter; //pour ne pas l'indenter dans le loop
unsigned long lastSDOffsetConfig = 0;
unsigned long lastSDOffset = 0;


// ----- Setup -----
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(SerialConfig::BAUD_RATE);
    unsigned long end_date = millis() + SerialConfig::TIMEOUT_MS;
    while (!Serial && millis() < end_date) {}

    DEBUG_LOG("\n=== Initialisation du Relais Molonari ===");

    DEBUG_LOG_NO_LN("Relay Firmware Version: ");
    DEBUG_LOG(Version::VERSION);


    // Lecture configuration CSV
    Reader reader;
    res=reader.lireConfigCSV(Storage::CONFIG_FILE, Pins::SD_CS);
    DEBUG_LOG("Configuration chargée.");

    // Initialisation LoRa communication
    lora.LoraUpdateAttributes(LoRaConfig::FREQUENCY_Hz, res.rel_config.appEui, res.rel_config.devEui, RoleType::MASTER);

    // Vérification SD
    if (!SD.begin(res.rel_config.CSPin)) {
        DEBUG_LOG("Erreur SD - arrêt système.");
        while (true) {}
    }

    InitialiseRTC();
    logger.EstablishConnection(Pins::SD_CS);

    DEBUG_LOG("Initialisation terminée !");
    pinMode(LED_BUILTIN, INPUT_PULLDOWN);
    digitalWrite(LED_BUILTIN, LOW);
}


// ----- Loop -----
void loop() {
    long currentTime = GetSecondsSinceMidnight();
    
    if (CalculateElapsedSeconds(currentTime, lastAttempt) >= res.int_config.lora_intervalle_secondes) {
        DEBUG_LOG("Réveil du relais pour vérification communication LoRa...");
        DEBUG_LOG("Fenêtre de communication LoRa atteinte, tentative de réception des paquets...");
        std::queue<String> receiveQueue;
        lora.startLoRa();

        // Réception des paquets via LoRa
        uint8_t deviceId = 0; // initialise avec packetNumber = 0 
        if (lora.handshake(deviceId)) {

            DEBUG_LOG("Handshake réussi. Réception des paquets...");
            int last = lora.receiveAllPackets(receiveQueue);
            DEBUG_LOG("Packets received when close session : " + String(last));
            lora.sendPacket(last, FIN, "FIN"); // Répond par un FIN de confirmation;

            // Met à jour le temps de la dernière tentative de réception
            lastAttempt = GetSecondsSinceMidnight();

            //envoie du csv
            if (modif==true) { // normalement modif est toujours false pour l'instant : la focntion de modif n'est pa sbien implémentée
                File config = SD.open(Storage::CONFIG_FILE, FILE_READ);
                config.seek(lastSDOffsetConfig);
                std::queue<memory_line> lines_config;

                while (config.available()) {
                    memory_line new_line = memory_line(config.readStringUntil('\n'), config.position());
                    lines_config.push(new_line);
                    
                        uint8_t lastPacket = lora.sendAllPacketsAndManageMemory(lines_config, lastSDOffsetConfig, config);
                        lora.closeSession(lastPacket);

                        modif = !(lastSDOffsetConfig == config.position());
                }
            }
            lora.stopLoRa();

            logger.LogString(receiveQueue); 
        } else {
            DEBUG_LOG("Handshake échoué, aucune donnée reçue.");
            lora.stopLoRa();

            // Met quand même à jour lastAttempt pour réessayer après l'intervalle de temps
            lastAttempt = GetSecondsSinceMidnight();
        }

        // Envoi via LoRaWAN si intervalle complet atteint

        File dataFile = SD.open(Storage::RECORDS_FILE, FILE_READ);
        DEBUG_LOG("File has been opened");

        if (!dataFile) {
            DEBUG_LOG("Impossible d'ouvrir le fichier de données pour LoRa");
            return;
        }

        if (loraWAN.begin(res.rel_config.appEui, res.rel_config.appKey)) {
            DEBUG_LOG("appKey : " + String(res.rel_config.appKey));
            DEBUG_LOG("temps avant communication : " + String(CalculateSleepTimeUntilNextCommunication(lastAttempt, res.int_config.lora_intervalle_secondes)));


            while (CalculateSleepTimeUntilNextCommunication(lastAttempt, res.int_config.lora_intervalle_secondes) > LoRaConfig::MIN_SLEEP_MS && dataFile.available()) { //racourcir de Radio::MIN_SLEEP_MS à 10000 pour les besoins de la démo
                //at this point, lastSDOffset must point to the first memory address of the first line to be sent
                DEBUG_LOG("entrée dans le while d'envoi LoRaWAN");
                std::queue<memory_line> linesToSend;
                dataFile.seek(lastSDOffset);
                while (dataFile.available()) {
                    DEBUG_LOG("lecture d'une nouvelle ligne dans le fichier");
                    String flush = dataFile.readStringUntil('\n');
                    DEBUG_LOG("data file until espace : " + flush);
                    memory_line new_line = memory_line(flush, dataFile.position());
                    DEBUG_LOG(String("new line: ") + new_line.flush);
                    linesToSend.push(new_line);

                // Si la ligne est vide aka plus rien à envoyer
                    if (linesToSend.front().flush.length() == 0) {
                        break;
                    }
                }
                if (loraWAN.sendAllPacketsAndManageMemoryWAN(linesToSend, lastSDOffset, dataFile)) {
                    DEBUG_LOG("Tous les paquets ont été envoyés !");
                } else {
                    DEBUG_LOG("Certains paquets n’ont pas pu être envoyés, ils seront réessayés.");
                }
            }    

            dataFile.close();

        } else {
            DEBUG_LOG("Connexion LoRaWAN impossible, report de l’envoi.");
        }
        lastLoraSend = currentTime;
    
        DEBUG_LOG("about to loop on modem.available()");
        // reception csv et modification
        while (modem.available()) {
            loraWAN.receiveConfig(Storage::CONFIG_FILE, modif);
            modif = true;
        }

    DEBUG_LOG("Relais en veille jusqu’à la prochaine fenêtre de communication...");
    long time_to_sleep = CalculateSleepTimeUntilNextCommunication(lastAttempt, res.int_config.lora_intervalle_secondes); 
    DEBUG_LOG("sleeping until next communication " + String (time_to_sleep) + "ms");
    LowPower.idle(uint32_t(time_to_sleep));
    }
}