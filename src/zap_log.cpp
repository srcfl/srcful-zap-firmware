#include "zap_log.h"
#include <Arduino.h> // Required for Serial
#include <stdarg.h>  // Explicitly include for va_list, va_start, va_arg, va_end

static log_level_t current_log_level = ZLOG_LEVEL_INFO; // Default log level

void set_log_level(log_level_t level) {
    current_log_level = level;
}

log_level_t get_log_level() {
    return current_log_level;
}

static const char* level_to_string(log_level_t level) {
    switch (level) {
        case ZLOG_LEVEL_ERROR: return "E";
        case ZLOG_LEVEL_WARN:  return "W";
        case ZLOG_LEVEL_INFO:  return "I";
        case ZLOG_LEVEL_DEBUG: return "D";
        case ZLOG_LEVEL_VERBOSE: return "V";
        default: return "";
    }
}

void zap_log_message(log_level_t level, const char *tag, const char *format, ...) {
    if (level == ZLOG_LEVEL_NONE || level > current_log_level) {
        return;
    }

    // Only attempt to log if Serial is available or if it's an error message.
    // This check relies on the Arduino Serial object's boolean conversion.
    // This basically checks if Serial is initialized in the main setup it will not react on plugging in or out
    if (!Serial && level > ZLOG_LEVEL_ERROR) {
        digitalWrite(3, LOW);
        return;
    }
    digitalWrite(3, HIGH);

    char log_buffer[256]; // Buffer for the formatted log message
    char *p_log_buffer = log_buffer;
    char * const log_buffer_end = log_buffer + sizeof(log_buffer) - 1; // Leave space for null terminator

    // Add log level and tag
    const char* level_str = level_to_string(level);
    while (*level_str && p_log_buffer < log_buffer_end) {
        *p_log_buffer++ = *level_str++;
    }
    if (p_log_buffer < log_buffer_end) *p_log_buffer++ = '_';
    
    const char* p_tag = tag;
    while (*p_tag && (p_log_buffer < log_buffer_end) && (p_tag - tag < 30)) { // Limit tag length
        *p_log_buffer++ = *p_tag++;
    }
    if (p_log_buffer < log_buffer_end) *p_log_buffer++ = ':';
    if (p_log_buffer < log_buffer_end) *p_log_buffer++ = ' ';

    // Format the rest of the message
    va_list args;
    va_start(args, format);

    for (const char* c = format; *c != '\0' && p_log_buffer < log_buffer_end; c++) {
        if (*c != '%') {
            *p_log_buffer++ = *c;
            continue;
        }
        
        c++; // Skip the %
        if (*c == '\0') { // Handle format string ending with %
            if (p_log_buffer < log_buffer_end) *p_log_buffer++ = '%';
            break;
        }
        
        switch (*c) {
            case 'i': {
                int val = va_arg(args, int);
                char temp_num_buffer[12]; // Buffer for number conversion
                char *p_num = &temp_num_buffer[sizeof(temp_num_buffer) - 1];
                *p_num = '\0'; // Null terminate
                if (val == 0) *--p_num = '0';
                else {
                    do {
                        if (p_num == temp_num_buffer) break; // Prevent buffer underflow (should not happen for int)
                        *--p_num = '0' + (val % 10);
                        val /= 10;
                    } while (val > 0);
                }
                while (*p_num && p_log_buffer < log_buffer_end) *p_log_buffer++ = *p_num++;
                break;
            }
            case 'd': {
                int val = va_arg(args, int);
                char temp_num_buffer[12]; // Buffer for number conversion
                char *p_num = &temp_num_buffer[sizeof(temp_num_buffer) - 1];
                *p_num = '\0'; // Null terminate
                bool negative = (val < 0);
                if (negative) val = -val;
                if (val == 0) *--p_num = '0';
                else {
                    do {
                        if (p_num == temp_num_buffer) break; // Prevent buffer underflow (should not happen for int)
                        *--p_num = '0' + (val % 10);
                        val /= 10;
                    } while (val > 0);
                }
                if (negative && p_num != temp_num_buffer) *--p_num = '-';
                
                while (*p_num && p_log_buffer < log_buffer_end) *p_log_buffer++ = *p_num++;
                break;
            }
            case 's': {
                char* str = va_arg(args, char*);
                if (str == nullptr) str = (char*)"(null)";
                while (*str && p_log_buffer < log_buffer_end) *p_log_buffer++ = *str++;
                break;
            }
            case 'f': { // Simplified float/double to string (e.g., %.2f)
                double val = va_arg(args, double);
                // Print integer part
                bool negative = (val < 0.0);
                if (negative) val = -val;

                int int_part = (int)val;
                char temp_num_buffer[12];
                char *p_num = &temp_num_buffer[sizeof(temp_num_buffer) - 1];
                *p_num = '\0';
                if (int_part == 0) *--p_num = '0';
                else {
                    int temp_int_part = int_part;
                    do {
                        if (p_num == temp_num_buffer) break;
                        *--p_num = '0' + (temp_int_part % 10);
                        temp_int_part /= 10;
                    } while (temp_int_part > 0);
                }
                if (negative && p_num != temp_num_buffer) *--p_num = '-';
                while (*p_num && p_log_buffer < log_buffer_end) *p_log_buffer++ = *p_num++;

                if (p_log_buffer < log_buffer_end) *p_log_buffer++ = '.'; // Decimal point

                // Print fractional part (2 decimal places)
                double frac_part = val - int_part;
                int frac_as_int = (int)(frac_part * 100.0 + 0.5); // Multiply by 100 for 2 decimal places and add 0.5 for rounding
                
                char temp_frac_buffer[3]; // "xx" + null
                temp_frac_buffer[2] = '\0';
                if (frac_as_int >= 10) {
                    temp_frac_buffer[1] = '0' + (frac_as_int % 10);
                    frac_as_int /= 10;
                    temp_frac_buffer[0] = '0' + (frac_as_int % 10);
                } else { // 0-9
                    temp_frac_buffer[1] = '0' + frac_as_int;
                    temp_frac_buffer[0] = '0';
                }
                char* p_frac = temp_frac_buffer;
                while (*p_frac && p_log_buffer < log_buffer_end) *p_log_buffer++ = *p_frac++;
                break;
            }
            case 'c': {
                char char_val = (char)va_arg(args, int); // char promotes to int in va_arg
                if (p_log_buffer < log_buffer_end) *p_log_buffer++ = char_val;
                break;
            }
            case '%': {
                if (p_log_buffer < log_buffer_end) *p_log_buffer++ = '%';
                break;
            }
            default: // Unknown format specifier
                if (p_log_buffer < log_buffer_end - 1) { // Need space for % and the char
                    *p_log_buffer++ = '%';
                    *p_log_buffer++ = *c;
                }
                break;
        }
    }
    va_end(args);
    *p_log_buffer = '\0'; // Null-terminate the buffer

    if (Serial) { // Check if Serial is connected/initialized
        Serial.println(log_buffer);
    }
}