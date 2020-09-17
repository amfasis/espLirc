
#if true
    #define PRINTF(fmt, ...) Serial.printf_P( (PGM_P)PSTR(fmt), ## __VA_ARGS__ )
    #define PRINTLN(arg) Serial.println(arg)
#else
    #define PRINTF(fmt, ...)
    #define PRINTLN(arg)
#endif
