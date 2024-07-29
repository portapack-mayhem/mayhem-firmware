
#ifndef __FPROTO_WEATHERTYPES_H__
#define __FPROTO_WEATHERTYPES_H__

/*
Define known protocols.
These values must be present on the protocol's constructor, like FProtoWeatherAcurite592TXR()  {   sensorType = FPW_Acurite592TXR;     }
Also it must have a switch-case element in the getWeatherSensorTypeName() function, to display it's name.
*/
#define WS_NO_ID 0xFFFFFFFF
#define WS_NO_BATT 0xFF
#define WS_NO_HUMIDITY 0xFF
#define WS_NO_CHANNEL 0xFF
// #define WS_NO_BTN 0xFF
#define WS_NO_TEMPERATURE -273.0f

enum FPROTO_WEATHER_SENSOR {
    FPW_Invalid = 0,
    FPW_NexusTH = 1,
    FPW_Acurite592TXR = 2,
    FPW_Acurite606TX = 3,
    FPW_Acurite609TX = 4,
    FPW_Ambient = 5,
    FPW_AuriolAhfl = 6,
    FPW_AuriolTH = 7,
    FPW_GTWT02 = 8,
    FPW_GTWT03 = 9,
    FPW_INFACTORY = 10,
    FPW_LACROSSETX = 11,
    FPW_LACROSSETX141thbv2 = 12,
    FPW_OREGON2 = 13,
    FPW_OREGON3 = 14,
    FPW_OREGONv1 = 15,
    FPW_THERMOPROTX4 = 16,
    FPW_TX_8300 = 17,
    FPW_WENDOX_W6726 = 18,
    FPW_Acurite986 = 19,
    FPW_KEDSUM = 20,
    FPW_Acurite5in1 = 21,
    FPW_EmosE601x = 22,
    FPW_COUNT  // this must be the last
};

#endif