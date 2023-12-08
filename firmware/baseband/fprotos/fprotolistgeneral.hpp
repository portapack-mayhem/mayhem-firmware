#ifndef __FPROTO_PROTOLISTGENERAL_H__
#define __FPROTO_PROTOLISTGENERAL_H__
#include <stdint.h>

class FProtoListGeneral {
   public:
    FProtoListGeneral() {}
    virtual ~FProtoListGeneral() {}
    virtual void feed(bool level, uint32_t duration) = 0;
    void setModulation(uint8_t modulation) { modulation_ = modulation; }

   protected:
    uint8_t modulation_ = 0;
};

#endif