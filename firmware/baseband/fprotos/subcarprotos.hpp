/*
This is the protocol list handler. It holds an instance of all known protocols.
So include here the .hpp, and add a new element to the protos vector in the constructor. That's all you need to do here if you wanna add a new proto.
    @htotoo
*/

#include <vector>
#include <memory>
#include "portapack_shared_memory.hpp"

#include "fprotolistgeneral.hpp"
#include "subcarbase.hpp"
#include "c-suzuki.hpp"
#include "c-vw.hpp"
#include "c-subaru.hpp"
#include "c-kia_v5.hpp"
#include "c-kia_v3v4.hpp"
#include "c-kia_v2.hpp"
#include "c-kia_v1.hpp"
#include "c-kia_v0.hpp"
#include "c-ford_v0.hpp"
#include "c-fiat_v0.hpp"
#include "c-bmw_v0.hpp"

#ifndef __FPROTO_PROTOLISTCAR_H__
#define __FPROTO_PROTOLISTCAR_H__

class SubCarProtos : public FProtoListGeneral {
   public:
    SubCarProtos(const SubCarProtos&) { SubCarProtos(); };          // won't use, but makes compiler happy
    SubCarProtos& operator=(const SubCarProtos&) { return *this; }  // won't use, but makes compiler happy
    SubCarProtos() {
        // add protos
        protos[FPC_SUZUKI] = new FProtoSubCarSuzuki();
        protos[FPC_VW] = new FProtoSubCarVW();
        protos[FPC_SUBARU] = new FProtoSubCarSubaru();
        protos[FPC_KIAV5] = new FProtoSubCarKiaV5();
        protos[FPC_KIAV3V4] = new FProtoSubCarKiaV3V4();
        protos[FPC_KIAV2] = new FProtoSubCarKiaV2();
        protos[FPC_KIAV1] = new FProtoSubCarKiaV1();
        protos[FPC_KIAV0] = new FProtoSubCarKiaV0();
        protos[FPC_FORDV0] = new FProtoSubCarFordV0();
        protos[FPC_FIATV0] = new FProtoSubCarFiatV0();
        protos[FPC_BMWV0] = new FProtoSubCarBMWV0();

        for (uint8_t i = 0; i < FPC_COUNT; ++i) {
            if (protos[i] != NULL) protos[i]->setCallback(callbackTarget);
        }
    }

    ~SubCarProtos() {  // not needed for current operation logic, but a bit more elegant :)
        for (uint8_t i = 0; i < FPC_COUNT; ++i) {
            if (protos[i] != NULL) {
                free(protos[i]);
                protos[i] = NULL;
            }
        }
    };

    static void callbackTarget(FProtoSubCarBase* instance) {
        SubCarDataMessage packet_message{instance->sensorType, instance->data_count_bit, instance->decode_data, instance->decode_data2};
        shared_memory.application_queue.push(packet_message);
    }

    void feed(bool level, uint32_t duration) {
        for (uint8_t i = 0; i < FPC_COUNT; ++i) {
            if (protos[i] != NULL) protos[i]->feed(level, duration);
        }
    }

   protected:
    FProtoSubCarBase* protos[FPC_COUNT] = {NULL};
};

#endif
