
#ifndef __FPROTO_SUBGHZDTYPES_H__
#define __FPROTO_SUBGHZDTYPES_H__

/*
Define known protocols.
These values must be present on the protocol's constructor, like FProtoWeatherAcurite592TXR()  {   sensorType = FPS_ANSONIC;     }
Also it must have a switch-case element in the getSubGhzDSensorTypeName() function, to display it's name.
*/

#define FPM_AM 0
#define FPM_FM 1

#define SD_NO_SERIAL 0xFFFFFFFF
#define SD_NO_BTN 0xFF
#define SD_NO_CNT 0xFF
#define SD_NO_KEY 0xFFFFFFFF
#define SD_NO_SEED 0xFFFFFFFF

enum FPROTO_SUBGHZD_SENSOR {
    FPS_Invalid = 0,
    FPS_ANSONIC = 1,
    FPS_PRINCETON = 2,
    FPS_BETT = 3,
    FPS_CAME = 4,
    FPS_PRASTEL = 5,
    FPS_AIRFORCE = 6,
    FPS_CAMEATOMO = 7,
    FPS_CAMETWEE = 8,
    FPS_CHAMBCODE = 9,
    FPS_CLEMSA = 10,
    FPS_DOITRAND = 11,
    FPS_DOOYA = 12,
    FPS_FAAC = 13,
    FPS_GATETX = 14,
    FPS_HOLTEK = 15,
    FPS_HOLTEKHT12X = 16,
    FPS_HONEYWELL = 17,
    FPS_HONEYWELLWDB = 18,
    FPS_HORMANN = 19,
    FPS_IDO = 20,
    FPS_INTERTECHNOV3 = 21,
    FPS_KEELOQ = 22,
    FPS_KIA = 23,
    FPS_KINGGATESSTYLO4K = 24,
    FPS_LINEAR = 25,
    FPS_LINEARDELTA3 = 26,
    FPS_MAGELLAN = 27,
    FPS_MARANTEC = 28,
    FPS_MASTERCODE = 29,
    FPS_MEGACODE = 30,

};

#endif
