// LoRaWan_Molonari.cpp
// This file defines the LoraWANCommunication class for handling LoRaWAN communication.
#include <pb.h>
#include <pb_common.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <MKRWAN.h>
#include <RTClib.h>
#include "encode.h"

#include "LoRaWan_Molonari.hpp"
#include <Reader.hpp>

#define LORA_DEBUG
#ifdef LORA_DEBUG
#define LORA_LOG(msg) Serial.print(msg);
#define LORA_LOG_HEX(msg) Serial.print(msg, HEX);
#define LORA_LOG_LN(msg) Serial.println(msg);
#else
#define LORA_LOG(msg)
#define LORA_LOG_HEX(msg)
#define LORA_LOG_LN(msg)
#endif

LoraWANCommunication::LoraWANCommunication() {}

bool LoraWANCommunication::begin(const String& appEui, const String& appKey) {
    LORA_LOG_LN("appKey : " + appKey);
    if (!modem.begin(EU868)) {
        LORA_LOG_LN("Échec initialisation module LoRaWAN");
        return false;
    }

    int connected = modem.joinOTAA(appEui, appKey);
    LORA_LOG_LN("Le statut de la connexion est celui-ci :  " + String(connected));
    if (!connected) {
        LORA_LOG_LN("Échec de la connexion LoRaWAN");
        return false;
    }

    modem.minPollInterval(60);
    modem.setADR(true);
    return true;
}

// Converts "DD/MM/YYYY" and "HH:MM:SS" into Unix seconds.
// Returns 0 if either string cannot be fully parsed.
static int32_t convertDateAndTimeToUnixTime(const String &dateString, const String &timeString) {
    int day = 0, month = 0, year = 0;
    int hour = 0, minute = 0, second = 0;
 
    int parsedDateFieldCount = sscanf(dateString.c_str(), "%d/%d/%d", &day, &month, &year);
    int parsedTimeFieldCount = sscanf(timeString.c_str(), "%d:%d:%d", &hour, &minute, &second);
 
    // Checks if sscanf parsed all 3 values for each field
    if (parsedDateFieldCount != 3 || parsedTimeFieldCount != 3) {
        return 0;
    }
 
    DateTime dateTime(year, month, day, hour, minute, second);
    return static_cast<int32_t>(dateTime.unixtime());
}

bool encode(uint8_t *outBuffer, size_t outSize, size_t &messageLength, String inputLine) {
    SensorData msg = SensorData_init_zero;

    // Exemple :
    // "ua4aef7; 27 ; 10/11/2025 ; 16:01:25 ; relay_receive_date; relay_receive_time; 1299.00; 1289.00; 1978.00; 1868.00; 2586.00; 2594.00;"
    inputLine.trim();

    String dateStr = "";
    String timeStr = "";
    String relayReceptionDateStr = "";
    String relayReceptionTimeStr = "";

    // --- Découpage de la ligne ---
    int fieldIndex = 0;
    int startIndex = 0;

    while (true) {
        int sepIndex = inputLine.indexOf(';', startIndex);
        if (sepIndex == -1) break;
        String token = inputLine.substring(startIndex, sepIndex);
        token.trim();

        switch (fieldIndex) {
            case 0: // Identifiant (UI)
                strncpy(msg.UI, token.c_str(), sizeof(msg.UI));
                msg.UI[sizeof(msg.UI) - 1] = '\0';
                break;

            case 1: // Numéro de mesure
                break;

            case 2: // Date
                dateStr = token;
                break;

            case 3: // Heure
                timeStr = token;
                break;
            case 4:
                relayReceptionDateStr = token;
                break;
            case 5:
                relayReceptionTimeStr = token;
                break;
            default: // Les mesures
                msg.measurements[msg.measurements_count] = static_cast<uint32_t>(token.toInt());
                msg.measurements_count = (msg.measurements_count < 9) ? msg.measurements_count + 1 : msg.measurements_count;
                break;
        }

        fieldIndex++;
        startIndex = sepIndex + 1;
    }

    msg.time = convertDateAndTimeToUnixTime(dateStr, timeStr);
    msg.relay_receive_time = convertDateAndTimeToUnixTime(relayReceptionDateStr, relayReceptionTimeStr);

    // --- Encodage Protobuf ---
    pb_ostream_t stream = pb_ostream_from_buffer(outBuffer, outSize);
    if (!pb_encode(&stream, SensorData_fields, &msg)) {
        LORA_LOG_LN("Protobuf encoding error!");
        return false;
    }

    messageLength = stream.bytes_written;
    LORA_LOG("Binary payload (length ");
    LORA_LOG(messageLength);
    LORA_LOG_LN(" bytes) ready to send!");
    return true;
}



/*
bool LoraWANCommunication::sendQueue(std::queue<String>& sendingQueue) {
    while (!sendingQueue.empty()) {
        String payload = sendingQueue.front();
        int err = 0, retries = 0;
        do {
            modem.beginPacket();
            modem.print(payload);
            err = modem.endPacket(true);
            if (err <= 0) {
                LORA_LOG_LN("payload : " + payload);
                LORA_LOG_LN("Erreur d’envoi, nouvelle tentative...");
                delay(10000);
            }
        } while (err <= 0 && ++retries < 6);

        if (err > 0) {
            LORA_LOG_LN("Donnée envoyée : " + payload);
            sendingQueue.pop();
        } else {
            LORA_LOG_LN("Échec après plusieurs tentatives, abandon du paquet.");
            return false;
        }
        delay(5000);
    }
    return true;
}*/



bool LoraWANCommunication::sendAllPacketsAndManageMemoryWAN(std::queue<memory_line>& sendQueue, unsigned long& SDOffset, File& dataFile) {
    // handles packet sending and acknowledgement verificaiton, SDOffset updating, and ensures dataFile.position() is at the right place (terminal SDOffset).

    while (!sendQueue.empty()) {
        uint8_t buffer[SensorData_size];
        size_t messageLength = 0;
        
        memory_line packet = sendQueue.front();
        LORA_LOG("Sending packet with data: " + packet.flush + "\n");

        if (!encode(buffer, sizeof(buffer), messageLength, packet.flush)) {
            LORA_LOG_LN("Error, payload pas encodé at all (du tout)");
            delay(10000);
            return false;
        }

        int send_retries = 0;
        int err = 0;
        while(send_retries < 2 && err <= 0){ //remettre send_retries à 10 après test
            modem.beginPacket();


            modem.write(buffer, messageLength);

            err = modem.endPacket(true);
            if (err <= 0) {
                LORA_LOG_LN("échec d'envoi de : " + packet.flush);
                LORA_LOG_LN("Erreur d’envoi, nouvelle tentative...");
                delay(1000);//remettre à 10 000 après les tetst
            }
            send_retries++;
        }
    
        if (err > 0) {
            LORA_LOG_LN("Donnée envoyée : " + packet.flush);
            sendQueue.pop();
            SDOffset = packet.memory_successor;

        } else {
            LORA_LOG_LN("Échec après plusieurs tentatives, abandon du paquet.");
            dataFile.seek(SDOffset);
            return false;
        }
        delay(5000);

    }
    return true;
}


bool LoraWANCommunication::receiveConfig(const char* configFilePath, bool modif) {
    String rcv = modem.readString();
    LORA_LOG("↓ Message reçu en downlink : ");
    LORA_LOG_LN(rcv);

    const char* tmpPath = "tmp_conf.csv";
    File tmp = SD.open(tmpPath, FILE_WRITE | O_TRUNC);
    tmp.println(rcv);
    tmp.close();
    
    File newF = SD.open(configFilePath, FILE_WRITE | O_TRUNC);
    if (!newF) {
        LORA_LOG("Impossible de créer ");
        LORA_LOG_LN(configFilePath);
        return false;
    }
    File tmpRead = SD.open(tmpPath, FILE_READ);
    if (!tmpRead) {
        LORA_LOG_LN("Impossible d'ouvrir tmp pour lecture");
        newF.close();
        return false;
    }
    while (tmpRead.available()) {
        newF.write(tmpRead.read());
    }
    tmpRead.close();
    newF.close();
    SD.remove(tmpPath);
    return true;

}
