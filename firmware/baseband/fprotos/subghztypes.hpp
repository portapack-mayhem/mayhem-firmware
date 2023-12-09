
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

};

#endif