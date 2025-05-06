#pragma once

// Define log levels
enum zap_log_level_t {
    ZLOG_LEVEL_NONE,
    ZLOG_LEVEL_ERROR,
    ZLOG_LEVEL_WARN,
    ZLOG_LEVEL_INFO,
    ZLOG_LEVEL_DEBUG,
    ZLOG_LEVEL_VERBOSE
};

void set_log_level(zap_log_level_t level);
zap_log_level_t get_log_level();

// Main logging function declaration
void zap_log_message(zap_log_level_t level, const char *tag, const char *format, ...);

#ifdef PLATFORMIO
    // log level macros
    #define LOG_E( tag, format, ... ) zap_log_message(ZLOG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
    #define LOG_W( tag, format, ... ) zap_log_message(ZLOG_LEVEL_WARN, tag, format, ##__VA_ARGS__)
    #define LOG_I( tag, format, ... ) zap_log_message(ZLOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
    #define LOG_D( tag, format, ... ) zap_log_message(ZLOG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
    #define LOG_V( tag, format, ... ) zap_log_message(ZLOG_LEVEL_VERBOSE, tag, format, ##__VA_ARGS__)
#else

    // expand to nothing
    #define LOG_E( tag, format, ... ) 
    #define LOG_W( tag, format, ... ) 
    #define LOG_I( tag, format, ... ) 
    #define LOG_D( tag, format, ... ) 
    #define LOG_V( tag, format, ... ) 

#endif