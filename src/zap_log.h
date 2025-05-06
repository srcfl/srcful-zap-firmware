#pragma once

#ifdef PLATFORMIO

    #include <esp_log.h>

    // log level macros
    #define LOG_E( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR,   tag, format __VA_OPT__(,) __VA_ARGS__)
    #define LOG_W( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_WARN,    tag, format __VA_OPT__(,) __VA_ARGS__)
    #define LOG_I( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO,    tag, format __VA_OPT__(,) __VA_ARGS__)
    #define LOG_D( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_DEBUG,   tag, format __VA_OPT__(,) __VA_ARGS__)
    #define LOG_V( tag, format, ... ) ESP_LOG_LEVEL_LOCAL(ESP_LOG_VERBOSE, tag, format __VA_OPT__(,) __VA_ARGS__)
#else

    // expand to nothing
    #define LOG_E( tag, format, ... ) 
    #define LOG_W( tag, format, ... ) 
    #define LOG_I( tag, format, ... ) 
    #define LOG_D( tag, format, ... ) 
    #define LOG_V( tag, format, ... ) 
#endif