#include "ascii_decoder.h"
#include <cstring> // For strncmp, strchr, strncpy, strlen
#include <cstdio>  // For sscanf
#include <ctime>   // For struct tm, mktime
#include <cstdlib> // For strtol (if needed for checksum)

// Define a reasonable maximum line length expected in P1 ASCII frames
#define MAX_LINE_LENGTH 128 

AsciiDecoder::AsciiDecoder() {
    // Constructor implementation (currently empty)
}

/**
 * @brief Parses a standard OBIS data line and adds it to P1Data.
 * 
 * Assumes the line is a valid OBIS data line containing an OBIS code and value,
 * e.g., "1-0:1.8.0(00013139.107*kWh)". It adds the entire line as a string.
 * 
 * @param line The null-terminated string containing the OBIS data line.
 * @param p1data The P1Data object to add the OBIS string to.
 * @return true if the line was added successfully, false otherwise (e.g., buffer full).
 */
bool AsciiDecoder::parseObisLine(const char* line, P1Data& p1data) {
    // Basic check: does it contain an opening parenthesis?
    if (strchr(line, '(') == nullptr) {
        return false; // Not a standard OBIS value line
    }
    // Add the entire line as a raw OBIS string using the P1Data method
    return p1data.addObisString(line);
}

/**
 * @brief Decodes a buffer containing a P1 ASCII telegram.
 * 
 * Iterates through the frame data line by line (delimited by \r\n),
 * identifies header, timestamp, data, and checksum lines, and populates
 * the P1Data object.
 * 
 * @param frame An IFrameData object containing the raw ASCII frame data.
 * @param p1data A P1Data object to populate with the decoded data.
 * @return true if the buffer was successfully decoded (at least partially), false otherwise.
 */
bool AsciiDecoder::decodeBuffer(const IFrameData& frame, P1Data& p1data) {
    bool dataFound = false;
    char currentLine[MAX_LINE_LENGTH];
    size_t linePos = 0;
    size_t frameSize = frame.getFrameSize();

    for (size_t i = 0; i < frameSize; ++i) {
        char currentChar = frame.getFrameByte(i);

        // Check for end-of-line markers (\r or \n)
        if (currentChar == '\r' || currentChar == '\n') {
            if (linePos > 0) { // Process line if it's not empty
                currentLine[linePos] = '\0'; // Null-terminate the extracted line

                // --- Process the complete line ---
                if (currentLine[0] == '/') {
                    // Header line: Contains the Meter Identification
                    // Skip the leading '/'
                    size_t idLen = strlen(currentLine + 1);
                    // Use the dedicated setter in P1Data
                    p1data.setDeviceId(currentLine + 1); 
                    dataFound = true; // Found at least the device ID

                } else if (currentLine[0] == '!') {
                    // Checksum line: Marks the end of the data telegram
                    // TODO: Optionally implement checksum verification here
                    // Example: uint16_t receivedCrc = (uint16_t)strtol(currentLine + 1, nullptr, 16);
                    // bool crcOk = verifyChecksum(...);
                    break; // Stop processing after the checksum line

                } else if (strchr(currentLine, '(') != nullptr && strchr(currentLine, ':') != nullptr) {
                    // Assume standard OBIS data line (contains '(' and ':')
                    if (parseObisLine(currentLine, p1data)) {
                        dataFound = true;
                    } else {
                        // Optional: Log error if adding fails (buffer full)
                    }
                }
                // --- End of line processing ---

                // Reset line buffer position for the next line
                linePos = 0; 
            }
            // Skip potential consecutive \r or \n characters
            while (i + 1 < frameSize && (frame.getFrameByte(i + 1) == '\r' || frame.getFrameByte(i + 1) == '\n')) {
                i++;
            }
        } else if (linePos < MAX_LINE_LENGTH - 1) {
            // Add character to the current line buffer
            currentLine[linePos++] = currentChar;
        } else {
            // Error: Line exceeds MAX_LINE_LENGTH. Discard current line progress.
            // Skip characters until the next end-of-line sequence is found.
            linePos = 0; // Reset buffer position
            while (i + 1 < frameSize && frame.getFrameByte(i + 1) != '\r' && frame.getFrameByte(i + 1) != '\n') {
                i++;
            }
            // Optional: Log an error about the long line
        }
    }

    // Process the very last line if the frame doesn't end with \r\n
    // (unlikely but possible)
    if (linePos > 0) {
         currentLine[linePos] = '\0';
         // Check if it looks like an OBIS line
         if (strchr(currentLine, '(') != nullptr && strchr(currentLine, ':') != nullptr) {
             if (parseObisLine(currentLine, p1data)) {
                 dataFound = true;
             }
         }
    }

    return dataFound; // Return true if any part of the frame was successfully processed
}

// TODO: Implement checksum verification if needed
// bool AsciiDecoder::verifyChecksum(const IFrameData& frame) { 
//     // 1. Find start ('/') and end ('!') markers.
//     // 2. Calculate CRC16 (often CRC16-IBM/ANSI) over the bytes between / and ! (inclusive of /).
//     // 3. Extract the received CRC value after '!'.
//     // 4. Compare calculated CRC with received CRC.
//     return false; 
// }
