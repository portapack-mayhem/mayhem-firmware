
#ifndef __FPROTO_SUBGHZDTYPES_H__
#define __FPROTO_SUBGHZDTYPES_H__

/*
Define known protocols.
These values must be present on the protocol's constructor, like FProtoWeatherAcurite592TXR()  {   sensorType = FPW_Acurite592TXR;     }
Also it must have a switch-case element in the getSubGhzDSensorTypeName() function, to display it's name.
*/

enum FPROTO_SUBGHZD_MODULATION {
    FPM_AM = 0,
    FPM_FM = 1,
};

enum FPROTO_SUBGHZD_SENSOR {
    FPS_Invalid = 0,
    FPS_ANSONIC = 1,

};

#endif