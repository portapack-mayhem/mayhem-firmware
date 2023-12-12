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
// #include "s-ansonic.hpp" //skip, since fm
#include "s-princeton.hpp"
#include "s-bett.hpp"
#include "s-came.hpp"
#include "s-came_atomo.hpp"
#include "s-came_twee.hpp"
#include "s-chambcode.hpp"
#include "s-clemsa.hpp"
#include "s-doitrand.hpp"
#include "s-dooya.hpp"
#include "s-faac.hpp"
#include "s-gate_tx.hpp"
#include "s-holtek.hpp"
#include "s-holtek_ht12x.hpp"
#include "s-honeywell.hpp"
#include "s-honeywellwdb.hpp"
#include "s-hormann.hpp"
#include "s-ido.hpp"
#include "s-intertechnov3.hpp"
#include "s-keeloq.hpp"
// #include "s-kia.hpp"
#include "s-kinggates_stylo_4k.hpp"
#include "s-linear.hpp"
#include "s-linear_delta3.hpp"
#include "s-magellan.hpp"
#include "s-marantec.hpp"
#include "s-mastercode.hpp"
#include "s-megacode.hpp"
#include "s-neroradio.hpp"

#ifndef __FPROTO_PROTOLISTSGZ_H__
#define __FPROTO_PROTOLISTSGZ_H__

class SubGhzDProtos : public FProtoListGeneral {
   public:
    SubGhzDProtos() {
        // add protos
        // protos.push_back(std::make_unique<FProtoSubGhzDAnsonic>());    // 1 //skip since fm
        protos.push_back(std::make_unique<FProtoSubGhzDPrinceton>());      // 2
        protos.push_back(std::make_unique<FProtoSubGhzDBett>());           // 3
        protos.push_back(std::make_unique<FProtoSubGhzDCame>());           // 4, 5, 6
        protos.push_back(std::make_unique<FProtoSubGhzDCameAtomo>());      // 7
        protos.push_back(std::make_unique<FProtoSubGhzDCameTwee>());       // 8
        protos.push_back(std::make_unique<FProtoSubGhzDChambCode>());      // 9
        protos.push_back(std::make_unique<FProtoSubGhzDClemsa>());         // 10
        protos.push_back(std::make_unique<FProtoSubGhzDDoitrand>());       // 11
        protos.push_back(std::make_unique<FProtoSubGhzDDooya>());          // 12
        protos.push_back(std::make_unique<FProtoSubGhzDFaac>());           // 13
        protos.push_back(std::make_unique<FProtoSubGhzDCGateTx>());        // 14
        protos.push_back(std::make_unique<FProtoSubGhzDCHoltek>());        // 15
        protos.push_back(std::make_unique<FProtoSubGhzDCHoltekHt12x>());   // 16
        protos.push_back(std::make_unique<FProtoSubGhzDCHoneywell>());     // 17
        protos.push_back(std::make_unique<FProtoSubGhzDCHoneywellWdb>());  // 18
        protos.push_back(std::make_unique<FProtoSubGhzDCHormann>());       // 19
        protos.push_back(std::make_unique<FProtoSubGhzDIdo>());            // 20
        protos.push_back(std::make_unique<FProtoSubGhzDIntertechnoV3>());  // 21
        protos.push_back(std::make_unique<FProtoSubGhzDKeeLoq>());         // 22
        // protos.push_back(std::make_unique<FProtoSubGhzDKia>());            // 23
        protos.push_back(std::make_unique<FProtoSubGhzDKinggatesStylo4K>());  // 24
        protos.push_back(std::make_unique<FProtoSubGhzDLinear>());            // 25
        protos.push_back(std::make_unique<FProtoSubGhzDLinearDelta3>());      // 26
        protos.push_back(std::make_unique<FProtoSubGhzDMagellan>());          // 27
        protos.push_back(std::make_unique<FProtoSubGhzDMarantec>());          // 28
        protos.push_back(std::make_unique<FProtoSubGhzDMastercode>());        // 29
        protos.push_back(std::make_unique<FProtoSubGhzDMegacode>());          // 30
        protos.push_back(std::make_unique<FProtoSubGhzDNeroRadio>());         // 31

        // set callback for them
        for (const auto& obj : protos) {
            obj->setCallback(callbackTarget);
        }
    }

    static void callbackTarget(FProtoSubGhzDBase* instance) {
        SubGhzDDataMessage packet_message{instance->getSensorType(), instance->getSensorSerial(), instance->getBits(), instance->getData(), instance->getData2(), instance->getBtn()};
        // todo add cnt, cnt2, maybe hop
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
