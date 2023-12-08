
#ifndef __FPROTO_SUBGHZDTYPES_H__
#define __FPROTO_SUBGHZDTYPES_H__

/*
Define known protocols.
These values must be present on the protocol's constructor, like FProtoWeatherAcurite592TXR()  {   sensorType = FPS_ANSONIC;     }
Also it must have a switch-case element in the getSubGhzDSensorTypeName() function, to display it's name.
*/

#define FPM_AM 0
#define FPM_FM 1

enum FPROTO_SUBGHZD_SENSOR {
    FPS_Invalid = 0,
    FPS_ANSONIC = 1,
    FPS_PRINCETON = 2,

};

#endif