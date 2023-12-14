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
#include "s-kinggates_stylo_4k.hpp"
#include "s-linear.hpp"
#include "s-linear_delta3.hpp"
#include "s-magellan.hpp"
#include "s-marantec.hpp"
#include "s-mastercode.hpp"
#include "s-megacode.hpp"
#include "s-neroradio.hpp"
#include "s-nero_sketch.hpp"
#include "s-nice_flo.hpp"
#include "s-nice_flors.hpp"
#include "s-phoenix_v2.hpp"
#include "s-power_smart.hpp"
#include "s-secplus_v1.hpp"

// GENIE FROM PR

#ifndef __FPROTO_PROTOLISTSGZ_H__
#define __FPROTO_PROTOLISTSGZ_H__

class SubGhzDProtos : public FProtoListGeneral {
   public:
    SubGhzDProtos(const SubGhzDProtos&){};                           // wont use, but makes compiler happy
    SubGhzDProtos* operator=(const SubGhzDProtos&) { return NULL; }  // wont use, but makes compiler happy
    SubGhzDProtos() {
        // add protos
        // 1 //skip since fm
        /*  protos.push_back(std::make_unique<FProtoSubGhzDPrinceton>());         // 2
          protos.push_back(std::make_unique<FProtoSubGhzDBett>());              // 3
          protos.push_back(std::make_unique<FProtoSubGhzDCame>());              // 4, 5, 6
          protos.push_back(std::make_unique<FProtoSubGhzDCameAtomo>());         // 7
          protos.push_back(std::make_unique<FProtoSubGhzDCameTwee>());          // 8
          protos.push_back(std::make_unique<FProtoSubGhzDChambCode>());         // 9
          protos.push_back(std::make_unique<FProtoSubGhzDClemsa>());            // 10
          protos.push_back(std::make_unique<FProtoSubGhzDDoitrand>());          // 11
          protos.push_back(std::make_unique<FProtoSubGhzDDooya>());             // 12
          protos.push_back(std::make_unique<FProtoSubGhzDFaac>());              // 13
          protos.push_back(std::make_unique<FProtoSubGhzDGateTx>());            // 14
          protos.push_back(std::make_unique<FProtoSubGhzDHoltek>());            // 15
          protos.push_back(std::make_unique<FProtoSubGhzDHoltekHt12x>());       // 16
          protos.push_back(std::make_unique<FProtoSubGhzDHoneywell>());         // 17
          protos.push_back(std::make_unique<FProtoSubGhzDHoneywellWdb>());      // 18
          protos.push_back(std::make_unique<FProtoSubGhzDHormann>());           // 19
          protos.push_back(std::make_unique<FProtoSubGhzDIdo>());               // 20
          protos.push_back(std::make_unique<FProtoSubGhzDIntertechnoV3>());     // 21
          protos.push_back(std::make_unique<FProtoSubGhzDKeeLoq>());            // 22
                                                                                // 23
          protos.push_back(std::make_unique<FProtoSubGhzDKinggatesStylo4K>());  // 24
          protos.push_back(std::make_unique<FProtoSubGhzDLinear>());            // 25
          protos.push_back(std::make_unique<FProtoSubGhzDLinearDelta3>());      // 26
          protos.push_back(std::make_unique<FProtoSubGhzDMagellan>());          // 27
          protos.push_back(std::make_unique<FProtoSubGhzDMarantec>());          // 28
          protos.push_back(std::make_unique<FProtoSubGhzDMastercode>());        // 29
          protos.push_back(std::make_unique<FProtoSubGhzDMegacode>());          // 30
          protos.push_back(std::make_unique<FProtoSubGhzDNeroRadio>());         // 31

          protos.push_back(std::make_unique<FProtoSubGhzDNeroSketch>());  // 32
          protos.push_back(std::make_unique<FProtoSubGhzDNiceflo>());     // 33
          protos.push_back(std::make_unique<FProtoSubGhzDNiceflors>());   // 34
          protos.push_back(std::make_unique<FProtoSubGhzDPhoenixV2>());   // 35
          protos.push_back(std::make_unique<FProtoSubGhzDPowerSmart>());  // 36
          protos.push_back(std::make_unique<FProtoSubGhzDSecPlusV1>());   // 37
  */
        protos[FPS_PRINCETON] = new FProtoSubGhzDPrinceton();
        protos[FPS_BETT] = new FProtoSubGhzDBett();
        protos[FPS_CAME] = new FProtoSubGhzDCame();
        protos[FPS_CAMEATOMO] = new FProtoSubGhzDCameAtomo();
        protos[FPS_CAMETWEE] = new FProtoSubGhzDCameTwee();
        protos[FPS_CHAMBCODE] = new FProtoSubGhzDChambCode();
        protos[FPS_CLEMSA] = new FProtoSubGhzDClemsa();
        protos[FPS_DOITRAND] = new FProtoSubGhzDDoitrand();
        protos[FPS_DOOYA] = new FProtoSubGhzDDooya();
        protos[FPS_FAAC] = new FProtoSubGhzDFaac();
        protos[FPS_GATETX] = new FProtoSubGhzDGateTx();
        protos[FPS_HOLTEK] = new FProtoSubGhzDHoltek();
        protos[FPS_HOLTEKHT12X] = new FProtoSubGhzDHoltekHt12x();
        protos[FPS_HONEYWELL] = new FProtoSubGhzDHoneywell();
        protos[FPS_HONEYWELLWDB] = new FProtoSubGhzDHoneywellWdb();
        protos[FPS_HORMANN] = new FProtoSubGhzDHormann();
        protos[FPS_IDO] = new FProtoSubGhzDIdo();
        protos[FPS_INTERTECHNOV3] = new FProtoSubGhzDIntertechnoV3();
        protos[FPS_KEELOQ] = new FProtoSubGhzDKeeLoq();
        protos[FPS_KINGGATESSTYLO4K] = new FProtoSubGhzDKinggatesStylo4K();
        protos[FPS_LINEAR] = new FProtoSubGhzDLinear();
        protos[FPS_LINEARDELTA3] = new FProtoSubGhzDLinearDelta3();
        protos[FPS_MAGELLAN] = new FProtoSubGhzDMagellan();
        protos[FPS_MARANTEC] = new FProtoSubGhzDMarantec();
        protos[FPS_MASTERCODE] = new FProtoSubGhzDMastercode();
        protos[FPS_MEGACODE] = new FProtoSubGhzDMegacode();
        protos[FPS_NERORADIO] = new FProtoSubGhzDNeroRadio();
        protos[FPS_NERO_SKETCH] = new FProtoSubGhzDNeroSketch();
        protos[FPS_NICEFLO] = new FProtoSubGhzDNiceflo();
        protos[FPS_NICEFLORS] = new FProtoSubGhzDNiceflors();
        protos[FPS_PHOENIXV2] = new FProtoSubGhzDPhoenixV2();
        protos[FPS_POWERSMART] = new FProtoSubGhzDPowerSmart();
        protos[FPS_SECPLUSV1] = new FProtoSubGhzDSecPlusV1();

        // set callback for them
        /*
        for (const auto& obj : protos) {
            obj->setCallback(callbackTarget);
        }*/
        for (uint8_t i = 0; i < FPS_COUNT; ++i) {
            if (protos[i] != NULL) protos[i]->setCallback(callbackTarget);
        }
    }

    static void callbackTarget(FProtoSubGhzDBase* instance) {
        SubGhzDDataMessage packet_message{instance->sensorType, instance->btn, instance->data_count_bit, instance->serial, instance->data, instance->cnt};
        shared_memory.application_queue.push(packet_message);
    }

    void feed(bool level, uint32_t duration) {
        for (uint8_t i = 0; i < FPS_COUNT; ++i) {
            if (protos[i] != NULL) feed(level, duration);
        }
    }

   protected:
    FProtoSubGhzDBase* protos[FPS_COUNT];
};

#endif
