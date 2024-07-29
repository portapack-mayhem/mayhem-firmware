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
#include "s-secplus_v2.hpp"
#include "s-smc5326.hpp"
#include "s-star_line.hpp"
#include "s-x10.hpp"
// #include "s-hormannbisecure.hpp" //fm
#include "s-legrand.hpp"
#include "s-somify_keytis.hpp"
#include "s-somify_telis.hpp"

// GENIE FROM PR

#ifndef __FPROTO_PROTOLISTSGZ_H__
#define __FPROTO_PROTOLISTSGZ_H__

class SubGhzDProtos : public FProtoListGeneral {
   public:
    SubGhzDProtos(const SubGhzDProtos&) { SubGhzDProtos(); };         // won't use, but makes compiler happy
    SubGhzDProtos& operator=(const SubGhzDProtos&) { return *this; }  // won't use, but makes compiler happy
    SubGhzDProtos() {
        // add protos
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
        protos[FPS_SECPLUSV2] = new FProtoSubGhzDSecPlusV2();
        protos[FPS_SMC5326] = new FProtoSubGhzDSmc5326();
        protos[FPS_SOMIFY_KEYTIS] = new FProtoSubGhzDSomifyKeytis();
        protos[FPS_SOMIFY_TELIS] = new FProtoSubGhzDSomifyTelis();
        protos[FPS_STARLINE] = new FProtoSubGhzDStarLine();
        protos[FPS_X10] = new FProtoSubGhzDX10();
        // protos[FPS_HORMANNBISECURE] = new FProtoSubGhzDHormannBiSecure();  //fm
        protos[FPS_LEGRAND] = new FProtoSubGhzDLegrand();

        for (uint8_t i = 0; i < FPS_COUNT; ++i) {
            if (protos[i] != NULL) protos[i]->setCallback(callbackTarget);
        }
    }

    ~SubGhzDProtos() {  // not needed for current operation logic, but a bit more elegant :)
        for (uint8_t i = 0; i < FPS_COUNT; ++i) {
            if (protos[i] != NULL) {
                free(protos[i]);
                protos[i] = NULL;
            }
        }
    };

    static void callbackTarget(FProtoSubGhzDBase* instance) {
        SubGhzDDataMessage packet_message{instance->sensorType, instance->data_count_bit, instance->data};
        shared_memory.application_queue.push(packet_message);
    }

    void feed(bool level, uint32_t duration) {
        for (uint8_t i = 0; i < FPS_COUNT; ++i) {
            if (protos[i] != NULL) protos[i]->feed(level, duration);
        }
    }

   protected:
    FProtoSubGhzDBase* protos[FPS_COUNT] = {NULL};
};

#endif
