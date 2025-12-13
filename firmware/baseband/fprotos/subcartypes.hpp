
#ifndef __FPROTO_SUBCARTYPES_H__
#define __FPROTO_SUBCARTYPES_H__

/*
Define known protocols.
These values must be present on the protocol's constructor, like FProtoWeatherAcurite592TXR()  {   sensorType = FPS_ANSONIC;     }
Also it must have a switch-case element in the getSubGhzDSensorTypeName() function, to display it's name.
*/

#define FPM_AM 0
#define FPM_FM 1

enum FPROTO_SUBCAR_SENSOR : uint8_t {
    FPC_Invalid = 0,
    FPC_SUZUKI = 1,
    FPC_COUNT
};

#endif
