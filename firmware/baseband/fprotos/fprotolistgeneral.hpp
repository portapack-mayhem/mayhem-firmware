#ifndef __FPROTO_PROTOLISTGENERAL_H__
#define __FPROTO_PROTOLISTGENERAL_H__
#include <stdint.h>

class FProtoListGeneral {
   public:
    FProtoListGeneral() {}
    virtual ~FProtoListGeneral() {}
    virtual void feed(bool level, uint32_t duration) = 0;
};

#endif