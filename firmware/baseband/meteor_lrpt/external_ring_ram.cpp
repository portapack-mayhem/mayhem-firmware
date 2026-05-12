/*
 * Copyright (C) 2026
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "external_ring.hpp"

#include <new>
#include <cstring>

namespace meteor_lrpt {

MeteorDeintRamRing::MeteorDeintRamRing() {
    data_ = new (std::nothrow) int8_t[kDeinterleaverRingBytes]();
}

MeteorDeintRamRing::~MeteorDeintRamRing() {
    delete[] data_;
    data_ = nullptr;
}

void MeteorDeintRamRing::write_byte(uint32_t index_mod, int8_t v) {
    if (!data_)
        return;
    data_[index_mod % kDeinterleaverRingBytes] = v;
}

int8_t MeteorDeintRamRing::read_byte(uint32_t index_mod) const {
    if (!data_)
        return 0;
    return data_[index_mod % kDeinterleaverRingBytes];
}

}  // namespace meteor_lrpt
