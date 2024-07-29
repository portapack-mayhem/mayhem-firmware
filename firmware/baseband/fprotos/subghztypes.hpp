
#ifndef __FPROTO_SUBGHZDTYPES_H__
#define __FPROTO_SUBGHZDTYPES_H__

/*
Define known protocols.
These values must be present on the protocol's constructor, like FProtoWeatherAcurite592TXR()  {   sensorType = FPS_ANSONIC;     }
Also it must have a switch-case element in the getSubGhzDSensorTypeName() function, to display it's name.
*/

#define FPM_AM 0
#define FPM_FM 1

enum FPROTO_SUBGHZD_SENSOR : uint8_t {
    FPS_Invalid = 0,
    FPS_PRINCETON,
    FPS_BETT,
    FPS_CAME,
    FPS_PRASTEL,
    FPS_AIRFORCE,
    FPS_CAMEATOMO,
    FPS_CAMETWEE,
    FPS_CHAMBCODE,
    FPS_CLEMSA,
    FPS_DOITRAND,
    FPS_DOOYA,
    FPS_FAAC,
    FPS_GATETX,
    FPS_HOLTEK,
    FPS_HOLTEKHT12X,
    FPS_HONEYWELL,
    FPS_HONEYWELLWDB,
    FPS_HORMANN,
    // FPS_HORMANNBISECURE,
    FPS_IDO,
    FPS_INTERTECHNOV3,
    FPS_KEELOQ,
    FPS_KINGGATESSTYLO4K,
    FPS_LEGRAND,
    FPS_LINEAR,
    FPS_LINEARDELTA3,
    FPS_MAGELLAN,
    FPS_MARANTEC,
    FPS_MASTERCODE,
    FPS_MEGACODE,
    FPS_NERORADIO,
    FPS_NERO_SKETCH,
    FPS_NICEFLO,
    FPS_NICEFLORS,
    FPS_PHOENIXV2,
    FPS_POWERSMART,
    FPS_SECPLUSV1,
    FPS_SECPLUSV2,
    FPS_SMC5326,
    FPS_STARLINE,
    FPS_X10,
    FPS_SOMIFY_KEYTIS,
    FPS_SOMIFY_TELIS,
    FPS_COUNT
};

#endif
