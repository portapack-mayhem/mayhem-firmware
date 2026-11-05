
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
#define WS_NO_U8 0xFF
#define WS_NO_U16 0xFFFF
#define WS_NO_U32 0xFFFFFFFF
#define WS_MAX_PAYLOAD 32  // max raw frame bytes a protocol can pass to the app (Fine Offset WS90 is the longest)

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
    FPW_SolightTE44 = 23,
    FPW_Bresser3CH = 24,
    FPW_Bresser3CH_V1 = 25,
    FPW_Vauno_EN8822 = 26,
    FPW_OREGON2B = 27,
    // Fine Offset / Ecowitt family. OOK ones are done by FProtoWeatherFineOffsetOOK, FSK ones by FProtoWeatherFineOffsetFSK.
    FPW_FO_WH2 = 28,
    FPW_FO_WH5 = 29,
    FPW_FO_WH0530 = 30,
    FPW_FO_WH1080 = 31,
    FPW_FO_WH3080UV = 32,
    FPW_FO_WH53 = 33,
    FPW_FO_WH24 = 34,
    FPW_FO_WH65 = 35,
    FPW_FO_WS69 = 36,
    FPW_FO_WH25 = 37,
    FPW_FO_WH32 = 38,
    FPW_FO_WH0290 = 39,
    FPW_FO_WH31E = 40,
    FPW_FO_WH40 = 41,
    FPW_FO_WN20 = 42,
    FPW_FO_WS68 = 43,
    FPW_FO_WH51 = 44,
    FPW_FO_WS80 = 45,
    FPW_FO_WS85 = 46,
    FPW_FO_WS90 = 47,
    FPW_FO_WN34 = 48,
    FPW_FO_WH57 = 49,
    FPW_FO_WH55 = 50,
    FPW_COUNT  // this must be the last
};

#endif