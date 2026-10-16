#ifndef WRITER_CLASS_H
#define WRITER_CLASS_H

#include <SD.h>
#include <queue>
#include <vector>
#include "Measure.hpp"

#define nVersion 1.2.0

class Writer
{
    private:
        File file;
        const char* filename;
        unsigned int next_id = 0;
        int CSPin; 

        // Read the next measurement ID from the last stored record
        unsigned int GetNextId();

        // Append a measurement (of type Measure) to a new line in the CSV file
        void WriteInNewLine(Measure& data);

        // Attempt to reconnect to the SD card if the connection is lost
        bool Reconnect();

    public:
        // Create a writer for the specified CSV file
        explicit Writer(const char* filename);

        // Initialize connection with the SD card using the specified Chip Select (CS) pin
        void EstablishConnection(const int CSpin);

        // NF 29/4/2025 modification to pass 
        void LogData(int ncapteur, const std::vector<double>& toute_mesure, String uidString);
        void LogString(std::queue<String> receiveQueue);

        // Safely close the connection with the SD card
        void Dispose();
};

#endif
