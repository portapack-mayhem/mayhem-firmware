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
    WeatherProtos() {
        // add protos
        protos.push_back(std::make_unique<FProtoWeatherNexusTH>());             // 1
        protos.push_back(std::make_unique<FProtoWeatherAcurite592TXR>());       // 2
        protos.push_back(std::make_unique<FProtoWeatherAcurite606TX>());        // 3
        protos.push_back(std::make_unique<FProtoWeatherAcurite609TX>());        // 4
        protos.push_back(std::make_unique<FProtoWeatherAmbient>());             // 5
        protos.push_back(std::make_unique<FProtoWeatherAuriolAhfl>());          // 6
        protos.push_back(std::make_unique<FProtoWeatherAuriolTh>());            // 7
        protos.push_back(std::make_unique<FProtoWeatherGTWT02>());              // 8
        protos.push_back(std::make_unique<FProtoWeatherGTWT03>());              // 9
        protos.push_back(std::make_unique<FProtoWeatherInfactory>());           // 10
        protos.push_back(std::make_unique<FProtoWeatherLaCrosseTx>());          // 11
        protos.push_back(std::make_unique<FProtoWeatherLaCrosseTx141thbv2>());  // 12
        protos.push_back(std::make_unique<FProtoWeatherOregon2>());             // 13
        protos.push_back(std::make_unique<FProtoWeatherOregon3>());             // 14
        protos.push_back(std::make_unique<FProtoWeatherOregonV1>());            // 15
        protos.push_back(std::make_unique<FProtoWeatherThermoProTx4>());        // 16
        protos.push_back(std::make_unique<FProtoWeatherTX8300>());              // 17
        protos.push_back(std::make_unique<FProtoWeatherWendoxW6726>());         // 18
        protos.push_back(std::make_unique<FProtoWeatherAcurite986>());          // 19
        protos.push_back(std::make_unique<FProtoWeatherKedsum>());              // 20
        protos.push_back(std::make_unique<FProtoWeatherAcurite5in1>());         // 21
        protos.push_back(std::make_unique<FProtoWeatherEmosE601x>());           // 22

        // set callback for them
        for (const auto& obj : protos) {
            obj->setCallback(callbackTarget);
        }
    }

    static void callbackTarget(FProtoWeatherBase* instance) {
        WeatherDataMessage packet_message{instance->getSensorType(), instance->getSensorId(),
                                          instance->getTemp(), instance->getHumidity(), instance->getBattLow(),
                                          instance->getChannel()};
        shared_memory.application_queue.push(packet_message);
    }

    void feed(bool level, uint32_t duration) {
        for (const auto& obj : protos) {
            obj->feed(level, duration);
        }
    }

   protected:
    std::vector<std::unique_ptr<FProtoWeatherBase>> protos{};
};

#endif
