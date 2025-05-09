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
void zap_log_message_f(zap_log_level_t level, const char *tag, const char *format, ...);



// use like:
// static constexpr LogTag TAG = LogTag("main", ZLOG_LEVEL_INFO);
// this will enable compile time removal of logging code that will not be used due to the log level.
struct LogTag {
    const char* tag;
    const zap_log_level_t defaultLevel;

    constexpr LogTag(const char* t, zap_log_level_t l) : tag(t), defaultLevel(l) {}
};

#define LOG_MESSAGE(level, tag_struct, format, ...) \
        do { \
            if ( (level <= (tag_struct).defaultLevel) ) { \
                zap_log_message_f(level, (tag_struct).tag, format, ##__VA_ARGS__); \
            } \
        } while(0)


#ifdef PLATFORMIO

    #define LOG_TE( tag_struct, format, ... ) LOG_MESSAGE(ZLOG_LEVEL_ERROR,   tag_struct, format, ##__VA_ARGS__)
    #define LOG_TW( tag_struct, format, ... ) LOG_MESSAGE(ZLOG_LEVEL_WARN,   tag_struct, format, ##__VA_ARGS__)
    #define LOG_TI( tag_struct, format, ... ) LOG_MESSAGE(ZLOG_LEVEL_INFO,   tag_struct, format, ##__VA_ARGS__)
    #define LOG_TD( tag_struct, format, ... ) LOG_MESSAGE(ZLOG_LEVEL_DEBUG,   tag_struct, format, ##__VA_ARGS__)
    #define LOG_TV( tag_struct, format, ... ) LOG_MESSAGE(ZLOG_LEVEL_VERBOSE,   tag_struct, format, ##__VA_ARGS__)

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