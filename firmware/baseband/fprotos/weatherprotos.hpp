/*
This is the protocol list handler. It holds an instance of all known protocols.
So include here the .hpp, and add a new element to the protos vector in the constructor. That's all you need to do here if you wanna add a new proto.
    @htotoo
*/

#include "fprotolistgeneral.hpp"

#include "w-nexus-th.hpp"
#include "w-acurite592txr.hpp"
#include "w-acurite606tx.hpp"
#include "w-acurite609tx.hpp"
#include "w-ambient.hpp"
#include "w-auriol-ahfl.hpp"
#include "w-auriol-th.hpp"
#include "w-gt-wt-02.hpp"
#include "w-gt-wt-03.hpp"
#include "w-infactory.hpp"
#include "w-lacrosse-tx.hpp"
#include "w-lacrosse-tx141thbv2.hpp"
#include "w-oregon2.hpp"
#include "w-oregon3.hpp"
#include "w-oregonv1.hpp"
#include "w-thermoprotx4.hpp"
#include "w-tx8300.hpp"
#include "w-wendox-w6726.hpp"
#include "w-acurite986.hpp"
#include "w-kedsum.hpp"
#include "w-acurite5in1.hpp"
#include "w-emose601x.hpp"

#include <vector>
#include <memory>
#include "portapack_shared_memory.hpp"

#ifndef __FPROTO_PROTOLISTWTH_H__
#define __FPROTO_PROTOLISTWTH_H__

class WeatherProtos : public FProtoListGeneral {
   public:
    WeatherProtos(const WeatherProtos&) { WeatherProtos(); };         // won't use, but makes compiler happy
    WeatherProtos& operator=(const WeatherProtos&) { return *this; }  // won't use, but makes compiler happy
    WeatherProtos() {
        // add protos
        protos[FPW_NexusTH] = new FProtoWeatherNexusTH();
        protos[FPW_Acurite592TXR] = new FProtoWeatherAcurite592TXR();
        protos[FPW_Acurite606TX] = new FProtoWeatherAcurite606TX();
        protos[FPW_Acurite609TX] = new FProtoWeatherAcurite609TX();
        protos[FPW_Ambient] = new FProtoWeatherAmbient();
        protos[FPW_AuriolAhfl] = new FProtoWeatherAuriolAhfl();
        protos[FPW_AuriolTH] = new FProtoWeatherAuriolTh();
        protos[FPW_GTWT02] = new FProtoWeatherGTWT02();
        protos[FPW_GTWT03] = new FProtoWeatherGTWT03();
        protos[FPW_INFACTORY] = new FProtoWeatherInfactory();
        protos[FPW_LACROSSETX] = new FProtoWeatherLaCrosseTx();
        protos[FPW_LACROSSETX141thbv2] = new FProtoWeatherLaCrosseTx141thbv2();
        protos[FPW_OREGON2] = new FProtoWeatherOregon2();
        protos[FPW_OREGON3] = new FProtoWeatherOregon3();
        protos[FPW_OREGONv1] = new FProtoWeatherOregonV1();
        protos[FPW_THERMOPROTX4] = new FProtoWeatherThermoProTx4();
        protos[FPW_TX_8300] = new FProtoWeatherTX8300();
        protos[FPW_WENDOX_W6726] = new FProtoWeatherWendoxW6726();
        protos[FPW_Acurite986] = new FProtoWeatherAcurite986();
        protos[FPW_KEDSUM] = new FProtoWeatherKedsum();
        protos[FPW_Acurite5in1] = new FProtoWeatherAcurite5in1();
        protos[FPW_EmosE601x] = new FProtoWeatherEmosE601x();

        // set callback for them
        for (uint8_t i = 0; i < FPW_COUNT; ++i) {
            protos[i]->setCallback(callbackTarget);
        }
    }

    ~WeatherProtos() {  // not needed for current operation logic, but a bit more elegant :)
        for (uint8_t i = 0; i < FPW_COUNT; ++i) {
            if (protos[i] != NULL) {
                free(protos[i]);
                protos[i] = NULL;
            }
        }
    };

    static void callbackTarget(FProtoWeatherBase* instance) {
        WeatherDataMessage packet_message{instance->getSensorType(), instance->getSensorId(),
                                          instance->getTemp(), instance->getHumidity(), instance->getBattLow(),
                                          instance->getChannel()};
        shared_memory.application_queue.push(packet_message);
    }

    void feed(bool level, uint32_t duration) {
        for (uint8_t i = 0; i < FPW_COUNT; ++i) {
            if (protos[i] != NULL) protos[i]->feed(level, duration);
        }
    }

   protected:
    FProtoWeatherBase* protos[FPW_COUNT] = {NULL};
};

#endif
