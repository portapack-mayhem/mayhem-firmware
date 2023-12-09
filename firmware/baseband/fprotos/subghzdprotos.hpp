/*
This is the protocol list handler. It holds an instance of all known protocols.
So include here the .hpp, and add a new element to the protos vector in the constructor. That's all you need to do here if you wanna add a new proto.
    @htotoo
*/

#include <vector>
#include <memory>
#include "portapack_shared_memory.hpp"

#include "fprotolistgeneral.hpp"
#include "subghzdbase.hpp"
#include "s-ansonic.hpp"
#include "s-princeton.hpp"
#include "s-bett.hpp"
#include "s-came.hpp"
#include "s-came_atomo.hpp"

#ifndef __FPROTO_PROTOLISTSGZ_H__
#define __FPROTO_PROTOLISTSGZ_H__

class SubGhzDProtos : public FProtoListGeneral {
   public:
    SubGhzDProtos() {
        // add protos
        protos.push_back(std::make_unique<FProtoSubGhzDAnsonic>());    // 1
        protos.push_back(std::make_unique<FProtoSubGhzDPrinceton>());  // 2
        protos.push_back(std::make_unique<FProtoSubGhzDBett>());       // 3
        protos.push_back(std::make_unique<FProtoSubGhzDCame>());       // 4, 5, 6
        protos.push_back(std::make_unique<FProtoSubGhzDCameAtomo>());  // 7

        // set callback for them
        for (const auto& obj : protos) {
            obj->setCallback(callbackTarget);
        }
    }

    static void callbackTarget(FProtoSubGhzDBase* instance) {
        SubGhzDDataMessage packet_message{instance->getSensorType(), instance->getSensorSerial(), instance->getBits(), instance->getData(), instance->getData2(), instance->getBtn()};
        // todo add cnt, cnt2
        shared_memory.application_queue.push(packet_message);
    }

    void feed(bool level, uint32_t duration) {
        for (const auto& obj : protos) {
            if (obj->modulation == modulation_) obj->feed(level, duration);
        }
    }

   protected:
    std::vector<std::unique_ptr<FProtoSubGhzDBase>> protos{};
};

#endif
