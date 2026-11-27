/*
 * Copyright (C) 2026 Belousov Oleg aka R1CBU
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

.syntax unified
.cpu cortex-m4
.thumb

.section .text.vocoder_rx_5,"ax",%progbits
.balign 4
.global asm_decode_wav
.type asm_decode_wav, %function
.thumb_func
asm_decode_wav:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
    sub	sp, #152
    cmp	r1, #76
    ite	ge
    movge	r6, r1
    movlt	r6, #76
    ldr	r4, [sp, #192]
    ldrsh.w	r5, [sp, #188]
    mov	r9, r3
    movs	r3, #80
    mov	r7, r0
    mov	sl, r2
    cmp	r6, #84
    it	ge
    movge	r6, #84
    strh.w	r3, [r4, #372]
    cbz	r5, .Lcode_2384
    add.w	r0, r4, #1004
    mov	r1, r7
    mov	r2, r6
    mov	r3, r5
    str	r4, [sp, #0]
    bl vocoder_code_5850
    movs	r0, #0
.Lcode_2381:
    sxth	r0, r0
    add	sp, #152
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, pc}
.Lcode_2384:
    add.w	r8, sp, #16
    mov	r1, r5
    movs	r2, #68
    mov	r0, r8
    bl vocoder_code_2535
    mov	r1, r9
    add.w	lr, r4, #1004
    ldrsh.w	r2, [sp, #184]
    str.w	lr, [sp]
    mov	r3, r6
    mov	r0, sl
    str.w	r8, [sp, #4]
    str	r4, [sp, #8]
    bl vocoder_code_6302
    mov	r1, r7
    mov	r9, r0
    mov	r2, r6
    mov	r0, r8
    mov	r3, r5
    str	r4, [sp, #0]
    bl vocoder_code_5850
    uxth.w	r0, r9
    b.n .Lcode_2381
    .byte 0x00
    .byte 0xbf


.section .text.vocoder_rx_29,"ax",%progbits
.balign 4
.global vocoder_code_5212
.hidden vocoder_code_5212
vocoder_code_5212:
.global asm_decode_and_prepare_frame
.type asm_decode_and_prepare_frame, %function
.thumb_func
asm_decode_and_prepare_frame:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #28
    mov	r5, r0
    ldr	r4, [sp, #84]
    ldr.w	r8, [sp, #64]
    ldrh	r1, [r4, #0]
    ldr.w	r9, [sp, #68]
    ldr.w	fp, [sp, #80]
    ldrsh.w	r7, [sp, #96]
    mov	r6, r2
    cbnz	r1, .Lcode_5226
.Lcode_5223:
    sxth	r0, r3
    add	sp, #28
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_5226:
    ldr	r3, [sp, #72]
    ldr	r2, .Lliteral_5366
    ldrb	r2, [r2, r3]
    sxth	r1, r1
    rsb	r2, r2, #49
    mov	r0, r4
    bl vocoder_code_4957
    add	r0, sp, #16
    mov	r1, r9
    bl vocoder_code_13469
    add.w	r0, r9, #12
    mov	r3, r9
    movs	r2, #0
.Lcode_5239:
    ldrsh.w	r1, [r3], #2
    cmp	r3, r0
    add.w	r2, r1, r2, lsl #1
    bne.n .Lcode_5239
    ldrsh.w	r3, [r4]
    cmp	r3, #0
    beq.n .Lcode_5327
    cmp	r2, #63
    beq.n .Lcode_5334
    cmp	r2, #62
    beq.w .Lcode_5350
    cmp	r2, #59
    ite	gt
    movgt	r2, #0
    movle	r2, #1
    uxth	r1, r2
    movs	r3, #255
    strh	r1, [r6, #0]
    strh	r3, [r6, #2]
    strh	r1, [r5, #0]
    strh	r3, [r5, #2]
    cmp	r2, #0
    beq.n .Lcode_5332
    mov.w	sl, #1
.Lcode_5263:
    mov	r2, r4
    mov	r0, r9
    movs	r1, #1
    bl vocoder_code_14550
    ldrsh.w	r1, [r4, #10]
    add	r0, sp, #16
    bl vocoder_code_14920
    add.w	r9, sp, #24
    ldrsh.w	r1, [r4, #8]
    strh.w	r0, [r9, #-2]!
    add	r0, sp, #16
    bl vocoder_code_14920
    ldrsh.w	lr, [r4, #8]
    mov	r3, r0
    add.w	r1, r6, #8
    add.w	r2, r5, #8
    mov	r0, sl
    str.w	lr, [sp]
    bl vocoder_code_14929
    mov.w	ip, #0
    ldrsh.w	lr, [r4, #10]
    str.w	ip, [sp]
    mov	r0, r9
    mov	r1, r6
    mov	r2, r5
    mov	r3, ip
    str.w	lr, [sp, #4]
    str.w	ip, [sp, #8]
    bl vocoder_code_13784
    cmp	r0, #0
    beq.n .Lcode_5357
    movw	r2, #4217
    movs	r3, #56
    strh	r2, [r6, #12]
    strh	r3, [r6, #4]
    strh	r2, [r5, #12]
    strh	r3, [r5, #4]
.Lcode_5300:
    mov	r1, r5
    mov	r2, r4
    add	r3, sp, #16
    mov	r0, r8
    mov.w	lr, #0
    str.w	lr, [sp]
    bl vocoder_code_14522
    ldrsh.w	r2, [fp, #48]
    ldrsh.w	r3, [fp, #36]
    mov	r0, r5
    mov	r1, r8
    bl vocoder_code_14949
.Lcode_5312:
    mov	r2, r8
    movs	r3, #0
    mov	r0, r6
    mov	r1, r5
    str	r7, [sp, #0]
    bl vocoder_code_15256
    mov	r3, r4
    mov	r0, r8
    mov	r1, r5
    movs	r2, #0
    bl vocoder_code_14066
    movs	r3, #0
.Lcode_5324:
    movw	r2, #65533
    strh.w	r2, [r8, #136]
    b.n .Lcode_5223
.Lcode_5327:
    movs	r2, #255
    strh	r3, [r6, #0]
    strh	r2, [r6, #2]
    strh	r3, [r5, #0]
    strh	r2, [r5, #2]
.Lcode_5332:
    movs	r3, #4
    b.n .Lcode_5324
.Lcode_5334:
    movs	r2, #3
    movs	r3, #255
    strh	r2, [r6, #0]
    strh	r3, [r6, #2]
    strh	r2, [r5, #0]
    strh	r3, [r5, #2]
    mov	r0, r5
    mov	r1, r6
    add	r2, sp, #16
    mov	r3, r4
    str.w	r8, [sp]
    bl vocoder_code_15400
    cmp	r0, #0
    beq.n .Lcode_5312
    sxth	r3, r0
    b.n .Lcode_5324
.Lcode_5350:
    mov.w	sl, #2
    movs	r3, #255
    strh.w	sl, [r6]
    strh	r3, [r6, #2]
    strh.w	sl, [r5]
    strh	r3, [r5, #2]
    b.n .Lcode_5263
.Lcode_5357:
    mov	r1, r5
    ldrsh.w	lr, [r4, #10]
    ldrsh.w	r0, [r1], #12
    ldrsh.w	r3, [sp, #22]
    str	r7, [sp, #4]
    adds	r2, r5, #4
    str.w	lr, [sp]
    bl vocoder_code_15489
    b.n .Lcode_5300
.Lliteral_5366:
    .word vocoder_data_20778

.section .text.vocoder_rx_30,"ax",%progbits
.balign 4
.global vocoder_code_5367
.hidden vocoder_code_5367
vocoder_code_5367:
.global asm_spread_spectral_peak
.type asm_spread_spectral_peak, %function
.thumb_func
asm_spread_spectral_peak:
    uxth	r1, r1
    subs	r3, r1, #1
    stmdb	sp!, {r4, r5, r6, r7, r8}
    uxth	r3, r3
    movw	r4, #34817
    movt	r4, #65535
    sxth	r5, r3
    cmp	r2, r4
    it	lt
    movlt	r2, r4
    cmp	r5, #0
    blt.n .Lcode_5397
    add.w	r5, r0, r5, lsl #1
    mov	r6, r2
    mov.w	r8, #67108864
.Lcode_5382:
    ldrsh.w	ip, [r5], #-2
    subs	r3, #1
    lsls	r7, r6, #16
    uxth	r3, r3
    qsub	r7, r7, r8
    asrs	r7, r7, #16
    cmp	ip, r6
    it	lt
    strhlt	r6, [r5, #2]
    cmp	r7, r4
    ite	ge
    movge	r6, r7
    movlt	r6, r4
    tst.w	r3, #32768
    beq.n .Lcode_5382
.Lcode_5397:
    adds	r1, #1
    uxth	r1, r1
    sxth	r3, r1
    cmp	r3, #55
    bgt.n .Lcode_5422
    movw	r6, #34817
    add.w	r0, r0, r3, lsl #1
    mov.w	r7, #67108864
    movt	r6, #65535
.Lcode_5406:
    adds	r1, #1
    ldrsh.w	r5, [r0], #2
    uxth	r1, r1
    lsls	r4, r2, #16
    sxth	r3, r1
    qsub	r4, r4, r7
    asrs	r4, r4, #16
    cmp	r2, r5
    it	gt
    strhgt.w	r2, [r0, #-2]
    cmp	r4, r6
    ite	ge
    movge	r2, r4
    movlt	r2, r6
    cmp	r3, #55
    ble.n .Lcode_5406
.Lcode_5422:
    ldmia.w	sp!, {r4, r5, r6, r7, r8}
    bx	lr
    .byte 0x00
    .byte 0xbf

.section .text.vocoder_rx_31,"ax",%progbits
.balign 4
.global vocoder_code_5424
.hidden vocoder_code_5424
vocoder_code_5424:
.global asm_classify_code
.type asm_classify_code, %function
.thumb_func
asm_classify_code:
    uxth	r0, r0
    subs	r3, r0, #5
    uxth	r3, r3
    cmp	r3, #117
    bls.n .Lcode_5448
    sub.w	r3, r0, #128
    uxth	r3, r3
    cmp	r3, #15
    bls.n .Lcode_5445
    sub.w	r3, r0, #144
    uxth	r3, r3
    cmp	r3, #15
    bls.n .Lcode_5451
    subs	r0, #160
    uxth	r0, r0
    cmp	r0, #3
    ite	hi
    movhi	r0, #4
    movls	r0, #3
    sxth	r0, r0
    bx	lr
.Lcode_5445:
    movs	r0, #1
    sxth	r0, r0
    bx	lr
.Lcode_5448:
    movs	r0, #0
    sxth	r0, r0
    bx	lr
.Lcode_5451:
    movs	r0, #2
    sxth	r0, r0
    bx	lr

.section .text.vocoder_rx_32,"ax",%progbits
.balign 4
.global vocoder_code_5454
.hidden vocoder_code_5454
vocoder_code_5454:
.global asm_lookup_partition_value
.type asm_lookup_partition_value, %function
.thumb_func
asm_lookup_partition_value:
    cbnz	r0, .Lcode_5466
    add.w	r1, r1, r1, lsl #2
    lsls	r0, r1, #10
    movw	r3, #65535
    qadd	r0, r3, r0
    lsrs	r0, r0, #16
    subs	r0, #1
    sxth	r0, r0
    bic.w	r0, r0, r0, asr #31
    uxth	r0, r0
    sxth	r0, r0
    bx	lr
.Lcode_5466:
    cmp	r0, #3
    bls.n .Lcode_5471
    movs	r0, #0
    sxth	r0, r0
    bx	lr
.Lcode_5471:
    subs	r1, #128
    add.w	r2, r2, r1, lsl #1
    ldr	r3, .Lliteral_5479
    ldrb	r0, [r3, r2]
    subs	r0, #1
    uxth	r0, r0
    sxth	r0, r0
    bx	lr
.Lliteral_5479:
    .word vocoder_data_20709

.section .text.vocoder_rx_34,"ax",%progbits
.balign 4
.global vocoder_code_5559
.hidden vocoder_code_5559
vocoder_code_5559:
.global asm_initialize_spectral_frame
.type asm_initialize_spectral_frame, %function
.thumb_func
asm_initialize_spectral_frame:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #12
    mov	r5, r1
    movs	r6, #0
    mov.w	sl, #3
    mov.w	r8, #1
    add.w	r7, r0, #16
    movw	r1, #45057
    str	r2, [sp, #4]
    strh.w	sl, [r0]
    movs	r2, #56
    strh	r6, [r0, #14]
    str.w	r8, [r0, #8]
    mov	r4, r0
    movt	r1, #65535
    mov	r0, r7
    mov	fp, r3
    ldrsh.w	r9, [sp, #48]
    bl vocoder_code_2535
    uxth	r2, r5
    subs	r3, r2, #5
    uxth	r3, r3
    cmp	r3, #117
    bls.n .Lcode_5669
    sub.w	r3, r2, #128
    uxth	r3, r3
    cmp	r3, #15
    bhi.w .Lcode_5714
.Lcode_5587:
    movs	r2, #0
    mov	r1, r5
    mov	r0, r8
    str	r3, [sp, #0]
    bl vocoder_code_5454
    mov	r1, r5
    mov	sl, r0
    movs	r2, #1
    mov	r0, r8
    bl vocoder_code_5454
    ldr	r3, [sp, #0]
    strh	r5, [r4, #2]
    uxth	r2, r0
    adds	r1, r2, #1
    mov	r6, r0
    sxth	r3, r3
    ldr	r0, .Lliteral_5750
    sxth	r1, r1
    ldrh.w	r0, [r0, r3, lsl #1]
    cmp	r1, #9
    it	lt
    movlt	r1, #9
    add.w	r2, r2, sl, lsl #8
    cmp	r1, #56
    it	ge
    movge	r1, #56
    strh	r2, [r4, #6]
    strh	r0, [r4, #12]
    strh	r1, [r4, #4]
    cmp.w	r9, #0
    beq.w .Lcode_5726
    ldr	r2, .Lliteral_5751
    ldrsh.w	r3, [r2, r3, lsl #1]
    ldr	r2, [sp, #4]
    mov.w	fp, fp, lsl #15
    mov.w	lr, r2, lsl #15
    mov.w	r5, #45056
    qadd	lr, fp, lr
    mov.w	lr, lr, asr #16
    mov.w	lr, lr, lsl #16
    lsls	r3, r3, #16
    movt	r5, #65535
    qsub	ip, lr, r3
    mov.w	ip, ip, asr #16
    cmp	ip, r5
    it	lt
    movlt	ip, r5
    movw	r8, #28671
    cmp	ip, r8
    it	ge
    movge	ip, r8
    add.w	r9, sl, #8
    uxth.w	ip, ip
    strh.w	ip, [r4, r9, lsl #1]
    qadd	r9, r3, lr
    mov.w	r9, r9, asr #16
    mov.w	r2, ip, lsl #16
    mov.w	fp, #1342177280
    mov	r0, r7
    qsub	r2, r2, fp
    mov	r1, sl
    asrs	r2, r2, #16
    cmp	r9, r5
    it	ge
    movge	r5, r9
    bl vocoder_code_5367
    cmp	r5, r8
    it	lt
    movlt	r8, r5
    add.w	r3, r6, #8
    uxth.w	r8, r8
    mov.w	r2, r8, lsl #16
    strh.w	r8, [r4, r3, lsl #1]
    qsub	fp, r2, fp
    mov	r0, r7
    mov.w	r2, fp, asr #16
    mov	r1, r6
    bl vocoder_code_5367
    mov.w	r9, #0
.Lcode_5666:
    sxth.w	r0, r9
    add	sp, #12
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_5669:
    mov	r1, r5
    mov	r2, r6
    mov	r0, r6
    bl vocoder_code_5454
    mov.w	r1, r0, lsl #24
    mov	r8, r0
    add.w	r1, r1, #16777216
    asrs	r1, r1, #16
    lsls	r0, r5, #20
    strh	r5, [r4, #2]
    strh.w	r8, [r4, #6]
    bl vocoder_code_6534
    mov.w	r3, #45056
    add.w	r5, r8, #1
    movt	r3, #65535
    sxth	r5, r5
    cmp	fp, r3
    it	ge
    movge	r3, fp
    movw	r2, #30719
    cmp	r5, #9
    it	lt
    movlt	r5, #9
    cmp	r3, r2
    it	ge
    movge	r3, r2
    add.w	r1, r8, #8
    cmp	r5, #56
    it	ge
    movge	r5, #56
    uxth	r3, r3
    strh	r0, [r4, #12]
    strh	r5, [r4, #4]
    strh.w	r3, [r4, r1, lsl #1]
    cmp.w	r9, #0
    beq.n .Lcode_5666
    lsls	r3, r3, #16
    mov.w	r2, #1342177280
    mov	r0, r7
    qsub	r2, r3, r2
    mov	r1, r8
    asrs	r2, r2, #16
    bl vocoder_code_5367
    mov	r9, r6
    b.n .Lcode_5666
.Lcode_5714:
    sub.w	r1, r2, #144
    uxth	r1, r1
    cmp	r1, #15
    bls.n .Lcode_5724
    subs	r2, #160
    uxth	r2, r2
    cmp	r2, sl
    bhi.n .Lcode_5747
    mov	r8, sl
    b.n .Lcode_5587
.Lcode_5724:
    mov.w	r8, #2
    b.n .Lcode_5587
.Lcode_5726:
    mov.w	r3, #45056
    ldr	r1, [sp, #4]
    movt	r3, #65535
    movw	r2, #28671
    add.w	sl, sl, #8
    cmp	r1, r3
    it	lt
    movlt	r1, r3
    adds	r6, #8
    cmp	fp, r3
    it	lt
    movlt	fp, r3
    cmp	r1, r2
    it	ge
    movge	r1, r2
    cmp	fp, r2
    it	lt
    movlt	r2, fp
    strh.w	r1, [r4, sl, lsl #1]
    strh.w	r2, [r4, r6, lsl #1]
    b.n .Lcode_5666
.Lcode_5747:
    strh	r6, [r4, #0]
    mov.w	r9, #4
    b.n .Lcode_5666
    .byte 0x00
    .byte 0xbf
.Lliteral_5750:
    .word vocoder_data_20710
.Lliteral_5751:
    .word vocoder_data_20708


.section .text.vocoder_rx_37,"ax",%progbits
.balign 4
.global vocoder_code_5780
.hidden vocoder_code_5780
vocoder_code_5780:
.global asm_generate_voicing_flags
.type asm_generate_voicing_flags, %function
.thumb_func
asm_generate_voicing_flags:
    ldrsh.w	r3, [r0]
    cmp	r3, #2
    push	{r4, lr}
    mov	r4, r0
    beq.n .Lcode_5815
    cmp	r3, #3
    beq.n .Lcode_5796
    cmp	r3, #1
    beq.n .Lcode_5790
.Lcode_5789:
    pop	{r4, pc}
.Lcode_5790:
    ldr	r1, [r0, #8]
    ldrsh.w	r2, [r0, #12]
    ldrsh.w	r3, [r0, #4]
    ldr.w	r0, [r0, #128]
    ldmia.w	sp!, {r4, lr}
    b.w vocoder_code_15563
.Lcode_5796:
    movs	r1, #0
    movs	r2, #56
    ldr.w	r0, [r0, #128]
    bl vocoder_code_2535
    ldrsh.w	r0, [r4, #2]
    bl vocoder_code_5424
    cmp	r0, #0
    beq.n .Lcode_5820
    blt.n .Lcode_5789
    cmp	r0, #3
    bgt.n .Lcode_5789
    ldrh	r2, [r4, #6]
    ldr.w	r3, [r4, #128]
    lsrs	r0, r2, #8
    uxtb	r1, r2
    movs	r2, #1
    strh.w	r2, [r3, r0, lsl #1]
    strh.w	r2, [r3, r1, lsl #1]
    b.n .Lcode_5789
.Lcode_5815:
    ldr.w	r0, [r0, #128]
    movs	r1, #0
    movs	r2, #56
    ldmia.w	sp!, {r4, lr}
    b.w vocoder_code_2535
.Lcode_5820:
    ldrsh.w	r2, [r4, #6]
    ldr.w	r3, [r4, #128]
    movs	r1, #1
    strh.w	r1, [r3, r2, lsl #1]
    pop	{r4, pc}

.section .text.vocoder_rx_38,"ax",%progbits
.balign 4
.global vocoder_code_5825
.hidden vocoder_code_5825
vocoder_code_5825:
.global asm_copy_frame_record
.type asm_copy_frame_record, %function
.thumb_func
asm_copy_frame_record:
    stmdb	sp!, {r4, r5, r6, r7, r8}
    mov	r3, r0
    ldrh	r4, [r1, #14]
    ldrh	r0, [r1, #0]
    ldr.w	r2, [r1, #132]
    ldrh.w	r8, [r1, #2]
    ldrh.w	ip, [r1, #4]
    ldrh	r7, [r1, #6]
    ldr	r6, [r1, #8]
    ldrh	r5, [r1, #12]
    strh	r0, [r3, #0]
    strh	r4, [r3, #14]
    str.w	r2, [r3, #132]
    movs	r4, #0
    adds	r1, #16
    add.w	r0, r3, #16
    movs	r2, #56
    strh.w	r8, [r3, #2]
    strh.w	ip, [r3, #4]
    strh	r7, [r3, #6]
    str	r6, [r3, #8]
    strh	r5, [r3, #12]
    str.w	r4, [r3, #128]
    ldmia.w	sp!, {r4, r5, r6, r7, r8}
    b.w vocoder_code_2496


.section .text.vocoder_rx_39,"ax",%progbits
.balign 4
.global vocoder_code_5850
.hidden vocoder_code_5850
vocoder_code_5850:
.global asm_synthesize_audio_frame
.type asm_synthesize_audio_frame, %function
.thumb_func
asm_synthesize_audio_frame:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub.w	sp, sp, #828
    ldr	r4, [sp, #864]
    str	r0, [sp, #8]
    ldrh.w	r5, [r4, #1982]
    str	r1, [sp, #24]
    add.w	r4, r4, #1136
    str	r4, [sp, #12]
    ldr	r0, [sp, #12]
    str	r2, [sp, #16]
    adds	r0, #4
    ands.w	r1, r5, #1
    mov	r4, r3
    str	r0, [sp, #12]
    bne.n .Lcode_5886
    ldr	r5, [sp, #8]
    ldrsh.w	r3, [r5]
    cmp	r3, #1
    beq.w .Lcode_6257
    ldr	r5, [sp, #864]
    movs	r2, #56
    add.w	r0, r5, #374
    bl vocoder_code_2535
    ldr	r5, [sp, #8]
    add.w	r6, r5, #16
.Lcode_5875:
    ldr	r5, [sp, #8]
    add.w	r7, r5, #132
    ldrsh.w	r2, [r5, #4]
    mov	r0, r6
    mov	r1, r7
    bl vocoder_code_15518
    ldrsh.w	r3, [r5]
    cmp	r3, #1
    beq.w .Lcode_6278
.Lcode_5884:
    cmp	r3, #2
    beq.w .Lcode_6264
.Lcode_5886:
    ldr	r0, [sp, #8]
    add	r3, sp, #372
    str.w	r3, [r0, #128]
    bl vocoder_code_5780
    ldr	r5, [sp, #864]
    ldr	r0, [sp, #12]
    add	r3, sp, #484
    str.w	r3, [r5, #1268]
    bl vocoder_code_5780
    ldr	r5, [sp, #8]
    ldrsh.w	r0, [r5, #2]
    cmp	r0, #255
    beq.n .Lcode_5992
    ldr	r4, [sp, #864]
    ldrsh.w	r3, [r4, #1956]
    cmp	r3, #1
    beq.w .Lcode_6183
    bl vocoder_code_5424
    ldr	r4, [sp, #8]
    uxtb	r5, r0
    ldrsh.w	r1, [r4, #2]
    movs	r2, #0
    mov	r0, r5
    bl vocoder_code_5454
    mov	r4, r0
    ldr	r0, [sp, #8]
    movs	r2, #1
    ldrsh.w	r1, [r0, #2]
    mov	r0, r5
    bl vocoder_code_5454
    ldr	r5, [sp, #8]
    ldrsh.w	r6, [r5, #4]
    cmp	r6, #0
    ble.n .Lcode_5935
    mov	r2, r5
    movs	r3, #0
    movw	r5, #1310
.Lcode_5923:
    cmp	r4, r3
    beq.n .Lcode_5931
    cmp	r0, r3
    beq.n .Lcode_5931
    ldrh	r1, [r2, #16]
    smulbb	r1, r1, r5
    asrs	r1, r1, #15
    strh	r1, [r2, #16]
.Lcode_5931:
    adds	r3, #1
    adds	r2, #2
    cmp	r3, r6
    blt.n .Lcode_5923
.Lcode_5935:
    ldr	r5, [sp, #8]
    ldrh	r2, [r5, #2]
    uxth	r3, r2
    sub.w	r1, r3, #128
    uxth	r1, r1
    cmp	r1, #31
    bls.w .Lcode_6236
    subs	r3, #20
    uxth	r3, r3
    cmp	r3, #2
    bls.w .Lcode_6227
.Lcode_5946:
    ldr	r5, [sp, #16]
    lsls	r2, r5, #1
    movs	r1, #0
    sxth	r2, r2
    add	r0, sp, #36
    bl vocoder_code_2535
    ldr	r4, [sp, #864]
    ldr	r1, [sp, #16]
    ldr	r3, [sp, #8]
    add.w	r2, r4, #1600
    ldrsh.w	r4, [r4, #1986]
    adds	r2, #12
    add	r0, sp, #36
    str	r4, [sp, #0]
    bl vocoder_code_15641
    ldr	r5, [sp, #864]
    ldr	r1, [sp, #16]
    ldrsh.w	r4, [r5, #1986]
    ldr	r5, [sp, #12]
    ldr	r3, [sp, #8]
    str	r5, [sp, #0]
    ldr	r5, [sp, #864]
    str	r4, [sp, #4]
    add.w	r2, r5, #24
    add	r0, sp, #36
    bl vocoder_code_16787
    add	r0, sp, #36
    mov	r1, r0
    ldr	r2, [sp, #864]
    ldr	r3, [sp, #16]
    bl vocoder_code_4566
    ldr	r4, [sp, #16]
    ldr	r0, [sp, #24]
    str	r4, [sp, #0]
    movs	r1, #15
    movs	r3, #17
    add	r2, sp, #36
    bl vocoder_code_7720
    ldr	r5, [sp, #8]
    ldr	r0, [sp, #12]
    movs	r3, #0
    str.w	r3, [r5, #128]
    mov	r1, r5
    bl vocoder_code_5825
    add.w	sp, sp, #828
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_5992:
    cmp	r4, #0
    beq.w .Lcode_6270
    ldr	r5, [sp, #864]
    ldrh.w	r0, [r5, #1986]
.Lcode_5996:
    sxth	r0, r0
    movw	r3, #13107
    cmp	r0, r3
    ite	lt
    movlt	r4, r0
    movge	r4, r3
    lsls	r0, r4, #16
    mov	r2, r0
    movw	r3, #65532
    cmp	r2, #0
    it	lt
    mvnlt	r2, r2
    itt	ne
    clzne	r2, r2
    subne	r2, #1
    sxth	r1, r2
    lsls.w	r2, r0, r1
    movt	r3, #65535
    asrs	r2, r2, #16
    subs	r3, r3, r1
    add	r5, sp, #820
    movs	r1, #1
    mov.w	r0, #1073741824
    str	r5, [sp, #0]
    bl vocoder_code_6721
    ldr	r5, [sp, #8]
    mov	r7, r0
    ldrh	r0, [r5, #12]
    sxth	r2, r0
    cmp	r2, r4
    mov	sl, r2
    bge.n .Lcode_6066
    ldr	r5, [sp, #820]
    ldr	r3, [sp, #8]
    uxth	r7, r7
    subs	r5, #4
    lsls	r6, r2, #16
    add.w	r1, r3, #16
    b.n .Lcode_6047
.Lcode_6035:
    lsls	r3, r3, #1
    lsls	r0, r3
.Lcode_6037:
    ldrh.w	lr, [r1]
    lsls	r2, r2, #16
    smultb	r3, r0, lr
    qadd	r2, r6, r2
    lsrs	r0, r2, #16
    sxth	r2, r0
    sbfx	r3, r3, #15, #16
    cmp	r2, r4
    strh.w	r3, [r1], #2
    bge.n .Lcode_6066
.Lcode_6047:
    smulbb	r0, r0, r7
    lsls	r0, r0, #1
    mov	r3, r0
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsls	r0, r3
    smultt	r0, r0, r0
    lsls	r0, r0, #1
    subs	r3, r5, r3
    bpl.n .Lcode_6035
    rsb	r3, r3, r3, lsl #31
    lsls	r3, r3, #1
    asrs	r0, r3
    b.n .Lcode_6037
.Lcode_6066:
    ldr	r4, [sp, #8]
    ldrh	r4, [r4, #4]
    sxth	r5, r4
    cmp	r5, #0
    str	r4, [sp, #28]
    str	r5, [sp, #20]
    ble.n .Lcode_6086
    ldr	r4, [sp, #8]
    lsls	r5, r5, #1
    ldrh.w	r6, [r4, #132]
    add.w	r1, r4, #14
    movs	r2, #0
    add	r3, sp, #596
    add	r4, sp, #708
.Lcode_6080:
    ldrh.w	r0, [r1, #2]!
    strh	r6, [r4, r2]
    strh	r0, [r3, r2]
    adds	r2, #2
    cmp	r2, r5
    bne.n .Lcode_6080
.Lcode_6086:
    ldr	r5, [sp, #20]
    mul.w	lr, sl, r5
    ubfx	lr, lr, #3, #16
    sxth.w	ip, lr
    movw	r8, #19661
    cmp	ip, r8
    ble.n .Lcode_6137
    ldr	r4, [sp, #8]
    ldr	r0, [sp, #20]
    ldrh.w	fp, [r4, #132]
    lsls	r5, r5, #1
    add.w	fp, fp, #1
    add	r3, sp, #596
    rsb	r6, r0, r0, lsl #31
    add	r1, sp, #708
    uxth.w	fp, fp
    mov.w	sl, sl, lsl #13
    adds	r2, r1, r5
    adds	r3, r3, r5
    movw	r9, #27307
    lsls	r6, r6, #1
.Lcode_6107:
    adds	r1, r3, r6
    adds	r1, r1, r5
    smulbb	lr, lr, r9
    ldrh.w	r0, [r1, #-2]
    ubfx	lr, lr, #15, #16
    smulbb	r4, lr, r0
    lsls	r4, r4, #1
    mov	r0, r4
    rsb	lr, sl, ip, lsl #16
    cmp	r0, #0
    it	lt
    mvnlt	r0, r0
    itt	ne
    clzne	r0, r0
    subne	r0, #1
    uxth	r0, r0
    sxth.w	ip, r0
    adds	r7, r2, r6
    mov.w	lr, lr, lsr #16
    lsl.w	r4, r4, ip
    adds	r7, r7, r5
    sxth.w	ip, lr
    asrs	r4, r4, #16
    rsb	r0, r0, fp
    subs	r2, #2
    subs	r3, #2
    cmp	ip, r8
    strh.w	r0, [r7, #-2]
    strh.w	r4, [r1, #-2]
    bgt.n .Lcode_6107
.Lcode_6137:
    ldr	r2, [sp, #20]
    ldrsh.w	r0, [sp, #708]
    cmp	r2, #1
    mov	r1, r0
    ble.w .Lcode_6298
    ldr	r5, [sp, #28]
    add	r2, sp, #704
    movs	r4, #2
    adds	r2, #2
    add	r3, sp, #708
    smlabb	r4, r5, r4, r2
.Lcode_6148:
    ldrsh.w	r2, [r3, #2]!
    cmp	r1, r2
    it	lt
    movlt	r1, r2
    cmp	r3, r4
    bne.n .Lcode_6148
.Lcode_6154:
    ldr	r2, [sp, #20]
    ldr	r5, [sp, #8]
    lsls	r6, r2, #1
    add.w	r4, r5, #14
    movs	r2, #0
    add	r3, sp, #596
    b.n .Lcode_6169
.Lcode_6161:
    lsls.w	r0, r5, r7
    adds	r2, #2
    asrs	r0, r0, #16
    cmp	r2, r6
    strh.w	r0, [r4, #2]!
    beq.n .Lcode_6180
.Lcode_6167:
    add	r0, sp, #708
    ldrsh	r0, [r0, r2]
.Lcode_6169:
    ldrsh	r5, [r3, r2]
    lsls	r5, r5, #16
    subs	r7, r0, r1
    bpl.n .Lcode_6161
    subs	r0, r1, r0
    asrs.w	r0, r5, r0
    adds	r2, #2
    asrs	r0, r0, #16
    cmp	r2, r6
    strh.w	r0, [r4, #2]!
    bne.n .Lcode_6167
.Lcode_6180:
    ldr	r4, [sp, #8]
    str.w	r1, [r4, #132]
    b.n .Lcode_5946
.Lcode_6183:
    bl vocoder_code_5424
    cmp	r0, #0
    bne.w .Lcode_5946
    ldrsh.w	r3, [r4, #1140]
    cmp	r3, #3
    bne.w .Lcode_5946
    ldrsh.w	r0, [r4, #1142]
    bl vocoder_code_5424
    cmp	r0, #0
    bne.w .Lcode_5946
    ldrsh.w	r1, [r4, #1146]
    ldrsh.w	r3, [r5, #6]
    subs	r3, r3, r1
    cmp	r3, #0
    it	lt
    neglt	r3, r3
    cmp	r3, #1
    ble.n .Lcode_6293
    ldr	r5, [sp, #8]
    ldrsh.w	r2, [r5, #2]
    adds	r3, r1, #1
.Lcode_6204:
    ldr	r4, [sp, #864]
    ldrsh.w	r1, [r4, #1142]
    add.w	r0, r1, r1, lsr #31
    asrs	r0, r0, #1
    mla	r0, r2, r3, r0
    bl vocoder_code_6588
    ldr	r5, [sp, #8]
    uxth	r2, r0
    sxth	r3, r2
    ldrsh.w	r1, [r5, #2]
    lsls	r3, r3, #4
    cmp	r3, r1
    bgt.n .Lcode_6222
.Lcode_6217:
    adds	r3, #16
    adds	r2, #1
    cmp	r1, r3
    uxth	r2, r2
    bge.n .Lcode_6217
.Lcode_6222:
    subs	r2, #1
    sxth	r1, r2
.Lcode_6224:
    ldr	r0, [sp, #8]
    bl vocoder_code_6459
    b.n .Lcode_5946
.Lcode_6227:
    ldr	r4, [sp, #8]
    ldrsh.w	r1, [r4, #6]
    adds	r1, #1
    lsls	r1, r1, #8
    lsls	r0, r2, #20
    sxth	r1, r1
    bl vocoder_code_6534
    strh	r0, [r4, #12]
    b.n .Lcode_5946
.Lcode_6236:
    add.w	r5, r0, #8
    ldr	r0, [sp, #8]
    adds	r4, #8
    ldrh.w	r3, [r0, r5, lsl #1]
    ldrh.w	r0, [r0, r4, lsl #1]
    smulbb	r3, r3, r3
    movs	r1, #8
    movs	r2, #4
    smlabb	r0, r0, r0, r3
    bl vocoder_code_6971
    ldr	r1, [sp, #8]
    ldrh	r2, [r1, #2]
    uxth	r3, r0
    strh.w	r3, [r1, r5, lsl #1]
    strh.w	r3, [r1, r4, lsl #1]
    uxth	r3, r2
    subs	r3, #20
    uxth	r3, r3
    cmp	r3, #2
    bhi.w .Lcode_5946
    b.n .Lcode_6227
.Lcode_6257:
    add.w	r6, r5, #16
    ldrsh.w	r2, [r5, #4]
    ldr	r5, [sp, #864]
    mov	r1, r6
    add.w	r0, r5, #374
    bl vocoder_code_17392
    b.n .Lcode_5875
.Lcode_6264:
    ldr	r5, [sp, #864]
    ldr	r1, [sp, #8]
    add.w	r0, r5, #1264
    adds	r0, #12
    bl vocoder_code_5825
    b.n .Lcode_5886
.Lcode_6270:
    ldr	r4, [sp, #864]
    ldr	r5, [sp, #8]
    ldrsh.w	r0, [r4, #1986]
    ldrsh.w	r1, [r5, #12]
    ldr	r2, [r5, #8]
    bl vocoder_code_15613
    strh.w	r0, [r4, #1986]
    b.n .Lcode_5996
.Lcode_6278:
    ldr	r5, [sp, #864]
    add.w	r3, r5, #1984
    ldr	r5, [sp, #8]
    ldrsh.w	r1, [r5, #12]
    ldrsh.w	r5, [r5, #4]
    str	r1, [sp, #0]
    adds	r2, r3, #4
    mov	r0, r6
    adds	r3, #6
    mov	r1, r7
    str	r5, [sp, #4]
    bl vocoder_code_17475
    ldr	r5, [sp, #8]
    ldrsh.w	r3, [r5]
    b.n .Lcode_5884
.Lcode_6293:
    ldrsh.w	r2, [r5, #2]
    adds	r3, r1, #1
    cmp.w	r2, r3, lsl #4
    bge.n .Lcode_6204
    b.n .Lcode_6224
.Lcode_6298:
    ldr	r3, [sp, #20]
    cmp	r3, #0
    ble.w .Lcode_6180
    b.n .Lcode_6154
    .byte 0x00
    .byte 0xbf

.section .text.vocoder_rx_40,"ax",%progbits
.balign 4
.global vocoder_code_6302
.hidden vocoder_code_6302
vocoder_code_6302:
.global asm_decode_frame_record
.type asm_decode_frame_record, %function
.thumb_func
asm_decode_frame_record:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #60
    mov	r9, r1
    ldr	r4, [sp, #104]
    ldr	r5, [sp, #96]
    ldrsh.w	r6, [r4, #1610]
    ldrsh.w	r7, [r4, #1982]
    ldr.w	fp, [sp, #100]
    orr.w	r8, r2, r6
    lsls	r7, r7, #1
    tst.w	r8, #12
    strh.w	r7, [r4, #1982]
    mov	sl, r3
    beq.n .Lcode_6380
    movs	r7, #0
    add.w	r6, r4, #1408
    mov	r9, r7
    adds	r6, #4
.Lcode_6320:
    ldrh	r3, [r5, #2]
    add	r0, sp, #56
    movs	r2, #0
    add.w	r1, r4, #1992
    strh.w	r3, [r0, #-2]!
    bl vocoder_code_5480
    ands.w	sl, r8, #16384
    beq.n .Lcode_6349
    ldrh.w	r1, [r4, #1952]
    ldrh.w	r2, [r4, #1954]
    strh.w	r1, [sp, #54]
.Lcode_6331:
    sxth	r2, r2
    sxth	r1, r1
    mov	r3, r2
    movs	r7, #0
    mov	r0, r5
    str	r7, [sp, #0]
    bl vocoder_code_5559
    mov	r0, fp
    mov	r1, r5
    bl vocoder_code_5825
    movw	r7, #32770
.Lcode_6342:
    movs	r3, #0
    strh.w	r3, [r4, #1984]
    mov	r0, r6
    bl vocoder_code_14108
.Lcode_6346:
    mov	r0, r7
    add	sp, #60
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_6349:
    cmp	r0, #0
    bne.n .Lcode_6416
    tst.w	r8, #8
    bne.n .Lcode_6366
    tst.w	r8, #4
    beq.n .Lcode_6364
.Lcode_6355:
    ldrh.w	r3, [r4, #1982]
    and.w	r3, r3, #14
    cmp	r3, #14
    beq.n .Lcode_6366
    add.w	r1, r4, #1136
    adds	r1, #4
    mov	r0, r5
    bl vocoder_code_5825
    b.n .Lcode_6370
.Lcode_6364:
    cmp.w	r9, #0
    beq.n .Lcode_6437
.Lcode_6366:
    add.w	r1, r4, #1264
    adds	r1, #12
    mov	r0, r5
    bl vocoder_code_5825
.Lcode_6370:
    mov	r0, fp
    mov	r1, r5
    bl vocoder_code_5825
    ldrh.w	r3, [r4, #1982]
    orr.w	r3, r3, #1
    strh.w	r3, [r4, #1982]
    mov	r0, r6
    bl vocoder_code_14108
    movs	r7, #32
    b.n .Lcode_6346
.Lcode_6380:
    ldrsh.w	r7, [r4, #1988]
    str	r7, [sp, #44]
    ldrsh.w	r7, [r4, #1990]
    str	r0, [sp, #4]
    str	r7, [sp, #28]
    add.w	r7, r4, #1952
    adds	r7, #4
    add.w	r6, r4, #1408
    movs	r1, #68
    str	r7, [sp, #20]
    ldr	r7, [sp, #44]
    str.w	r9, [sp, #8]
    adds	r6, #4
    add.w	ip, r4, #1560
    and.w	lr, r8, #128
    mov	r0, r5
    mov	r2, fp
    mov	r3, r1
    str	r6, [sp, #0]
    str.w	sl, [sp, #12]
    str.w	ip, [sp, #16]
    str	r7, [sp, #24]
    str.w	lr, [sp, #32]
    bl vocoder_code_5212
    cbz	r0, .Lcode_6413
    and.w	r9, r0, #8
    cmp.w	r9, #0
    ite	ne
    movne	r7, #0
    moveq	r7, #1
    it	ne
    movne.w	r9, #1
    b.n .Lcode_6320
.Lcode_6413:
    mov	r7, r0
    mov	r9, r0
    b.n .Lcode_6320
.Lcode_6416:
    ldrsh.w	r0, [sp, #54]
    bl vocoder_code_5424
    ldrsh.w	r1, [sp, #54]
    mov	r2, sl
    uxtb	r0, r0
    bl vocoder_code_5454
    ldrsh.w	r2, [sp, #54]
    add.w	r3, r0, #8
    mov	r0, r2
    ldrsh.w	r7, [r5, r3, lsl #1]
    bl vocoder_code_5424
    ldrsh.w	r1, [sp, #54]
    movs	r2, #1
    uxtb	r0, r0
    bl vocoder_code_5454
    adds	r0, #8
    ldrh.w	r1, [sp, #54]
    ldrsh.w	r2, [r5, r0, lsl #1]
    adds	r7, r7, r2
    ubfx	r2, r7, #1, #16
    b.n .Lcode_6331
.Lcode_6437:
    cmp	r7, #0
    bne.n .Lcode_6355
    ldrsh.w	r3, [r5]
    cmp	r3, #2
    beq.w .Lcode_6342
    cmp	r3, #3
    beq.n .Lcode_6455
    cmp	r3, #1
    bne.w .Lcode_6355
    ldrsh.w	r3, [r4, #1956]
    cbnz	r3, .Lcode_6457
    ldrsh.w	r7, [r5, #2]
    movw	r3, #32770
    cmp	r7, #255
    ite	ne
    movne	r7, r3
    moveq	r7, #2
    b.n .Lcode_6342
.Lcode_6455:
    movw	r7, #32770
    b.n .Lcode_6342
.Lcode_6457:
    movs	r7, #2
    b.n .Lcode_6342


.section .text.vocoder_rx_41,"ax",%progbits
.balign 4
.global vocoder_code_6459
.hidden vocoder_code_6459
vocoder_code_6459:
.global asm_move_active_harmonic
.type asm_move_active_harmonic, %function
.thumb_func
asm_move_active_harmonic:
    push	{r4, r5, r6, r7, lr}
    mov	r4, r0
    ldrsh.w	r0, [r0, #6]
    ldr.w	r3, [r4, #128]
    add.w	r6, r0, #8
    uxth	r1, r1
    sxth	r2, r1
    ldrh.w	lr, [r4, r6, lsl #1]
    movs	r5, #0
    add.w	r7, r2, #8
    strh.w	r5, [r4, r6, lsl #1]
    strh.w	r5, [r3, r0, lsl #1]
    strh	r1, [r4, #6]
    movs	r1, #1
    strh.w	lr, [r4, r7, lsl #1]
    strh.w	r1, [r3, r2, lsl #1]
    ldrsh.w	r2, [r4, #6]
    ldrsh.w	r0, [r4, #2]
    adds	r2, r2, r1
    lsls	r2, r2, #16
    lsls	r0, r0, #16
    mov	r3, r2
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    sub	sp, #20
    lsls	r2, r3
    mov	r1, r0
    cmp	r1, #0
    it	lt
    mvnlt	r1, r1
    itt	ne
    clzne	r1, r1
    subne	r1, #1
    sxth	r1, r1
    lsls	r0, r1
    asrs	r2, r2, #16
    rsb	r3, r3, #23
    rsb	r1, r1, #15
    add	r5, sp, #12
    str	r5, [sp, #0]
    bl vocoder_code_6721
    ldr	r3, [sp, #12]
    lsls	r0, r0, #16
    adds	r2, r3, #4
    blt.n .Lcode_6529
    adds	r3, #4
    lsls	r0, r3
.Lcode_6511:
    ldrh	r2, [r4, #6]
    ldrsh.w	r3, [r4, #4]
    sxth	r1, r2
    asrs	r0, r0, #16
    cmp	r3, r1
    strh	r0, [r4, #12]
    bgt.n .Lcode_6520
    adds	r3, r2, #1
    sxth	r3, r3
.Lcode_6520:
    cmp	r3, #9
    it	lt
    movlt	r3, #9
    cmp	r3, #56
    it	ge
    movge	r3, #56
    strh	r3, [r4, #4]
    add	sp, #20
    pop	{r4, r5, r6, r7, pc}
.Lcode_6529:
    movw	r2, #65532
    movt	r2, #65535
    subs	r2, r2, r3
    asrs	r0, r2
    b.n .Lcode_6511


.section .text.vocoder_rx_55,"ax",%progbits
.balign 4
.global vocoder_code_6971
.hidden vocoder_code_6971
vocoder_code_6971:
.global asm_sqrt_scaled
.type asm_sqrt_scaled, %function
.thumb_func
asm_sqrt_scaled:
    push	{r4, lr}
    sub	sp, #8
    add.w	ip, sp, #8
    mov	r4, r2
    str.w	r1, [ip, #-4]!
    mov	r1, ip
    bl vocoder_code_6932
    ldr	r3, [sp, #4]
    subs	r2, r3, r4
    itet	mi
    rsbmi	r4, r3, r4
    lslpl	r0, r2
    asrmi	r0, r4
    add.w	r0, r0, #32768
    asrs	r0, r0, #16
    add	sp, #8
    pop	{r4, pc}


.section .text.vocoder_rx_73,"ax",%progbits
.balign 4
.global vocoder_code_7521
.hidden vocoder_code_7521
vocoder_code_7521:
.global asm_multiply_i16_between_exponents
.type asm_multiply_i16_between_exponents, %function
.thumb_func
asm_multiply_i16_between_exponents:
    push	{r4, r5}
    ldr	r4, [sp, #12]
    ldr	r5, [sp, #8]
    subs	r4, r5, r4
    subs	r4, #15
    bmi.n .Lcode_7536
    add.w	r2, r0, r2, lsl #1
.Lcode_7528:
    ldrsh.w	r5, [r1], #2
    mul.w	r5, r3, r5
    lsls	r5, r4
    strh.w	r5, [r0], #2
    cmp	r0, r2
    bne.n .Lcode_7528
.Lcode_7534:
    pop	{r4, r5}
    bx	lr
.Lcode_7536:
    negs	r4, r4
    add.w	r2, r0, r2, lsl #1
.Lcode_7538:
    ldrsh.w	r5, [r1], #2
    mul.w	r5, r3, r5
    asrs	r5, r4
    strh.w	r5, [r0], #2
    cmp	r0, r2
    bne.n .Lcode_7538
    b.n .Lcode_7534

.section .text.vocoder_rx_118,"ax",%progbits
.balign 4
.global vocoder_code_14108
.hidden vocoder_code_14108
vocoder_code_14108:
.global asm_increment_counter_100
.type asm_increment_counter_100, %function
.thumb_func
asm_increment_counter_100:
    ldrh.w	r3, [r0, #134]
    sxth	r2, r3
    cmp	r2, #99
    bgt.n .Lcode_14114
    adds	r3, #1
    strh.w	r3, [r0, #134]
.Lcode_14114:
    bx	lr
    .byte 0x00
    .byte 0xbf

.section .text.vocoder_rx_123,"ax",%progbits
.balign 4
.global vocoder_code_14929
.hidden vocoder_code_14929
vocoder_code_14929:
.global asm_decode_spread_codeword
.type asm_decode_spread_codeword, %function
.thumb_func
asm_decode_spread_codeword:
    push	{r4}
    cmp	r0, #1
    ldr	r4, [sp, #4]
    beq.n .Lcode_14938
    movs	r3, #0
    str	r3, [r2, #0]
    str	r3, [r1, #0]
    pop	{r4}
    bx	lr
.Lcode_14938:
    rsb	r4, r4, #7
    lsls	r3, r4
    ldr	r0, .Lliteral_14948
    sxth	r3, r3
    ldr.w	r3, [r0, r3, lsl #2]
    mov	r0, r1
    mov	r1, r2
    mov	r2, r3
    pop	{r4}
    b.w vocoder_code_19386
.Lliteral_14948:
    .word vocoder_data_20695

.section .text.vocoder_rx_124,"ax",%progbits
.balign 4
.global vocoder_code_14949
.hidden vocoder_code_14949
vocoder_code_14949:
.global asm_adjust_harmonic_samples
.type asm_adjust_harmonic_samples, %function
.thumb_func
asm_adjust_harmonic_samples:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    mov	r4, r0
    sub	sp, #156
    ldrsh.w	r0, [r0, #4]
    add.w	lr, r4, #16
    add	r7, sp, #144
    mov	r6, r1
    mov	r9, r2
    mov	r1, lr
    mov	r2, r0
    mov	r0, r7
    mov	r8, r3
    bl vocoder_code_7556
    ldrsh.w	r2, [r4, #4]
    str	r7, [sp, #0]
    lsls	r2, r2, #16
    ldr	r7, [sp, #144]
    mov	r3, r2
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    adds	r7, #8
    lsls	r2, r3
    asrs	r2, r2, #16
    rsb	r3, r3, #15
    mov	r1, r7
    str	r7, [sp, #144]
    bl vocoder_code_6721
    movs	r2, #4
    lsls	r0, r0, #16
    ldr	r1, [sp, #144]
    bl vocoder_code_6971
    ldrh.w	r2, [r6, #130]
    movs	r3, #4
    uxth	r0, r0
    lsls	r7, r0, #16
    str	r3, [sp, #144]
    mov.w	r1, #134217728
    lsls	r3, r2, #16
    qsub	r1, r3, r1
    qsub	r1, r1, r7
    cmp	r1, #0
    blt.w .Lcode_15150
    sub.w	r3, r3, #1310720
    asrs	r3, r3, #16
    strh.w	r3, [r6, #130]
.Lcode_14999:
    cmp.w	r9, #3
    bgt.n .Lcode_15003
    cmp.w	r8, #1
    ble.w .Lcode_15145
.Lcode_15003:
    mov	r0, r6
    add	r2, sp, #152
    ldrh.w	r1, [r0, #16]!
    movs	r3, #0
    strh.w	r1, [r2, #-128]!
    b.n .Lcode_15010
.Lcode_15009:
    ldrh.w	r1, [r0, #2]!
.Lcode_15010:
    adds	r3, #1
    cmp	r3, #56
    strh.w	r1, [r2, #2]!
    bne.n .Lcode_15009
    ldrsh.w	r2, [r6, #12]
    ldrsh.w	r0, [r4, #12]
    ldrh.w	r7, [sp, #136]
    ldrsh.w	lr, [r6, #130]
    strh.w	r7, [sp, #138]
    lsls	r2, r2, #16
    lsls	r0, r0, #16
    movw	r1, #65532
    mov	r6, r2
    mov	r3, r1
    cmp	r6, #0
    it	lt
    mvnlt	r6, r6
    itt	ne
    clzne	r6, r6
    subne	r6, #1
    movt	r1, #65535
    sxth.w	ip, r6
    mov	r6, r0
    cmp	r6, #0
    it	lt
    mvnlt	r6, r6
    itt	ne
    clzne	r6, r6
    subne	r6, #1
    sxth	r6, r6
    lsls	r0, r6
    subs	r1, r1, r6
    lsl.w	r2, r2, ip
    movt	r3, #65535
    mov.w	r6, r8, lsl #27
    asrs	r2, r2, #16
    rsb	r3, ip, r3
    asrs	r6, r6, #1
    strh.w	r7, [sp, #140]
    strh.w	r7, [sp, #142]
    add	r7, sp, #148
    mov.w	lr, lr, lsl #16
    rsb	r6, r6, #738197504
    str	r7, [sp, #0]
    qadd	r6, lr, r6
    bl vocoder_code_6721
    ldrsh.w	sl, [r4, #4]
    movs	r2, #0
    movw	r1, #34817
    sxth	r0, r0
    asrs	r6, r6, #16
    movt	r1, #65535
    adds	r3, r2, #1
    mov	r9, r0
    lsls	r6, r6, #16
    str	r1, [sp, #16]
    cmp	r2, sl
    mul.w	r1, r9, r3
    str	r6, [sp, #12]
    mov.w	fp, #268435456
    str.w	sl, [sp, #20]
    mov.w	r1, r1, lsl #1
    bge.n .Lcode_15145
.Lcode_15073:
    ldr	r0, [sp, #148]
    ldrh	r6, [r4, #12]
    cmp	r0, #0
    blt.n .Lcode_15147
    lsls	r1, r0
.Lcode_15078:
    asrs	r0, r1, #16
    add	r7, sp, #152
    ubfx	r1, r1, #1, #15
    add.w	ip, r7, r0, lsl #1
    mov.w	r8, r1, lsl #16
    ldrh.w	r7, [ip, #-128]
    ldrsh.w	r0, [ip, #-126]
    rsb	r8, r8, #2130706432
    adds	r2, #8
    add.w	r8, r8, #16711680
    smultb	r8, r8, r7
    mla	r8, r0, r1, r8
    add.w	ip, r3, r3, lsl #1
    ldrsh.w	r7, [r4, r2, lsl #1]
    mov.w	ip, ip, lsl #17
    sbfx	r8, r8, #15, #16
    lsls	r7, r7, #16
    smultb	ip, ip, r6
    mov.w	r8, r8, lsl #16
    qsub	r1, r7, r8
    qsub	r1, r1, fp
    cmp	r1, #0
    sbfx	ip, ip, #8, #16
    ble.n .Lcode_15139
    ldr	r0, [sp, #12]
    cmp.w	ip, #4096
    it	ge
    movge.w	ip, #4096
    sub.w	ip, r0, ip, lsl #16
    qsub	ip, r7, ip
    cmp.w	ip, #0
    mov.w	r6, ip, asr #16
    ble.n .Lcode_15139
    cmp.w	r6, #1024
    it	ge
    movge.w	r6, #1024
    ldr	r1, [sp, #8]
    qsub	r7, r8, r7
    asrs	r7, r7, #16
    smull	r5, r1, r7, r6
    qadd	r0, r5, r5
    str	r1, [sp, #8]
    lsls	r1, r0, #4
    cmp.w	r0, r1, asr #4
    itt	ne
    mvnne.w	r6, #2147483648
    eorne.w	r1, r6, r0, asr #31
    ldrsh.w	r0, [r4, r2, lsl #1]
    ldr	r7, [sp, #16]
    ldr.w	sl, [sp, #20]
    lsls	r0, r0, #16
    qadd	r1, r0, r1
    asrs	r1, r1, #16
    cmp	r1, r7
    it	lt
    movlt	r1, r7
    movw	r0, #30719
    cmp	r1, r0
    it	ge
    movge	r1, r0
    strh.w	r1, [r4, r2, lsl #1]
.Lcode_15139:
    mov	r2, r3
    adds	r3, r2, #1
    mul.w	r1, r9, r3
    cmp	r2, sl
    mov.w	r1, r1, lsl #1
    blt.n .Lcode_15073
.Lcode_15145:
    add	sp, #156
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_15147:
    negs	r0, r0
    asrs	r1, r0
    b.n .Lcode_15078
.Lcode_15150:
    movw	r3, #32112
    smulbb	r3, r2, r3
    movw	r2, #20971
    smulbb	r0, r0, r2
    lsls	r3, r3, #1
    add.w	r3, r3, r0, asr #4
    asrs	r3, r3, #16
    strh.w	r3, [r6, #130]
    b.n .Lcode_14999

.section .text.vocoder_rx_125,"ax",%progbits
.balign 4
.global vocoder_code_15159
.hidden vocoder_code_15159
vocoder_code_15159:
.global asm_resample_step_q16
.type asm_resample_step_q16, %function
.thumb_func
asm_resample_step_q16:
    cmp	r0, r1
    push	{r3, r4, r5, lr}
    beq.n .Lcode_15201
    lsls	r1, r1, #16
    mov	r3, r1
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsls	r1, r3
    movw	r4, #65532
    movt	r4, #65535
    asrs	r1, r1, #16
    subs	r4, r4, r3
    lsls	r3, r0, #16
    cmp	r0, r1
    bge.n .Lcode_15189
    mov	r0, r3
    bl vocoder_code_6534
    mvn.w	r5, #3
    subs	r3, r5, r4
    lsls	r0, r0, #16
    cmp	r3, #14
    ble.n .Lcode_15197
.Lcode_15186:
    subs	r3, #15
    lsls	r0, r3
    pop	{r3, r4, r5, pc}
.Lcode_15189:
    asrs	r3, r3, #1
    mov	r0, r3
    bl vocoder_code_6534
    mvn.w	r5, #2
    subs	r3, r5, r4
    lsls	r0, r0, #16
    cmp	r3, #14
    bgt.n .Lcode_15186
.Lcode_15197:
    subs	r4, r4, r5
    adds	r4, #15
    asrs	r0, r4
    pop	{r3, r4, r5, pc}
.Lcode_15201:
    mov.w	r0, #65536
    pop	{r3, r4, r5, pc}

.section .text.vocoder_rx_126,"ax",%progbits
.balign 4
.global vocoder_code_15203
.hidden vocoder_code_15203
vocoder_code_15203:
.global asm_resample_spectrum
.type asm_resample_spectrum, %function
.thumb_func
asm_resample_spectrum:
    push	{r4, r5, r6, r7, lr}
    ldrh	r5, [r3, #0]
    sub	sp, #124
    mov	r4, r0
    strh.w	r5, [sp]
    add.w	r6, sp, #114
    add.w	r0, sp, #2
    b.n .Lcode_15212
.Lcode_15211:
    ldrh.w	r5, [r3, #2]!
.Lcode_15212:
    strh.w	r5, [r0], #2
    cmp	r0, r6
    bne.n .Lcode_15211
    mov	r0, r1
    mov	r1, r2
    strh.w	r5, [sp, #114]
    strh.w	r5, [sp, #116]
    strh.w	r5, [sp, #118]
    bl vocoder_code_15159
    ldr	r6, [sp, #144]
    mov	r2, r0
    add.w	r6, r4, r6, lsl #1
    b.n .Lcode_15243
.Lcode_15225:
    adds	r1, r3, #1
    cmp	r1, #60
    it	eq
    moveq	r1, #59
.Lcode_15229:
    add.w	ip, sp, #120
    add.w	r3, ip, r3, lsl #1
    add.w	r1, ip, r1, lsl #1
    ldrh.w	r3, [r3, #-120]
    ldrh.w	r1, [r1, #-120]
    smulbb	r3, r3, r7
    lsls	r3, r3, #1
    smulbb	r5, r1, r5
    qdadd	r5, r3, r5
    asrs	r5, r5, #16
    strh.w	r5, [r4], #2
    cmp	r4, r6
    add	r2, r0
    beq.n .Lcode_15254
.Lcode_15243:
    ubfx	r5, r2, #1, #15
    lsls	r7, r5, #16
    rsb	r7, r7, #2130706432
    asrs	r3, r2, #16
    add.w	r7, r7, #16711680
    lsrs	r7, r7, #16
    cmp	r3, #59
    ble.n .Lcode_15225
    movs	r3, #59
    mov	r1, r3
    b.n .Lcode_15229
.Lcode_15254:
    add	sp, #124
    pop	{r4, r5, r6, r7, pc}
    .byte 0x00
    .byte 0xbf

.section .text.vocoder_rx_127,"ax",%progbits
.balign 4
.global vocoder_code_15256
.hidden vocoder_code_15256
vocoder_code_15256:
.global asm_interpolate_frame_state
.type asm_interpolate_frame_state, %function
.thumb_func
asm_interpolate_frame_state:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
    ldrsh.w	r3, [r1, #2]
    cmp	r3, #255
    sub	sp, #232
    mov	r5, r1
    mov	r6, r0
    mov	r4, r2
    bne.n .Lcode_15267
    ldrsh.w	r3, [r2, #2]
    cmp	r3, #255
    beq.n .Lcode_15272
.Lcode_15267:
    mov	r0, r6
    mov	r1, r5
    bl vocoder_code_19831
.Lcode_15270:
    add	sp, #232
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, pc}
.Lcode_15272:
    ldrsh.w	r3, [r2]
    cmp	r3, #2
    beq.w .Lcode_15365
    ldrsh.w	r2, [r1]
    cmp	r2, #2
    beq.n .Lcode_15349
    ldr	r3, [r4, #8]
    ldr	r2, [r0, #8]
    tst.w	r3, #1431655765
    beq.n .Lcode_15284
    tst.w	r2, #2863311530
    beq.n .Lcode_15360
.Lcode_15284:
    ldrsh.w	r0, [r5, #12]
.Lcode_15285:
    uxth	r3, r0
    sxth	r0, r3
    strh	r3, [r6, #12]
    bl vocoder_code_19411
    ldrh	r3, [r5, #2]
    ldrsh.w	r2, [r5, #12]
    ldrsh.w	r1, [r6, #12]
    strh	r3, [r6, #2]
    uxth	r7, r0
    add.w	r3, r5, #16
    strh	r7, [r6, #4]
    sxth	r5, r7
    add	r0, sp, #120
    str	r5, [sp, #0]
    bl vocoder_code_15203
    ldrsh.w	r5, [r6, #4]
    ldrsh.w	r2, [r4, #12]
    ldrsh.w	r1, [r6, #12]
    str	r5, [sp, #0]
    add.w	r3, r4, #16
    add	r0, sp, #8
    bl vocoder_code_15203
    ldrh	r3, [r6, #4]
    add	r2, sp, #8
    movw	r7, #34817
    mov.w	ip, #2
    add.w	r0, r6, #16
    sxth.w	sl, r3
    smlabb	ip, r3, ip, r2
    add	r4, sp, #120
    mov	r1, r2
    movw	r9, #30719
    movt	r7, #65535
.Lcode_15318:
    ldrsh.w	r8, [r4], #2
    ldrsh.w	r5, [r1], #2
    mov.w	r8, r8, lsl #16
    mov	r2, r8
    lsls	r5, r5, #16
    asrs	r3, r2, #31
    adds.w	r2, r8, r5
    adc.w	r3, r3, r5, asr #31
    asrs	r3, r3, #1
    mov.w	r2, r2, rrx
    asrs	r2, r2, #16
    cmp	r2, r9
    it	ge
    movge	r2, r9
    cmp	r2, r7
    it	lt
    movlt	r2, r7
    uxth	r2, r2
    cmp	r1, ip
    strh.w	r2, [r0], #2
    bne.n .Lcode_15318
    ldrsh.w	r3, [r6, #4]
    add.w	sl, sl, #8
    rsbs	r3, r3, #56
    add.w	r6, r6, sl, lsl #1
    beq.n .Lcode_15270
    add.w	r3, r6, r3, lsl #1
.Lcode_15345:
    strh.w	r2, [r6], #2
    cmp	r6, r3
    bne.n .Lcode_15345
    b.n .Lcode_15270
.Lcode_15349:
    ldrsh.w	r2, [r4, #134]
    cmp	r2, #1
    ble.n .Lcode_15381
.Lcode_15352:
    mov	r1, r5
    bl vocoder_code_19831
.Lcode_15354:
    ldrsh.w	r3, [r6, #14]
    cmp	r3, #0
    ble.n .Lcode_15386
    sub.w	r3, r3, #20480
    strh	r3, [r6, #14]
    b.n .Lcode_15270
.Lcode_15360:
    ldr	r3, [r1, #8]
    tst.w	r3, #1431655765
    bne.n .Lcode_15392
    ldrsh.w	r0, [r4, #12]
    b.n .Lcode_15285
.Lcode_15365:
    ldr	r7, [r6, #8]
    ldrh	r4, [r6, #14]
    bl vocoder_code_19831
    ldrsh.w	r3, [r5]
    str	r7, [r6, #8]
    cmp	r3, #2
    strh	r4, [r6, #14]
    beq.w .Lcode_15270
    add.w	r3, r6, #16
    adds	r6, #128
.Lcode_15375:
    ldrsh.w	r2, [r3]
    asrs	r2, r2, #1
    strh.w	r2, [r3], #2
    cmp	r3, r6
    bne.n .Lcode_15375
    b.n .Lcode_15270
.Lcode_15381:
    cmp	r3, #3
    beq.n .Lcode_15352
    mov	r1, r4
    bl vocoder_code_19831
    b.n .Lcode_15354
.Lcode_15386:
    ldr	r3, [r6, #8]
    and.w	r3, r3, #1431655765
    str	r3, [r6, #8]
    movs	r3, #0
    strh	r3, [r6, #14]
    b.n .Lcode_15270
.Lcode_15392:
    ldrh	r0, [r1, #12]
    ldrh	r3, [r4, #12]
    smulbb	r0, r0, r3
    lsls	r0, r0, #1
    mvn.w	r1, #7
    mvn.w	r2, #3
    bl vocoder_code_6971
    b.n .Lcode_15285
    .byte 0x00
    .byte 0xbf

.section .text.vocoder_rx_128,"ax",%progbits
.balign 4
.global vocoder_code_15400
.hidden vocoder_code_15400
vocoder_code_15400:
.global asm_decode_spectral_frame_block
.type asm_decode_spectral_frame_block, %function
.thumb_func
asm_decode_spectral_frame_block:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #36
    mov	r4, r2
    str	r1, [sp, #12]
    mov	r5, r0
    movs	r1, #6
    mov	r0, r2
    mov	r7, r3
    bl vocoder_code_14920
    movs	r1, #6
    mov	r0, r4
    bl vocoder_code_14920
    movs	r1, #0
    mov	r6, r0
    movs	r2, #8
    add	r0, sp, #16
    bl vocoder_code_2535
    ldrsh.w	sl, [r7, #4]
    sub.w	sl, sl, #13
    mov.w	sl, sl, asr #3
    lsls	r6, r6, #1
    cmp.w	sl, #0
    ble.n .Lcode_15482
    add	r7, sp, #16
    mov.w	fp, sl, lsl #1
    mov	r8, r7
    add.w	r9, r7, fp
.Lcode_15427:
    movs	r1, #8
    mov	r0, r4
    bl vocoder_code_14920
    strh.w	r0, [r7], #2
    cmp	r7, r9
    bne.n .Lcode_15427
    ldrh.w	r0, [sp, #16]
    cmp.w	sl, #1
    sxth	r7, r0
    ble.n .Lcode_15486
    ldr	r1, .Lliteral_15488
    add.w	r3, sp, #14
    ldrb.w	lr, [r1]
    add	fp, r3
    movs	r3, #0
.Lcode_15442:
    ldrsh.w	r2, [r8, #2]!
    eors	r2, r7
    uxtb	r2, r2
    cmp	r8, fp
    ldrb	r2, [r1, r2]
    add	r2, lr
    add	r3, r2
    bne.n .Lcode_15442
.Lcode_15450:
    ldrh.w	r2, [sp, #18]
    eors	r0, r2
    tst.w	r0, #240
    bne.n .Lcode_15456
    cmp	r3, #5
    ble.n .Lcode_15480
.Lcode_15456:
    movw	r7, #65535
.Lcode_15457:
    movs	r1, #1
    mov	r0, r4
    bl vocoder_code_14920
    orr.w	r2, r0, r6
    lsls	r2, r2, #17
    movw	r3, #30962
    sub.w	r2, r2, #1114112
    smultb	r2, r2, r3
    sbfx	r2, r2, #8, #16
    movs	r4, #0
    sxth	r1, r7
    mov	r0, r5
    mov	r3, r2
    str	r4, [sp, #0]
    bl vocoder_code_5559
    mov	r4, r0
    cbnz	r0, .Lcode_15477
    ldr	r0, [sp, #12]
    mov	r1, r5
    bl vocoder_code_19831
.Lcode_15477:
    mov	r0, r4
    add	sp, #36
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_15480:
    uxth	r7, r7
    b.n .Lcode_15457
.Lcode_15482:
    ldrh.w	r0, [sp, #16]
    movs	r3, #0
    sxth	r7, r0
    b.n .Lcode_15450
.Lcode_15486:
    movs	r3, #0
    b.n .Lcode_15450
.Lliteral_15488:
    .word vocoder_data_20707

.section .text.vocoder_rx_129,"ax",%progbits
.balign 4
.global vocoder_code_15489
.hidden vocoder_code_15489
vocoder_code_15489:
.global asm_decode_pitch_parameters
.type asm_decode_pitch_parameters, %function
.thumb_func
asm_decode_pitch_parameters:
    push	{r4, r5, r6, lr}
    cmp	r0, #1
    mov	r5, r2
    mov	r4, r1
    ldr	r2, [sp, #16]
    ldrsh.w	r6, [sp, #20]
    beq.n .Lcode_15501
    movw	r3, #16423
    strh	r3, [r1, #0]
    movs	r3, #15
    strh	r3, [r5, #0]
    pop	{r4, r5, r6, pc}
.Lcode_15501:
    subs	r2, #6
    movs	r1, #120
    lsls	r3, r3, #1
    lsls	r1, r2
    movw	r0, #10834
    mla	r0, r3, r0, r0
    sxth	r1, r1
    bl vocoder_code_6588
    lsls	r2, r0, #16
    rsb	r2, r2, #3137339392
    add.w	r2, r2, #196608
    asrs	r2, r2, #16
    mov	r0, r4
    mov	r1, r5
    mov	r3, r6
    ldmia.w	sp!, {r4, r5, r6, lr}
    b.w vocoder_code_19437

.section .text.vocoder_rx_130,"ax",%progbits
.balign 4
.global vocoder_code_15518
.hidden vocoder_code_15518
vocoder_code_15518:
.global asm_expand_log_spectrum
.type asm_expand_log_spectrum, %function
.thumb_func
asm_expand_log_spectrum:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
    mov.w	r3, #32768
    mov.w	r7, r2, lsl #1
    mov	r8, r2
    movw	r2, #34817
    mov	sl, r1
    adds	r5, r0, r7
    movt	r3, #65535
    movt	r2, #65535
    movw	r4, #30719
.Lcode_15528:
    ldrsh.w	r1, [r0]
    cmp	r1, r2
    it	lt
    movlt	r1, r2
    cmp	r1, r4
    it	ge
    movge	r1, r4
    strh.w	r1, [r0], #2
    cmp	r3, r1
    it	lt
    movlt	r3, r1
    cmp	r0, r5
    bne.n .Lcode_15528
    lsls	r6, r3, #16
    asrs	r6, r6, #11
    rsb	r9, r7, r0
    add.w	r6, r6, #65536
    asrs	r6, r6, #16
    mov	r5, r9
    mov	r4, r8
.Lcode_15548:
    ldrsh.w	r0, [r5]
    movs	r1, #4
    mov	r2, r6
    bl vocoder_code_6911
    subs	r4, #1
    strh.w	r0, [r5], #2
    bne.n .Lcode_15548
    rsb	r8, r8, #56
    subs	r6, #15
    sxth.w	r2, r8
    add.w	r0, r9, r7
    mov	r1, r4
    str.w	r6, [sl]
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, lr}
    b.w vocoder_code_2535
    .byte 0x00
    .byte 0xbf
.global vocoder_code_15563
.hidden vocoder_code_15563
vocoder_code_15563:
    push	{r4, r5, r6, r7}
    sub	sp, #32
    add	r6, sp, #32
    mov	r4, sp
.Lcode_15567:
    and.w	r5, r1, #3
    strh.w	r5, [r4], #2
    asrs	r1, r1, #2
    cmp	r4, r6
    bne.n .Lcode_15567
    lsls	r2, r2, #2
    ldrsh.w	r6, [sp, #30]
    add.w	r7, r0, r3, lsl #1
    mov	r4, r2
    b.n .Lcode_15592
.Lcode_15577:
    cmp.w	ip, #16
    ldrsh.w	r5, [r5, #-32]
    beq.n .Lcode_15611
    add	r1, sp, #32
    add.w	ip, r1, ip, lsl #1
    ldrsh.w	r1, [ip, #-32]
.Lcode_15583:
    orrs	r1, r5
    cmp	r1, #2
    ite	le
    uxthle	r1, r1
    movgt	r1, #1
    strh.w	r1, [r0], #2
    cmp	r0, r7
    add	r4, r2
    beq.n .Lcode_15601
.Lcode_15592:
    asrs	r1, r4, #16
    add.w	ip, sp, #32
    cmp	r1, #15
    add.w	r5, ip, r1, lsl #1
    add.w	ip, r1, #1
    ble.n .Lcode_15577
    mov	r1, r6
    mov	r5, r6
    b.n .Lcode_15583
.Lcode_15601:
    rsbs	r3, r3, #56
    beq.n .Lcode_15608
    add.w	r3, r0, r3, lsl #1
    movs	r2, #0
.Lcode_15605:
    strh.w	r2, [r0], #2
    cmp	r0, r3
    bne.n .Lcode_15605
.Lcode_15608:
    add	sp, #32
    pop	{r4, r5, r6, r7}
    bx	lr
.Lcode_15611:
    mov	r1, r6
    b.n .Lcode_15583
    .byte 0x00
    .byte 0xbf

.section .text.vocoder_rx_131,"ax",%progbits
.balign 4
.global vocoder_code_15613
.hidden vocoder_code_15613
vocoder_code_15613:
.global asm_smooth_if_mask_majority
.type asm_smooth_if_mask_majority, %function
.thumb_func
asm_smooth_if_mask_majority:
    and.w	r2, r2, #1431655765
    push	{r4, r5, r6}
    lsrs	r5, r2, #8
    ldr	r3, .Lliteral_15640
    lsrs	r4, r2, #16
    uxtb	r6, r2
    uxtb	r5, r5
    uxtb	r4, r4
    ldrb	r6, [r3, r6]
    ldrb	r5, [r3, r5]
    ldrb	r4, [r3, r4]
    lsrs	r2, r2, #24
    adds	r5, r6, r5
    ldrb	r2, [r3, r2]
    adds	r3, r5, r4
    adds	r3, r3, r2
    cmp	r3, #7
    ble.n .Lcode_15638
    movw	r3, #29491
    smulbb	r0, r0, r3
    movw	r3, #26214
    smulbb	r1, r1, r3
    lsls	r0, r0, #1
    add.w	r0, r0, r1, asr #2
    asrs	r0, r0, #16
.Lcode_15638:
    pop	{r4, r5, r6}
    bx	lr
    .byte 0x00
    .byte 0xbf
.Lliteral_15640:
    .word vocoder_data_20707

.section .text.vocoder_rx_132,"ax",%progbits
.balign 4
.global vocoder_code_15641
.hidden vocoder_code_15641
vocoder_code_15641:
.global asm_synthesize_excitation_frame
.type asm_synthesize_excitation_frame, %function
.thumb_func
asm_synthesize_excitation_frame:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    ldrsh.w	r5, [r2, #338]
    sub.w	sp, sp, #796
    cmp	r1, #128
    str	r2, [sp, #60]
    mov	fp, r1
    str	r0, [sp, #76]
    mov	r7, r3
    str	r5, [sp, #44]
    ble.w .Lcode_16169
    movs	r3, #8
    movs	r1, #1
.Lcode_15653:
    lsls.w	r2, r1, r3
    adds	r3, #1
    cmp	r2, fp
    blt.n .Lcode_15653
    str	r3, [sp, #68]
    ldr.w	r8, [sp, #68]
    movs	r3, #1
    lsl.w	r3, r3, r8
    str	r3, [sp, #64]
.Lcode_15662:
    mov.w	r2, fp, lsl #16
    mov	r3, r2
    mov.w	r0, #5242880
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsls	r2, r3
    mov	r1, r0
    cmp	r1, #0
    it	lt
    mvnlt	r1, r1
    itt	ne
    clzne	r1, r1
    subne	r1, #1
    sxth	r1, r1
    lsls	r0, r1
    negs	r3, r3
    negs	r1, r1
    asrs	r2, r2, #16
    add.w	r9, sp, #780
    str.w	r9, [sp]
    bl vocoder_code_6721
    ldr	r3, [sp, #780]
    lsls	r0, r0, #16
    cmp	r3, #14
    ble.w .Lcode_16044
    subs	r3, #15
    lsl.w	lr, r0, r3
.Lcode_15694:
    ldr	r4, .Lliteral_15895
    ldrh	r3, [r4, #0]
    cmp.w	fp, #1
    strh.w	r3, [sp, #608]
    ble.n .Lcode_15719
    mov	r2, lr
    movs	r3, #1
.Lcode_15701:
    asrs	r0, r2, #16
    adds	r1, r0, #1
    ldrh.w	r0, [r4, r0, lsl #1]
    ldrh.w	r5, [r4, r1, lsl #1]
    ubfx	r1, r2, #1, #15
    smulbb	r6, r0, r1
    lsls	r6, r6, #1
    rsb	r0, r6, r0, lsl #16
    smulbb	r1, r1, r5
    add.w	r8, sp, #792
    add.w	ip, r8, r3, lsl #1
    add.w	r1, r0, r1, lsl #1
    adds	r3, #1
    asrs	r1, r1, #16
    cmp	fp, r3
    strh.w	r1, [ip, #-184]
    add	r2, lr
    bgt.n .Lcode_15701
.Lcode_15719:
    mov.w	sl, fp, lsl #1
    add	r0, sp, #792
    ldrsh.w	r3, [r7]
    ldrh.w	r1, [r4, #160]
    str.w	sl, [sp, #52]
    add.w	r2, r0, sl
    cmp	r3, #3
    strh.w	r1, [r2, #-184]
    beq.w .Lcode_16147
    ldr.w	r8, [sp, #60]
    cmp.w	fp, #0
    add.w	r3, r8, #2
    ble.w .Lcode_16176
    ldr	r5, [sp, #52]
    add.w	sl, sp, #96
    add.w	r8, sp, #608
    str.w	sl, [sp, #56]
    mov	r0, r3
    add.w	r6, r8, r5
    mov	r1, sl
    mov	r2, r8
.Lcode_15740:
    ldrh.w	r4, [r2], #2
    ldrh.w	r5, [r0], #2
    smulbb	r4, r5, r4
    sbfx	r4, r4, #15, #16
    cmp	r2, r6
    strh.w	r4, [r1], #2
    bne.n .Lcode_15740
    ldr.w	r8, [sp, #52]
    ldr.w	sl, [sp, #60]
    add.w	r5, sl, r8
    add.w	sl, sp, #608
    add.w	r1, sl, r8
    ldr.w	sl, [sp, #56]
    adds	r5, #2
    add.w	r6, sl, r8
.Lcode_15755:
    rsb	r8, fp, #84
    cmp.w	r8, #0
    str.w	r8, [sp, #72]
    ble.n .Lcode_15770
    mov	r2, r5
    mov.w	r5, r8, lsl #1
    adds	r4, r2, r5
.Lcode_15762:
    ldrh.w	r0, [r2], #2
    cmp	r2, r4
    strh.w	r0, [r3], #2
    bne.n .Lcode_15762
    ldr.w	sl, [sp, #60]
    add.w	r3, sl, r5
    adds	r3, #2
    mov	r5, r2
.Lcode_15770:
    cmp.w	fp, #0
    ble.n .Lcode_15798
    ldr.w	r8, [sp, #52]
    ldr.w	lr, [sp, #60]
    add.w	r4, r3, r8
    movs	r0, #173
.Lcode_15776:
    ldrh.w	r2, [lr]
    smulbb	r2, r2, r0
    add.w	r2, r2, #13824
    adds	r2, #25
    uxth	r2, r2
    strh.w	r2, [lr]
    strh.w	r2, [r3], #2
    cmp	r3, r4
    bne.n .Lcode_15776
    ldr.w	sl, [sp, #52]
    subs	r5, #170
    rsb	r4, sl, r1
    mov	r3, r6
.Lcode_15789:
    ldrh.w	r2, [r1], #-2
    ldrh.w	r0, [r5, #2]!
    smulbb	r2, r0, r2
    sbfx	r2, r2, #15, #16
    cmp	r1, r4
    strh.w	r2, [r3], #2
    bne.n .Lcode_15789
    ldr	r5, [sp, #52]
    adds	r6, r6, r5
.Lcode_15798:
    ldr.w	r8, [sp, #64]
    rsb	r2, fp, fp, lsl #15
    add.w	r2, r8, r2, lsl #1
    movs	r1, #0
    sxth	r2, r2
    mov	r0, r6
    bl vocoder_code_2535
    movs	r1, #0
    mov	r3, r1
    ldr	r2, [sp, #68]
    ldr	r0, [sp, #56]
    bl vocoder_code_7371
    ldrsh.w	r2, [r7]
    ldrh	r6, [r7, #4]
    ldr.w	sl, [sp, #64]
    ldr	r5, [sp, #64]
    ldr.w	r3, [r7, #128]
    sub.w	r1, r2, #2
    negs	r2, r1
    sxth.w	lr, r6
    adc.w	r2, r2, r1
    mov.w	sl, sl, asr #1
    cmp.w	r5, #512
    ite	ne
    movne	r5, #121
    moveq	r5, #242
    cmp.w	lr, #0
    mov	r8, r0
    str	r2, [sp, #24]
    str.w	sl, [sp, #16]
    str	r5, [sp, #80]
    ble.w .Lcode_16174
    movs	r5, #2
    add.w	r2, r7, #16
    smlabb	r5, r6, r5, r3
    movs	r4, #0
.Lcode_15834:
    ldrh.w	r0, [r2], #2
    ldrh.w	r1, [r3], #2
    lsls	r0, r0, #16
    cbnz	r1, .Lcode_15841
    cmp	r4, r0
    it	lt
    movlt	r4, r0
.Lcode_15841:
    cmp	r3, r5
    bne.n .Lcode_15834
.Lcode_15843:
    ldr	r5, [sp, #24]
    mov	ip, r4
    cmp.w	ip, #0
    it	lt
    mvnlt.w	ip, ip
    itt	ne
    clzne	ip, ip
    subne.w	ip, ip, #1
    cmp	r5, #0
    bne.w .Lcode_16047
    mul.w	lr, lr, fp
    movw	r3, #17243
    smulbb	r3, lr, r3
    lsls	r0, r3, #1
    mov	r1, r9
    movs	r3, #17
    str.w	ip, [sp, #12]
    str	r3, [sp, #780]
    bl vocoder_code_6932
    ldr	r2, [sp, #780]
    ldr.w	ip, [sp, #12]
    lsrs	r0, r0, #16
    str	r0, [sp, #32]
.Lcode_15866:
    ldrsh.w	r3, [r7, #12]
    ldr.w	r8, [sp, #64]
    ldr	r0, [sp, #56]
    str.w	ip, [sp, #12]
    mul.w	r3, r8, r3
    sbfx	r3, r3, #3, #28
    add.w	r8, r3, #65536
    mov.w	r6, r8, asr #17
    str	r3, [sp, #20]
    lsls	r4, r6, #1
    rsb	r3, ip, r2
    adds	r3, #2
    movs	r1, #0
    mov	r2, r4
    str	r3, [sp, #28]
    bl vocoder_code_2535
    ldrsh.w	r3, [r7, #4]
    ldr.w	ip, [sp, #12]
    cmp	r3, #0
    mov.w	r8, r8, asr #1
    mov	r5, r0
    ble.w .Lcode_16182
    rsb	r0, ip, #0
    str.w	fp, [sp, #84]
    mov	sl, r7
    movs	r4, #0
    str	r0, [sp, #48]
    mov	fp, ip
    b.n .Lcode_15927
    .byte 0x00
    .byte 0xbf
.Lliteral_15895:
    .word vocoder_data_20711
.Lcode_15896:
    ldr	r3, [sp, #24]
    cmp	r3, #0
    beq.w .Lcode_16065
    ldrh.w	r3, [sl, #16]
    ldr	r0, [sp, #32]
    ldr	r2, [sp, #780]
    smulbb	r3, r0, r3
    mov.w	ip, r6, lsl #1
    lsls	r3, r3, #1
.Lcode_15905:
    cmp.w	fp, #0
    it	lt
    ldrlt	r0, [sp, #48]
    ldr	r1, [sp, #28]
    ite	ge
    lslge.w	r3, r3, fp
    asrlt	r3, r0
    rsb	r2, fp, r2
    asrs	r3, r3, #16
    str	r2, [sp, #0]
    str	r1, [sp, #4]
    mov	r0, r5
    mov	r1, r5
    mov	r2, ip
    bl vocoder_code_7521
    ldrsh.w	r3, [r7, #4]
    adds	r4, #1
    cmp	r4, r3
    add.w	r5, r5, r6, lsl #2
    add.w	sl, sl, #2
    bge.n .Lcode_15950
.Lcode_15926:
    mov	r6, r9
.Lcode_15927:
    ldr	r1, [sp, #20]
    ldr.w	r3, [r7, #128]
    ldr	r2, [sp, #16]
    ldrsh.w	r3, [r3, r4, lsl #1]
    add	r8, r1
    mov.w	r9, r8, asr #16
    cmp	r9, r2
    it	ge
    movge	r9, r2
    rsb	r6, r6, r9
    cmp	r3, #0
    beq.n .Lcode_15896
    lsls	r2, r6, #1
    mov	r0, r5
    movs	r1, #0
    sxth	r2, r2
    bl vocoder_code_2535
    ldrsh.w	r3, [r7, #4]
    adds	r4, #1
    cmp	r4, r3
    add.w	r5, r5, r6, lsl #2
    add.w	sl, sl, #2
    blt.n .Lcode_15926
.Lcode_15950:
    ldr.w	fp, [sp, #84]
.Lcode_15951:
    ldr	r5, [sp, #16]
    ldr.w	r8, [sp, #80]
    ldr.w	sl, [sp, #64]
    rsb	r2, r8, r5
    rsb	r9, r9, r5
    cmp	r9, r2
    it	ge
    movge	r2, r9
    ldr	r5, [sp, #56]
    sub.w	r0, sl, r2, lsl #1
    lsls	r2, r2, #1
    movs	r1, #0
    add.w	r0, r5, r0, lsl #1
    sxth	r2, r2
    bl vocoder_code_2535
    ldrsh.w	r3, [sp, #832]
    ldr	r0, [sp, #56]
    mul.w	r3, sl, r3
    sbfx	r2, r3, #19, #12
    lsls	r2, r2, #1
    movs	r1, #0
    bl vocoder_code_2535
    ldr.w	r1, [r7, #132]
    ldr.w	r8, [sp, #28]
    ldr	r0, [sp, #56]
    ldr	r2, [sp, #68]
    movs	r3, #0
    add	r1, r8
    bl vocoder_code_7387
    ldr	r5, [sp, #60]
    uxth.w	sl, r0
    cmp.w	fp, #0
    str.w	sl, [sp, #32]
    add.w	r4, r5, #170
    ble.w .Lcode_16167
.Lcode_15986:
    ldr.w	r8, [sp, #52]
    ldr	r5, [sp, #56]
    str	r0, [sp, #16]
    add.w	sl, sp, #608
    add.w	r9, r5, r8
    add.w	ip, sl, r8
    ldr.w	r8, [sp, #44]
    str.w	r9, [sp, #20]
    mov	lr, sl
    rsb	r1, r0, #2
    rsb	sl, r8, #2
    sub.w	r7, r8, #2
    str.w	sl, [sp, #24]
    subs	r6, r0, #2
    str	r1, [sp, #28]
    ldr.w	r9, [sp, #76]
    str.w	fp, [sp, #48]
    ldr	r3, [sp, #88]
    ldr	r2, [sp, #92]
    b.n .Lcode_16028
.Lcode_16006:
    ldr.w	fp, [sp, #24]
    asr.w	r0, r1, fp
.Lcode_16008:
    ldrsh.w	r1, [r5], #2
    ldrsh.w	r8, [lr], #2
    ldr.w	sl, [sp, #36]
    smull	r3, sl, r1, r8
    str.w	sl, [sp, #36]
    ldr.w	sl, [sp, #16]
    qadd	r8, r3, r3
    cmp.w	sl, #2
    lsl.w	r1, r8, r6
    asr.w	fp, r1, r6
    bgt.w .Lcode_16115
    ldr.w	fp, [sp, #28]
    asr.w	r1, r8, fp
    ldr.w	fp, [sp, #20]
    ldr.w	r8, [r9]
    qadd	r1, r1, r0
    cmp	r5, fp
    qadd	r1, r8, r1
    str.w	r1, [r9], #4
    beq.w .Lcode_16126
.Lcode_16028:
    ldr.w	sl, [sp, #44]
    ldrsh.w	r1, [r4], #2
    ldrsh.w	r0, [ip], #-2
    ldr.w	r8, [sp, #40]
    smull	r2, r8, r1, r0
    qadd	r1, r2, r2
    lsls.w	r0, r1, r7
    cmp.w	sl, #2
    str.w	r8, [sp, #40]
    asr.w	r8, r0, r7
    ble.n .Lcode_16006
    cmp	r1, r8
    itt	ne
    mvnne.w	r8, #2147483648
    eorne.w	r0, r8, r1, asr #31
    b.n .Lcode_16008
.Lcode_16044:
    rsb	r3, r3, #15
    asr.w	lr, r0, r3
    b.n .Lcode_15694
.Lcode_16047:
    movw	r3, #17243
    smulbb	r3, r6, r3
    lsls	r0, r3, #1
    mov	r1, r9
    movs	r3, #17
    str	r3, [sp, #780]
    str.w	ip, [sp, #12]
    bl vocoder_code_6932
    ldr	r3, [sp, #780]
    ldr.w	ip, [sp, #12]
    add.w	r2, r8, #1
    adds	r2, r2, r3
    movw	r3, #22522
    smultb	r3, r0, r3
    ubfx	r3, r3, #15, #16
    str	r2, [sp, #780]
    str	r3, [sp, #32]
    b.n .Lcode_15866
.Lcode_16065:
    mov.w	ip, r6, lsl #1
    mov	r1, r5
    mov	r2, ip
    add	r0, sp, #784
    str.w	ip, [sp, #12]
    bl vocoder_code_7556
    asrs	r1, r0, #16
    cmp	r1, #0
    ldr.w	ip, [sp, #12]
    ble.n .Lcode_16112
    lsls	r0, r6, #16
    mov	r3, r0
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsls	r0, r3
    rsb	r2, r3, #15
    cmp.w	r1, r0, asr #16
    str	r2, [sp, #788]
    bgt.n .Lcode_16092
    rsb	r3, r3, #16
    asrs	r0, r0, #1
    str	r3, [sp, #788]
.Lcode_16092:
    str.w	ip, [sp, #12]
    bl vocoder_code_6534
    ldr	r2, [sp, #788]
    ldr	r3, [sp, #784]
    add	r1, sp, #788
    subs	r3, r2, r3
    lsls	r0, r0, #16
    str	r3, [sp, #788]
    bl vocoder_code_6932
    ldr	r1, [sp, #32]
    ldrh.w	r2, [sl, #16]
    ldr.w	ip, [sp, #12]
    smultb	r3, r0, r1
    ubfx	r3, r3, #15, #16
    smulbb	r3, r3, r2
    ldr	r1, [sp, #788]
    lsls	r3, r3, #1
.Lcode_16109:
    ldr	r2, [sp, #780]
    adds	r2, r1, r2
    b.n .Lcode_15905
.Lcode_16112:
    ldr	r3, [sp, #24]
    mov	r1, r3
    b.n .Lcode_16109
.Lcode_16115:
    cmp	r8, fp
    ldr.w	fp, [sp, #20]
    itt	ne
    mvnne.w	sl, #2147483648
    eorne.w	r1, sl, r8, asr #31
    cmp	r5, fp
    ldr.w	r8, [r9]
    qadd	r1, r1, r0
    qadd	r1, r8, r1
    str.w	r1, [r9], #4
    bne.w .Lcode_16028
.Lcode_16126:
    ldr.w	fp, [sp, #48]
    ldr	r5, [sp, #60]
    ldr.w	r8, [sp, #56]
    ldr.w	sl, [sp, #52]
    add.w	r4, r5, fp, lsl #1
    adds	r4, #170
    add.w	r1, r8, sl
.Lcode_16133:
    ldr	r5, [sp, #52]
    sxth.w	r2, fp
    subs	r0, r4, r5
    bl vocoder_code_2496
    ldr.w	r8, [sp, #72]
    mov	r0, r4
    sxth.w	r2, r8
    movs	r1, #0
    bl vocoder_code_2535
    ldr.w	sl, [sp, #32]
    ldr.w	fp, [sp, #60]
    strh.w	sl, [fp, #338]
    add.w	sp, sp, #796
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_16147:
    ldr	r5, [sp, #60]
    movs	r1, #0
    adds	r0, r5, #2
    movs	r2, #84
    mov.w	sl, #0
    add.w	r8, sp, #96
    rsb	r5, fp, #84
    str.w	r8, [sp, #56]
    bl vocoder_code_2535
    ldr	r0, [sp, #56]
    str.w	sl, [sp, #32]
    mov	r1, sl
    mov.w	r2, #256
    bl vocoder_code_2535
    str	r5, [sp, #72]
    ldr	r5, [sp, #60]
    ldr	r0, [sp, #32]
    cmp.w	fp, #0
    add.w	r4, r5, #170
    bgt.w .Lcode_15986
.Lcode_16167:
    ldr	r1, [sp, #56]
    b.n .Lcode_16133
.Lcode_16169:
    mov.w	sl, #256
    movs	r5, #8
    str.w	sl, [sp, #64]
    str	r5, [sp, #68]
    b.n .Lcode_15662
.Lcode_16174:
    movs	r4, #0
    b.n .Lcode_15843
.Lcode_16176:
    add	r5, sp, #96
    str	r5, [sp, #56]
    ldr	r6, [sp, #56]
    mov	r5, r3
    add	r1, sp, #608
    b.n .Lcode_15755
.Lcode_16182:
    mov	r9, r6
    b.n .Lcode_15951
    .byte 0x00
    .byte 0xbf

.section .text.vocoder_rx_133,"ax",%progbits
.balign 4
.global vocoder_code_16184
.hidden vocoder_code_16184
vocoder_code_16184:
.global asm_accumulate_interpolated_samples
.type asm_accumulate_interpolated_samples, %function
.thumb_func
asm_accumulate_interpolated_samples:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp}
    sub	sp, #24
    ldrsh.w	r6, [sp, #64]
    ldrsh.w	r5, [sp, #68]
    str	r6, [sp, #4]
    ldr	r6, [sp, #56]
    ldrsh.w	r4, [sp, #60]
    ldr	r6, [r6, #0]
    cmp	r5, #0
    str	r6, [sp, #12]
    ble.n .Lcode_16267
    subs	r5, #1
    uxth	r5, r5
    str	r5, [sp, #20]
    ldr	r5, [sp, #4]
    ldr.w	ip, .Lliteral_16278
    lsls	r5, r5, #16
    add.w	fp, r6, #4
    str	r5, [sp, #8]
    mov	r5, r6
    ldr	r6, [sp, #20]
    lsls	r2, r2, #5
    add.w	fp, fp, r6, lsl #2
    negs	r6, r4
    str	r6, [sp, #16]
    mov	sl, r2
    mov	r7, r3
    b.n .Lcode_16218
.Lcode_16212:
    lsls	r3, r4
    ldr	r6, [r5, #0]
    qadd	r3, r6, r3
    str.w	r3, [r5], #4
    cmp	r5, fp
    beq.n .Lcode_16262
.Lcode_16218:
    ldr	r6, [r0, #0]
    ubfx	r3, r6, #16, #8
    adds	r2, r3, #1
    ldrsh.w	r3, [r1, r3, lsl #1]
    ldrsh.w	r2, [r1, r2, lsl #1]
    lsls	r3, r3, #15
    rsb	r2, r3, r2, lsl #15
    ubfx	r9, r6, #1, #15
    add	r6, sl
    lsrs	r2, r2, #16
    str	r6, [r0, #0]
    ldr	r6, [sp, #4]
    sbfx	r8, r7, #6, #16
    smulbb	r2, r9, r2
    mov.w	r8, r8, lsl #16
    add.w	r3, r3, r2, lsl #1
    cbz	r6, .Lcode_16253
    asrs.w r6, r8, #6
    bmi.n .Lcode_16273
    mov.w	r8, r8, asr #22
    add.w	r9, r8, #1
    uxth	r6, r6
.Lcode_16240:
    ldrsh.w	r2, [ip, r8, lsl #1]
    ldrsh.w	r8, [ip, r9, lsl #1]
    mul.w	r9, r6, r2
    rsb	r2, r9, r2, lsl #16
    mla	r2, r6, r8, r2
    asrs	r2, r2, #16
    rsb	r2, r2, #32768
    sbfx	r2, r2, #1, #16
    smull	r2, r3, r2, r3
    lsrs	r2, r2, #15
    orr.w	r3, r2, r3, lsl #17
    ldr	r2, [sp, #8]
    adds	r7, r7, r2
.Lcode_16253:
    cmp	r4, #0
    bge.n .Lcode_16212
    ldr	r6, [sp, #16]
    asrs	r3, r6
    ldr	r6, [r5, #0]
    qadd	r3, r6, r3
    str.w	r3, [r5], #4
    cmp	r5, fp
    bne.n .Lcode_16218
.Lcode_16262:
    ldr	r2, [sp, #20]
    ldr	r4, [sp, #12]
    adds	r5, r2, #1
    add.w	r4, r4, r5, lsl #2
    str	r4, [sp, #12]
.Lcode_16267:
    ldr	r5, [sp, #12]
    ldr	r6, [sp, #56]
    str	r5, [r6, #0]
    add	sp, #24
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp}
    bx	lr
.Lcode_16273:
    negs	r6, r6
    mov.w	r8, r6, asr #16
    add.w	r9, r8, #1
    uxth	r6, r6
    b.n .Lcode_16240
.Lliteral_16278:
    .word vocoder_data_19902

.section .text.vocoder_rx_134,"ax",%progbits
.balign 4
.global vocoder_code_16279
.hidden vocoder_code_16279
vocoder_code_16279:
.global asm_build_inverse_spectrum
.type asm_build_inverse_spectrum, %function
.thumb_func
asm_build_inverse_spectrum:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #20
    mov	r7, r0
    ldr	r4, [sp, #68]
    ldr.w	r8, [sp, #72]
    ldr	r6, [sp, #56]
    ldr.w	fp, [sp, #60]
    ldr	r5, [sp, #76]
    str	r1, [sp, #12]
    cmp	r8, r4
    stmia.w	sp, {r2, r3}
    bge.w .Lcode_16411
    mov.w	lr, r8, lsl #1
    movs	r3, #0
    add.w	r0, r6, lr
    mov	r2, r8
    mov	r9, r3
    add	lr, fp
    b.n .Lcode_16301
.Lcode_16298:
    adds	r3, #2
    cmp	r2, r4
    beq.n .Lcode_16313
.Lcode_16301:
    ldrsh	r1, [r0, r3]
    adds	r2, #1
    cmp	r5, r1
    bne.n .Lcode_16298
    ldrsh.w	r1, [lr, r3]
    lsls	r1, r1, #16
    cmp	r9, r1
    it	lt
    movlt	r9, r1
    adds	r3, #2
    cmp	r2, r4
    bne.n .Lcode_16301
.Lcode_16313:
    mov.w	r2, #256
    mov	r0, r7
    movs	r1, #0
    bl vocoder_code_2535
    cmp.w	r9, #0
    it	lt
    mvnlt.w	r9, r9
    itt	ne
    clzne	r9, r9
    subne.w	r9, r9, #1
    rsb	r2, r9, #0
    str	r2, [sp, #8]
    ldr	r0, .Lliteral_16415
    mov	r2, r8
    mov.w	r8, #0
    b.n .Lcode_16332
.Lcode_16329:
    adds	r2, #1
    cmp	r4, r2
    ble.n .Lcode_16381
.Lcode_16332:
    ldrsh.w	r1, [r6, r2, lsl #1]
    lsls	r3, r2, #1
    cmp	r1, r5
    bne.n .Lcode_16329
    ldr.w	sl, [sp, #4]
    ldr.w	ip, [sp]
    ldrsh.w	r1, [sl, r2, lsl #1]
    adds	r2, #1
    mla	r1, r2, ip, r1
    lsls	r1, r1, #10
    add.w	r1, r1, #32768
    and.w	ip, r1, #2147483648
    lsls	r1, r1, #7
    add.w	r1, ip, r1, lsr #1
    asrs	r1, r1, #6
    bmi.n .Lcode_16399
    lsrs	r1, r1, #16
    lsls	r1, r1, #16
.Lcode_16350:
    ubfx	r1, r1, #16, #9
    ldrh.w	sl, [fp, r3]
    ldrh.w	ip, [r0, r1, lsl #1]
    cmp.w	r9, #0
    smulbb	ip, sl, ip
    it	lt
    ldrlt.w	sl, [sp, #8]
    mov.w	ip, ip, lsl #1
    ite	ge
    lslge.w	ip, ip, r9
    asrlt.w	ip, ip, sl
    lsls	r1, r1, #16
    mov.w	ip, ip, asr #16
    add.w	r1, r1, #25165824
    strh.w	ip, [r7, r2, lsl #2]
    ubfx	r1, r1, #16, #9
    ldrh.w	r3, [fp, r3]
    ldrh.w	r1, [r0, r1, lsl #1]
    smulbb	r1, r3, r1
    lsls	r1, r1, #1
    cmp.w	r9, #0
    itet	lt
    ldrlt.w	ip, [sp, #8]
    lslge.w	r1, r1, r9
    asrlt.w	r1, r1, ip
    add.w	r3, r7, r2, lsl #2
    asrs	r1, r1, #16
    cmp	r4, r2
    strh	r1, [r3, #2]
    add.w	r8, r8, #1
    bgt.n .Lcode_16332
.Lcode_16381:
    ldr	r1, [sp, #64]
    ldr	r2, [sp, #8]
    ldr.w	sl, [sp, #12]
    adds	r1, #23
    adds	r1, r1, r2
    uxth	r1, r1
    strh.w	r1, [sl]
    cmp.w	r8, #0
    bne.n .Lcode_16404
.Lcode_16390:
    ldr.w	ip, [sp, #12]
    movw	r3, #65504
    strh.w	r3, [ip]
    mov.w	r8, #0
.Lcode_16394:
    ldrh	r3, [r7, #0]
    mov	r0, r8
    strh.w	r3, [r7, #512]
    add	sp, #20
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_16399:
    negs	r1, r1
    lsrs	r1, r1, #16
    lsls	r1, r1, #16
    negs	r1, r1
    b.n .Lcode_16350
.Lcode_16404:
    sxth	r1, r1
    mov	r0, r7
    movs	r2, #8
    movs	r3, #0
    bl vocoder_code_7387
    strh.w	r0, [sl]
    b.n .Lcode_16394
.Lcode_16411:
    movs	r1, #0
    mov.w	r2, #256
    bl vocoder_code_2535
    b.n .Lcode_16390
    .byte 0x00
    .byte 0xbf
.Lliteral_16415:
    .word vocoder_data_19902

.section .text.vocoder_rx_135,"ax",%progbits
.balign 4
.global vocoder_code_16416
.hidden vocoder_code_16416
vocoder_code_16416:
.global asm_synthesize_interpolated_segment
.type asm_synthesize_interpolated_segment, %function
.thumb_func
asm_synthesize_interpolated_segment:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    mov	r5, r2
    lsls	r2, r1, #16
    cmp	r5, #14
    itete	gt
    subgt.w	r8, r5, #15
    rsble	r8, r5, #15
    lslgt.w	r8, r2, r8
    asrle.w	r8, r2, r8
    lsls	r6, r3, #16
    mov	r7, r1
    mov	r2, r6
    movw	r3, #65532
    cmp	r2, #0
    it	lt
    mvnlt	r2, r2
    itt	ne
    clzne	r2, r2
    subne	r2, #1
    sxth	r1, r2
    sub	sp, #44
    lsls.w	r2, r6, r1
    movt	r3, #65535
    asrs	r2, r2, #16
    subs	r3, r3, r1
    add.w	lr, sp, #36
    movs	r1, #1
    mov	r9, r0
    mov.w	r0, #1073741824
    str.w	lr, [sp]
    bl vocoder_code_6721
    mov	r1, r5
    mov	r2, r0
    ldr	r3, [sp, #36]
    mov	r0, r7
    add	r5, sp, #32
    str	r5, [sp, #0]
    bl vocoder_code_6611
    ldr	r3, [sp, #32]
    movw	r4, #65535
    lsls	r0, r0, #16
    qadd	r4, r4, r8
    asrs	r4, r4, #16
    cmp	r3, #14
    mov.w	sl, r4, lsl #16
    ble.n .Lcode_16550
    subs	r3, #15
    lsls.w	r3, r0, r3
    asrs	r2, r3, #16
.Lcode_16465:
    cmp	r2, #166
    ite	lt
    movlt	r5, r2
    movge	r5, #166
    cmp	r5, #150
    bgt.n .Lcode_16548
    add.w	fp, r5, #16
    cmp.w	fp, #0
    ble.n .Lcode_16546
.Lcode_16474:
    rsb	r8, r8, sl
    ubfx	r8, r8, #1, #16
    sub.w	ip, r3, r2, lsl #16
    lsrs	r6, r6, #16
    smulbb	r1, r6, r8
    sbfx	ip, ip, #0, #17
    mov.w	r8, r8, lsl #16
    bic.w	ip, ip, #1
    sbfx	r1, r1, #10, #21
    cmp	r4, #0
    str	r1, [sp, #24]
    mov.w	r3, r8, asr #15
    add.w	ip, ip, #983040
    blt.n .Lcode_16559
    lsls	r2, r4, #2
    mov	r7, r4
.Lcode_16490:
    add.w	sl, r4, #16
    ldrsh.w	r4, [sp, #84]
    rsb	lr, r7, sl
    subs	r4, #16
    add	r2, r9
    cmp.w	lr, #0
    str	r2, [sp, #28]
    uxth	r4, r4
    ble.n .Lcode_16554
    sxth.w	r9, r6
    add.w	r8, sp, #24
    sxth.w	lr, lr
    movs	r7, #1
    sxth	r4, r4
    add	r6, sp, #28
    mov	r0, r8
    ldr	r1, [sp, #80]
    str	r7, [sp, #8]
    mov	r2, r9
    str.w	ip, [sp, #20]
    str	r6, [sp, #0]
    str	r4, [sp, #4]
    str.w	lr, [sp, #12]
    bl vocoder_code_16184
    ldr.w	ip, [sp, #20]
    mov	r7, sl
.Lcode_16516:
    subs	r5, r5, r7
    adds	r5, #1
    cmp	r5, #0
    ble.n .Lcode_16533
    sxth	r2, r5
    movs	r3, #0
    str	r2, [sp, #12]
    mov	r0, r8
    ldr	r1, [sp, #80]
    str.w	ip, [sp, #20]
    mov	r2, r9
    str	r6, [sp, #0]
    str	r4, [sp, #4]
    str	r3, [sp, #8]
    bl vocoder_code_16184
    ldr.w	ip, [sp, #20]
    adds	r7, r7, r5
.Lcode_16533:
    rsb	r7, r7, fp
    mov.w	r3, #4294967295
    adds	r7, #1
    sxth	r7, r7
    str	r3, [sp, #8]
    mov	r0, r8
    ldr	r1, [sp, #80]
    str	r6, [sp, #0]
    mov	r2, r9
    mov	r3, ip
    str	r4, [sp, #4]
    str	r7, [sp, #12]
    bl vocoder_code_16184
.Lcode_16546:
    add	sp, #44
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_16548:
    mov.w	fp, #166
    b.n .Lcode_16474
.Lcode_16550:
    rsb	r3, r3, #15
    asrs.w	r3, r0, r3
    asrs	r2, r3, #16
    b.n .Lcode_16465
.Lcode_16554:
    sxth.w	r9, r6
    add.w	r8, sp, #24
    add	r6, sp, #28
    sxth	r4, r4
    b.n .Lcode_16516
.Lcode_16559:
    negs	r2, r4
    smulbb	r2, r2, r6
    cmn.w	r4, #16
    add.w	r1, r1, r2, lsl #5
    ite	lt
    addlt.w	r3, r3, #1048576
    rsbge	r3, sl, r3
    cmp	r5, #0
    str	r1, [sp, #24]
    blt.n .Lcode_16572
    movs	r2, #0
    mov	r7, r2
    b.n .Lcode_16490
.Lcode_16572:
    adds	r1, r5, #1
    movs	r2, #0
    add.w	ip, ip, r1, lsl #16
    mov	r7, r2
    b.n .Lcode_16490

.section .text.vocoder_rx_136,"ax",%progbits
.balign 4
.global vocoder_code_16577
.hidden vocoder_code_16577
vocoder_code_16577:
.global asm_synthesize_harmonic_series
.type asm_synthesize_harmonic_series, %function
.thumb_func
asm_synthesize_harmonic_series:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #140
    ldr	r5, [sp, #200]
    ldrsh.w	r4, [sp, #188]
    str	r0, [sp, #40]
    cmp	r5, #0
    str	r4, [sp, #28]
    str	r1, [sp, #32]
    ldrsh.w	r4, [sp, #192]
    str	r2, [sp, #36]
    str	r3, [sp, #44]
    ble.w .Lcode_16775
    ldr	r5, [sp, #196]
    mul.w	r1, r4, r4
    mul.w	r3, r5, r5
    lsls	r1, r1, #1
    lsls	r3, r3, #1
    mov	fp, r1
    mov	r2, r3
    cmp.w	fp, #0
    it	lt
    mvnlt.w	fp, fp
    itt	ne
    clzne	fp, fp
    subne.w	fp, fp, #1
    cmp	r2, #0
    it	lt
    mvnlt	r2, r2
    itt	ne
    clzne	r2, r2
    subne	r2, #1
    sxth.w	fp, fp
    sxth	r2, r2
    lsl.w	r1, r1, fp
    lsls	r3, r2
    smultt	r3, r1, r3
    lsls	r3, r3, #1
    str	r3, [sp, #20]
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    mul.w	r4, r4, r5
    str	r3, [sp, #24]
    lsls	r4, r4, #1
    rsb	fp, fp, #22
    ldr.w	lr, [sp, #24]
    str	r4, [sp, #16]
    mov	r3, r4
    rsb	fp, r2, fp
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    mov.w	r8, r5, lsl #16
    add.w	r9, sp, #52
    add.w	sl, sp, #92
    movs	r5, #0
    add	r4, sp, #132
    rsb	fp, lr, fp
    str	r3, [sp, #12]
    b.n .Lcode_16667
.Lcode_16645:
    sub.w	r1, r3, #15
    lsls.w	r1, r2, r1
.Lcode_16647:
    rsb	r1, r8, r1
    cmp	r1, #0
    blt.n .Lcode_16662
    mov	r0, r8
    cmp	r0, #0
    it	lt
    mvnlt	r0, r0
    itt	ne
    clzne	r0, r0
    subne	r0, #1
    sxth	r0, r0
    rsb	r3, r0, #15
    lsl.w	r0, r8, r0
    str	r3, [sp, #132]
    lsrs	r0, r0, #16
.Lcode_16662:
    ldr.w	lr, [sp, #200]
    strh.w	r0, [r9], #2
    cmp	r5, lr
    strh.w	r3, [sl], #2
    beq.n .Lcode_16757
.Lcode_16667:
    ldr	r0, [sp, #184]
    ldr	r1, [sp, #196]
    ldr.w	lr, [sp, #28]
    str.w	fp, [sp, #132]
    adds	r5, #1
    rsb	r7, r0, r5, lsl #16
    mov	r6, r7
    cmp	r6, #0
    it	lt
    mvnlt	r6, r6
    itt	ne
    clzne	r6, r6
    subne	r6, #1
    sxth	r6, r6
    lsls	r7, r6
    asrs	r7, r7, #16
    mul.w	r7, r7, r1
    lsls	r7, r7, #1
    mov	r3, r7
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsls	r7, r3
    lsrs	r7, r7, #16
    smulbb	r2, r7, lr
    lsls	r2, r2, #1
    ldr.w	lr, [sp, #24]
    ldr	r1, [sp, #20]
    str	r4, [sp, #0]
    rsb	r6, r6, #31
    subs	r6, r6, r3
    mov	r3, r2
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsl.w	r0, r1, lr
    lsls	r2, r3
    subs	r1, r6, #4
    asrs	r2, r2, #16
    subs	r3, r1, r3
    asrs	r0, r0, #16
    mov	r1, fp
    bl vocoder_code_6611
    mov	r1, r4
    lsls	r0, r0, #16
    bl vocoder_code_6932
    ldr	r1, [sp, #16]
    ldr	r3, [sp, #12]
    str	r4, [sp, #0]
    mov	lr, r0
    cmp.w	lr, #0
    it	lt
    mvnlt.w	lr, lr
    itt	ne
    clzne	lr, lr
    subne.w	lr, lr, #1
    sxth.w	lr, lr
    lsl.w	r2, r0, lr
    lsls.w	r0, r1, r3
    ldr	r3, [sp, #132]
    rsb	lr, lr, r3
    ldr	r3, [sp, #12]
    str.w	lr, [sp, #132]
    asrs	r2, r2, #16
    rsb	r1, r3, #11
    asrs	r0, r0, #16
    mov	r3, lr
    bl vocoder_code_6611
    ldr	r3, [sp, #132]
    str	r4, [sp, #0]
    sxth	r2, r0
    mov	r1, r6
    lsls	r0, r7, #16
    bl vocoder_code_6721
    ldr	r3, [sp, #132]
    uxth	r0, r0
    lsls	r2, r0, #16
    cmp	r3, #14
    bgt.w .Lcode_16645
    rsb	r1, r3, #15
    asrs.w	r1, r2, r1
    b.n .Lcode_16647
.Lcode_16757:
    ldrsh.w	r5, [sp, #180]
    ldr.w	r9, [sp, #40]
    ldr.w	r8, [sp, #44]
    ldr	r7, [sp, #176]
    mov.w	r6, lr, lsl #1
    movs	r4, #0
.Lcode_16763:
    add	r0, sp, #52
    add	r3, sp, #92
    ldrsh	r1, [r0, r4]
    ldrsh	r2, [r3, r4]
    str	r7, [sp, #0]
    mov	r0, r9
    mov	r3, r8
    adds	r4, #2
    str	r5, [sp, #4]
    bl vocoder_code_16416
    cmp	r4, r6
    bne.n .Lcode_16763
.Lcode_16775:
    ldr	r4, [sp, #200]
    add	r5, sp, #136
    subs	r3, r4, #1
    add.w	r3, r5, r3, lsl #1
    ldr	r4, [sp, #32]
    ldrh.w	r2, [r3, #-84]
    ldr	r5, [sp, #36]
    ldrh.w	r3, [r3, #-44]
    strh	r2, [r4, #0]
    strh	r3, [r5, #0]
    add	sp, #140
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}

.section .text.vocoder_rx_137,"ax",%progbits
.balign 4
.global vocoder_code_16787
.hidden vocoder_code_16787
vocoder_code_16787:
.global asm_synthesize_frame_transition
.type asm_synthesize_frame_transition, %function
.thumb_func
asm_synthesize_frame_transition:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub.w	sp, sp, #796
    mov	r7, r3
    ldr	r5, [sp, #832]
    ldr	r3, [r3, #8]
    ldr	r4, [r5, #8]
    str	r0, [sp, #32]
    tst.w	r4, #1431655765
    ite	eq
    moveq	r5, #0
    movne	r5, #1
    mov.w	r4, r1, lsl #16
    tst.w	r3, #1431655765
    ite	eq
    moveq.w	fp, #0
    movne.w	fp, #1
    mov	r6, r2
    str	r5, [sp, #40]
    mov	r3, r4
    add	r5, sp, #792
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r2, r3
    rsb	r3, r2, #15
    str.w	r3, [r5, #-12]!
    ldrh.w	lr, [r6, #340]
    ldr.w	ip, [r6, #344]
    str	r4, [sp, #44]
    lsls.w	r2, r4, r2
    mov	sl, r1
    asrs	r2, r2, #16
    movs	r1, #1
    mov.w	r0, #1073741824
    str	r5, [sp, #0]
    strh.w	lr, [sp, #788]
    strh.w	ip, [sp, #790]
    bl vocoder_code_6721
    ldr.w	r4, [r6, #336]
    ldr	r5, [sp, #832]
    str	r4, [sp, #56]
    ldrh	r5, [r5, #12]
    ldr	r4, [sp, #40]
    str	r5, [sp, #72]
    uxth	r0, r0
    str	r0, [sp, #68]
    ldrh	r3, [r7, #12]
    cbnz	r4, .Lcode_16868
    cmp.w	fp, #0
    beq.w .Lcode_17373
    sxth	r1, r3
    mul.w	r1, r1, sl
    ldr	r4, [sp, #56]
    ldr	r5, [sp, #40]
    str	r3, [sp, #72]
    sbfx	r1, r1, #3, #28
    adds	r1, r4, r1
    mov	r2, r1
    cmp	r2, #0
    it	lt
    mvnlt	r2, r2
    itt	ne
    clzne	r2, r2
    subne	r2, #1
    sxth	r2, r2
    lsls	r1, r2
    rsb	r2, r2, #15
    cmp	r2, #14
    str	r5, [sp, #96]
    str	r5, [sp, #100]
    str	r5, [sp, #92]
    bgt.n .Lcode_16891
.Lcode_16862:
    rsb	r2, r2, #15
    asrs.w	r2, r1, r2
    asrs	r4, r2, #16
    str	r2, [sp, #52]
    str	r4, [sp, #48]
    b.n .Lcode_16896
.Lcode_16868:
    cmp.w	fp, #0
    bne.w .Lcode_17266
    ldr	r4, [sp, #72]
    ldr	r5, [sp, #56]
    str.w	fp, [sp, #96]
    sxth	r1, r4
    mul.w	r1, r1, sl
    sbfx	r1, r1, #3, #28
    adds	r1, r5, r1
    mov	r3, r1
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsls	r1, r3
    rsb	r2, r3, #15
    str.w	fp, [sp, #100]
    str.w	fp, [sp, #92]
.Lcode_16889:
    cmp	r2, #14
    ble.n .Lcode_16862
.Lcode_16891:
    subs	r2, #15
    lsls.w	r2, r1, r2
    asrs	r5, r2, #16
    str	r2, [sp, #52]
    str	r5, [sp, #48]
.Lcode_16896:
    add	r5, sp, #108
    mov	r1, r6
    movs	r2, #168
    mov	r0, r5
    bl vocoder_code_2496
    add	r0, sp, #444
    movs	r1, #0
    movs	r2, #168
    bl vocoder_code_2535
    ldr	r4, [sp, #40]
    cmp	r4, #0
    beq.w .Lcode_17323
    ldr	r4, [sp, #48]
    cmp	r4, #0
    ble.w .Lcode_17323
    ldr	r4, [sp, #832]
    ldr	r0, [sp, #56]
    ldr	r2, [sp, #92]
    ldr.w	lr, [sp, #72]
    ldrsh.w	r3, [r4, #12]
    ldrsh.w	r4, [r6, #462]
    str	r0, [sp, #8]
    add.w	r1, r6, #464
    str	r1, [sp, #60]
    ldr	r0, [sp, #60]
    str.w	sl, [sp, #20]
    sxth	r1, r2
    sxth.w	r2, lr
    stmia.w	sp, {r0, r4}
    str	r2, [sp, #16]
    ldr	r4, [sp, #48]
    str	r1, [sp, #12]
    add	r2, sp, #784
    mov	r0, r5
    add	r1, sp, #788
    adds	r2, #6
    str	r4, [sp, #24]
    bl vocoder_code_16577
.Lcode_16934:
    sxth.w	r4, sl
    cmp	r4, #0
    str	r4, [sp, #64]
    ldrh.w	r3, [sp, #780]
    ble.n .Lcode_16977
    ldr	r4, [sp, #68]
    ldr	r0, [r5, #0]
    sxth	r3, r3
    add.w	r8, sl, #4294967295
    lsls	r1, r4, #16
    rsb	r9, r3, #0
    uxth.w	r8, r8
    lsls.w	r2, r1, r3
    asr.w	r9, r1, r9
    add.w	r8, r8, #1
    movs	r1, #0
    cmp	r3, r1
    it	ge
    movge	r9, r2
    mov.w	r8, r8, lsl #2
    mov	lr, r1
    ldr	r4, [sp, #32]
    b.n .Lcode_16958
.Lcode_16957:
    ldr	r0, [r5, r1]
.Lcode_16958:
    mov.w	r2, lr, lsr #16
    sxth	r2, r2
    mov.w	ip, r0, asr #31
    asrs	r3, r2, #31
    mul.w	ip, r2, ip
    mla	ip, r0, r3, ip
    umull	r2, r3, r2, r0
    add	r3, ip
    lsrs	r2, r2, #15
    orr.w	r2, r2, r3, lsl #17
    subs	r0, r0, r2
    ldr	r3, [r4, r1]
    qadd	r0, r3, r0
    str	r0, [r4, r1]
    qadd	lr, r9, lr
    adds	r1, #4
    cmp	r1, r8
    bne.n .Lcode_16957
    str	r4, [sp, #32]
.Lcode_16977:
    movs	r1, #0
    mov.w	r2, #336
    mov	r0, r5
    bl vocoder_code_2535
    ldrsh.w	lr, [r7, #12]
    ldr.w	r4, [r7, #128]
    ldrsh.w	r0, [sp, #836]
    str	r4, [sp, #80]
    mov.w	r2, lr, lsl #16
    ldr.w	r4, [r7, #132]
    movw	r1, #65532
    lsls	r0, r0, #16
    mov	ip, r2
    mov	r3, r1
    cmp.w	ip, #0
    it	lt
    mvnlt.w	ip, ip
    itt	ne
    clzne	ip, ip
    subne.w	ip, ip, #1
    mov.w	lr, lr, lsl #5
    sxth.w	ip, ip
    str	r4, [sp, #84]
    mov	r8, r0
    ldrsh.w	r4, [r7, #4]
    cmp.w	r8, #0
    it	lt
    mvnlt.w	r8, r8
    itt	ne
    clzne	r8, r8
    subne.w	r8, r8, #1
    lsl.w	r2, r2, ip
    sxth.w	r8, r8
    rsb	lr, lr, #0
    movt	r1, #65535
    movt	r3, #65535
    rsb	r3, ip, r3
    lsl.w	r0, r0, r8
    ubfx	ip, lr, #5, #16
    rsb	r1, r8, r1
    add.w	lr, sp, #784
    asrs	r2, r2, #16
    str	r4, [sp, #88]
    str.w	lr, [sp]
    str.w	ip, [sp, #36]
    bl vocoder_code_6721
    ldr	r3, [sp, #784]
    ldr.w	ip, [sp, #36]
    add.w	r4, r7, #16
    cmp	r3, #14
    add.w	r8, r6, #462
    add.w	r9, r6, #350
    str	r4, [sp, #76]
    mov.w	lr, r0, lsl #16
    ble.w .Lcode_17347
    subs	r3, #15
    lsl.w	lr, lr, r3
.Lcode_17034:
    ldr	r4, [sp, #80]
    ldr	r0, [sp, #60]
    str	r4, [sp, #0]
    ldr	r4, [sp, #76]
    str	r4, [sp, #4]
    ldr	r4, [sp, #84]
    add.w	lr, lr, #32768
    mov.w	lr, lr, asr #16
    str	r4, [sp, #8]
    ldr	r4, [sp, #88]
    str.w	lr, [sp, #16]
    sxth.w	r2, ip
    mov.w	lr, #2
    mov	r1, r8
    mov	r3, r9
    str	r4, [sp, #12]
    str.w	lr, [sp, #20]
    bl vocoder_code_16279
    cmp	r0, #0
    bne.w .Lcode_17240
.Lcode_17054:
    cmp.w	fp, #0
    beq.w .Lcode_17183
    ldr	r4, [sp, #100]
    cmp	r4, #0
    beq.w .Lcode_17350
    ldr.w	r3, [r6, #336]
    ldr	r4, [sp, #832]
    ldrh	r2, [r4, #12]
    lsls	r4, r3, #1
    str	r4, [sp, #56]
    ldr	r4, [sp, #52]
    rsb	ip, r3, r4
    ldr	r4, [sp, #56]
    cmp.w	r4, #65536
    blt.n .Lcode_17107
    sub.w	r4, r4, #65536
    cmp.w	r4, #2147483648
    str	r4, [sp, #56]
    beq.w .Lcode_17383
    negs	r3, r4
    asrs	r1, r3, #1
.Lcode_17075:
    lsls	r0, r2, #16
    mov	r2, r0
    cmp	r2, #0
    it	lt
    mvnlt	r2, r2
    itt	ne
    clzne	r2, r2
    subne	r2, #1
    sxth.w	lr, r2
    lsl.w	r0, r0, lr
    str	r0, [sp, #48]
    ldr	r4, [sp, #48]
    str.w	ip, [sp, #36]
    movw	r3, #65532
    mov	r2, r1
    movt	r3, #65535
    cmp	r2, #0
    it	lt
    mvnlt	r2, r2
    itt	ne
    clzne	r2, r2
    subne	r2, #1
    sxth	r2, r2
    lsls.w	r0, r1, r2
    rsb	r3, lr, r3
    rsb	r1, r2, #15
    add.w	lr, r6, #344
    asrs	r2, r4, #16
    str.w	lr, [sp]
    bl vocoder_code_6721
    ldr.w	ip, [sp, #36]
    strh.w	r0, [r6, #340]
.Lcode_17107:
    ldr	r4, [sp, #72]
    sxth	r3, r4
    ldr	r4, [sp, #92]
    lsls	r1, r4, #17
    ldr	r4, [sp, #56]
    lsls	r2, r3, #16
    lsls	r3, r3, #17
    lsrs	r1, r1, #16
    str.w	r4, [r6, #336]
    cmp.w	r2, r3, asr #1
    ldr	r4, [sp, #56]
    str	r1, [sp, #92]
    it	ne
    mvnne.w	r3, #2147483648
    add.w	ip, r4, ip, lsl #1
    it	ne
    eorne.w	r3, r3, r2, asr #31
    lsrs	r3, r3, #16
    mov.w	r4, ip, asr #16
    str.w	ip, [sp, #52]
    str	r3, [sp, #72]
    str	r4, [sp, #48]
.Lcode_17129:
    ldrsh.w	r2, [r7, #12]
    ldr.w	lr, [r7, #132]
    ldr.w	r3, [r7, #128]
    ldrsh.w	ip, [r7, #4]
    ldr	r4, [sp, #76]
    str.w	lr, [sp, #8]
    lsls	r2, r2, #5
    negs	r2, r2
    mov.w	lr, #0
    ubfx	r2, r2, #5, #16
    str	r3, [sp, #0]
    sxth	r2, r2
    mov	r1, r8
    mov	r3, r9
    str.w	lr, [sp, #16]
    ldr	r0, [sp, #60]
    str	r4, [sp, #4]
    mov.w	lr, #1
    str.w	ip, [sp, #12]
    str.w	lr, [sp, #20]
    bl vocoder_code_16279
    ldrsh.w	r0, [r6, #462]
    ldr	r4, [sp, #60]
    ldrsh.w	r1, [r6, #340]
    ldrsh.w	r2, [r6, #344]
    ldrsh.w	r3, [r7, #12]
    str	r0, [sp, #4]
    mov	r0, r5
    str	r4, [sp, #0]
    bl vocoder_code_16416
    ldr	r4, [sp, #48]
    cmp	r4, #0
    ble.n .Lcode_17183
    ldr	r4, [sp, #60]
    ldrsh.w	r1, [r6, #462]
    ldrsh.w	r3, [r7, #12]
    str	r4, [sp, #0]
    ldr	r4, [sp, #92]
    str	r1, [sp, #4]
    sxth	r0, r4
    ldr	r4, [sp, #72]
    str	r0, [sp, #12]
    sxth	r2, r4
    ldr	r4, [sp, #56]
    str	r2, [sp, #16]
    str	r4, [sp, #8]
    add	r2, sp, #784
    ldr	r4, [sp, #48]
    str.w	sl, [sp, #20]
    mov	r0, r5
    add	r1, sp, #788
    adds	r2, #6
    str	r4, [sp, #24]
    bl vocoder_code_16577
.Lcode_17183:
    ldr	r4, [sp, #52]
    ldr.w	lr, [sp, #48]
    ldrh.w	r2, [sp, #780]
    sub.w	r3, r4, lr, lsl #16
    ldr	r4, [sp, #64]
    str.w	r3, [r6, #336]
    cmp	r4, #0
    ble.n .Lcode_17227
    ldr	r4, [sp, #68]
    sxth	r2, r2
    add.w	lr, sl, #4294967295
    lsls	r3, r4, #16
    rsb	ip, r2, #0
    uxth.w	lr, lr
    lsls.w	r0, r3, r2
    asr.w	ip, r3, ip
    movs	r1, #0
    add.w	lr, lr, #1
    cmp	r2, r1
    it	ge
    movge	ip, r0
    ldr	r3, [r5, #0]
    ldr	r4, [sp, #32]
    mov.w	lr, lr, lsl #2
    mov	r0, r1
    b.n .Lcode_17210
.Lcode_17209:
    ldr	r3, [r5, r1]
.Lcode_17210:
    lsrs	r2, r0, #16
    sxth	r2, r2
    asrs	r7, r3, #31
    mov.w	r8, r2, asr #31
    mul.w	r7, r2, r7
    mla	r7, r3, r8, r7
    umull	r2, r3, r2, r3
    adds	r3, r7, r3
    lsrs	r2, r2, #15
    ldr	r7, [r4, r1]
    qadd	r0, ip, r0
    orr.w	r2, r2, r3, lsl #17
    qadd	r2, r2, r7
    str	r2, [r4, r1]
    adds	r1, #4
    cmp	r1, lr
    bne.n .Lcode_17209
.Lcode_17227:
    add.w	r1, r5, sl, lsl #2
    mov	r0, r6
    movs	r2, #168
    bl vocoder_code_2496
    ldr	r5, [sp, #40]
    cmp	r5, #0
    bne.n .Lcode_17326
    cmp.w	fp, #0
    bne.n .Lcode_17326
    strh.w	fp, [r6, #340]
    str.w	fp, [r6, #344]
.Lcode_17238:
    add.w	sp, sp, #796
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_17240:
    ldr	r4, [sp, #60]
    ldrsh.w	r3, [r7, #14]
    ldrsh.w	lr, [r7, #12]
    ldrsh.w	r0, [r6, #462]
    str	r4, [sp, #0]
    ldr	r4, [sp, #44]
    str	r0, [sp, #4]
    add.w	r3, r4, r3, lsl #8
    sub.w	r2, r3, #1048576
    mov	r3, r2
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    uxth	r1, r3
    sxth	r3, r1
    lsls.w	r3, r2, r3
    rsb	r2, r1, #15
    sxth	r2, r2
    asrs	r1, r3, #16
    mov	r0, r5
    mov	r3, lr
    bl vocoder_code_16416
    b.n .Lcode_17054
.Lcode_17266:
    movw	r0, #19660
    sxth	r2, r3
    sxth	r1, r5
    smulbb	r3, r3, r0
    cmp.w	r1, r3, asr #15
    mov	r0, r2
    bge.n .Lcode_17287
    add.w	r3, r2, r2, lsl #1
    add.w	r3, r3, r3, lsl #4
    add.w	r3, r3, r3, lsl #8
    sbfx	r3, r3, #15, #16
    cmp	r1, r3
    ble.n .Lcode_17287
    cmp.w	r1, #16384
    bge.n .Lcode_17287
    movs	r5, #0
    movs	r4, #1
    asrs	r2, r2, #1
    str	r5, [sp, #96]
    str	r4, [sp, #100]
    b.n .Lcode_17300
.Lcode_17287:
    ldr	r5, [sp, #72]
    movw	r3, #13653
    smulbb	r3, r5, r3
    cmp.w	r0, r3, asr #15
    ble.n .Lcode_17297
    add.w	r3, r1, r1, lsl #2
    cmp.w	r0, r3, asr #3
    bge.n .Lcode_17379
    cmp.w	r0, #16384
    blt.n .Lcode_17385
.Lcode_17297:
    movs	r4, #0
    str	r4, [sp, #96]
    str	r4, [sp, #100]
.Lcode_17300:
    lsls	r1, r1, #16
    lsls	r2, r2, #16
    asrs	r3, r1, #1
    add.w	r3, r3, r2, asr #1
    asrs	r3, r3, #16
    mul.w	r3, r3, sl
    ldr	r5, [sp, #56]
    sbfx	r3, r3, #3, #28
    adds	r3, r5, r3
    subs	r1, r2, r1
    lsrs	r1, r1, #16
    mov	r2, r3
    cmp	r2, #0
    it	lt
    mvnlt	r2, r2
    itt	ne
    clzne	r2, r2
    subne	r2, #1
    sxth	r2, r2
    str	r1, [sp, #92]
    lsls.w	r1, r3, r2
    rsb	r2, r2, #15
    b.n .Lcode_16889
.Lcode_17323:
    add.w	r4, r6, #464
    str	r4, [sp, #60]
    b.n .Lcode_16934
.Lcode_17326:
    ldr	r4, [sp, #44]
    ldr	r3, [sp, #44]
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    uxth	r3, r3
    sxth	r0, r3
    lsls.w	r0, r4, r0
    rsb	r1, r3, #15
    ldrsh.w	r2, [sp, #788]
    ldrsh.w	r3, [sp, #790]
    asrs	r0, r0, #16
    sxth	r1, r1
    add.w	r4, r6, #344
    str	r4, [sp, #0]
    bl vocoder_code_6664
    strh.w	r0, [r6, #340]
    b.n .Lcode_17238
.Lcode_17347:
    rsb	r3, r3, #15
    asr.w	lr, lr, r3
    b.n .Lcode_17034
.Lcode_17350:
    ldr	r4, [sp, #96]
    cmp	r4, #0
    beq.w .Lcode_17129
    ldr.w	r3, [r6, #336]
    asrs	r4, r3, #1
    str	r4, [sp, #56]
    ldr	r4, [sp, #52]
    subs	r3, r4, r3
    ldr	r4, [sp, #92]
    sbfx	r1, r4, #1, #15
    ldr	r4, [sp, #72]
    sbfx	r2, r4, #1, #15
    ldr	r4, [sp, #56]
    add.w	r3, r4, r3, asr #1
    str.w	r4, [r6, #336]
    uxth	r1, r1
    uxth	r2, r2
    asrs	r4, r3, #16
    str	r3, [sp, #52]
    str	r1, [sp, #92]
    str	r2, [sp, #72]
    str	r4, [sp, #48]
    b.n .Lcode_17129
.Lcode_17373:
    str.w	fp, [sp, #48]
    str.w	fp, [sp, #52]
    str.w	fp, [sp, #92]
    str.w	fp, [sp, #100]
    str.w	fp, [sp, #96]
    b.n .Lcode_16896
.Lcode_17379:
    movs	r5, #0
    str	r5, [sp, #96]
    str	r5, [sp, #100]
    b.n .Lcode_17300
.Lcode_17383:
    mvn.w	r1, #3221225472
    b.n .Lcode_17075
.Lcode_17385:
    lsls	r2, r0, #17
    movs	r4, #1
    movs	r5, #0
    asrs	r2, r2, #16
    str	r4, [sp, #96]
    str	r5, [sp, #100]
    b.n .Lcode_17300

.section .text.vocoder_rx_138,"ax",%progbits
.balign 4
.global vocoder_code_17392
.hidden vocoder_code_17392
vocoder_code_17392:
.global asm_filter_log_spectrum
.type asm_filter_log_spectrum, %function
.thumb_func
asm_filter_log_spectrum:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #212
    movs	r3, #0
    mov	r4, r2
    mov	r5, r0
    sxth	r2, r2
    add	r0, sp, #40
    mov	r6, r1
    strh.w	r3, [sp, #38]
    bl vocoder_code_2496
    subs	r3, r4, #1
    cmp	r4, #63
    ldrh.w	r2, [r6, r3, lsl #1]
    bgt.n .Lcode_17419
    movs	r6, #0
    add.w	r3, sp, r6
    add.w	r1, r4, #19
    add.w	r1, r3, r1, lsl #1
    lsls	r2, r2, #16
    mov	r3, r4
    movt	r6, #1477
.Lcode_17413:
    adds	r3, #1
    qsub	r2, r2, r6
    asrs	r0, r2, #16
    cmp	r3, #64
    strh.w	r0, [r1, #2]!
    bne.n .Lcode_17413
.Lcode_17419:
    cmp	r4, #45
    ble.n .Lcode_17472
    add.w	r3, sp, #166
    mov.w	r8, r4, lsl #1
    add	r0, sp, #256
    mov	r2, r3
    rsb	r0, r8, r0
.Lcode_17426:
    ldrh.w	r1, [r3, #-2]!
    cmp	r3, r0
    strh.w	r1, [r2, #2]!
    bne.n .Lcode_17426
.Lcode_17430:
    add.w	r2, sp, #38
    mov	lr, sp
    sub.w	r0, sp, #2
    add	r3, sp, #36
.Lcode_17434:
    ldrh.w	r1, [r2, #2]!
    strh.w	r1, [r3], #-2
    cmp	r3, r0
    bne.n .Lcode_17434
    add	r3, sp, #0
    add.w	r9, r3, r8
    mov	ip, r5
.Lcode_17441:
    ldr	r6, .Lliteral_17474
    add.w	r2, lr, #46
    sub.w	r7, lr, #40
    mov	r3, lr
    movs	r0, #0
.Lcode_17446:
    ldrh.w	fp, [r2, #-4]
    ldrh.w	r1, [r6], #2
    ldrh.w	sl, [r3, #38]
    smulbb	fp, fp, r1
    subs	r3, #4
    add.w	r0, r0, fp, lsl #1
    smulbb	r1, r1, sl
    adds	r2, #4
    cmp	r3, r7
    sub.w	r0, r0, r1, lsl #1
    bne.n .Lcode_17446
    and.w	r3, r0, #2147483648
    lsls	r0, r0, #2
    add.w	r0, r3, r0, lsr #1
    add.w	lr, lr, #2
    asrs	r0, r0, #16
    cmp	lr, r9
    strh.w	r0, [ip], #2
    bne.n .Lcode_17441
    rsb	r2, r4, #56
    sxth	r2, r2
    add.w	r0, r5, r8
    movs	r1, #0
    bl vocoder_code_2535
    add	sp, #212
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_17472:
    mov.w	r8, r4, lsl #1
    b.n .Lcode_17430
.Lliteral_17474:
    .word vocoder_data_20766

.section .text.vocoder_rx_139,"ax",%progbits
.balign 4
.global vocoder_code_17475
.hidden vocoder_code_17475
vocoder_code_17475:
.global asm_apply_adaptive_spectral_attenuation
.type asm_apply_adaptive_spectral_attenuation, %function
.thumb_func
asm_apply_adaptive_spectral_attenuation:
    stmdb	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr}
    sub	sp, #172
    mov	r4, r0
    ldr	r6, [r1, #0]
    str	r2, [sp, #32]
    mov	r5, r1
    add	r0, sp, #156
    mov	r1, r4
    ldr	r2, [sp, #212]
    str	r3, [sp, #36]
    bl vocoder_code_7556
    mov.w	fp, r6, lsl #1
    ldr	r6, [sp, #156]
    lsrs	r0, r0, #16
    add	r6, fp
    sxth	r7, r0
    str	r7, [sp, #28]
    str	r6, [sp, #156]
    cmp	r7, #0
    beq.n .Lcode_17533
    ldr	r7, [sp, #208]
    ldr.w	r9, .Lliteral_17876
    str	r4, [sp, #8]
    mov.w	r8, r7, lsl #10
    ldr	r7, [sp, #212]
    str	r5, [sp, #24]
    movs	r2, #0
    movs	r3, #0
    mov.w	r8, r8, asr #4
    strd	r2, r3, [sp, #16]
    add.w	sl, r4, r7, lsl #1
    add.w	ip, sp, #44
    mov	r7, r0
    mov	r3, r4
    mov	r2, r8
.Lcode_17510:
    asrs	r1, r2, #16
    ldrh.w	r0, [r3], #2
    ldrh.w	r1, [r9, r1, lsl #1]
    smulbb	r0, r0, r0
    ldrd	r4, r5, [sp, #16]
    ubfx	r0, r0, #15, #16
    smulbb	r0, r1, r0
    lsls	r0, r0, #1
    adds	r4, r4, r0
    adc.w	r5, r5, r0, asr #31
    cmp	r3, sl
    strh.w	r1, [ip], #2
    strd	r4, r5, [sp, #16]
    add	r2, r8
    bne.n .Lcode_17510
    ldrd	r2, r3, [sp, #16]
    orrs	r3, r2
    mov	r0, r7
    ldr	r4, [sp, #8]
    ldr	r5, [sp, #24]
    bne.w .Lcode_17796
    mov.w	r8, #0
    b.n .Lcode_17535
.Lcode_17533:
    ldr.w	fp, [sp, #28]
    mov	r8, fp
.Lcode_17535:
    ldr	r3, [sp, #36]
    ldr	r7, [sp, #32]
    ldrsh.w	r2, [r3]
    ldrh	r1, [r7, #0]
    movw	r3, #31130
    smulbb	r1, r1, r3
    cmp	r2, r6
    ite	ge
    movge	r3, r2
    movlt	r3, r6
    lsls	r1, r1, #1
    subs	r7, r2, r3
    bmi.w .Lcode_17833
    lsls.w	r2, r1, r7
.Lcode_17549:
    movw	r7, #26214
    smulbb	r7, r0, r7
    subs	r1, r6, #4
    lsls	r7, r7, #1
    subs	r1, r1, r3
    bmi.w .Lcode_17829
    lsls	r7, r1
.Lcode_17556:
    adds	r2, r2, r7
    mov	r1, r2
    cmp	r1, #0
    it	lt
    mvnlt	r1, r1
    itt	ne
    clzne	r1, r1
    subne	r1, #1
    sxth	r1, r1
    lsls	r2, r1
    beq.n .Lcode_17570
    subs	r1, r3, r1
    cmn.w	r1, #16
    bge.w .Lcode_17793
.Lcode_17570:
    movw	r1, #65519
    movw	r2, #32767
.Lcode_17572:
    ldr	r7, [sp, #32]
    ldr	r3, [sp, #36]
    strh	r2, [r7, #0]
    ldr	r7, [sp, #28]
    strh	r1, [r3, #0]
    cmp	r7, #0
    ble.w .Lcode_17788
    mov.w	r9, r6, lsl #1
    smulbb	r0, r0, r0
    add.w	r9, r9, #1
    lsls	r0, r0, #1
    sub.w	r3, r9, r6, lsl #1
    asrs	r0, r3
    cmp.w	r8, #0
    beq.w .Lcode_17790
    mul.w	r7, r8, r8
    lsls	r7, r7, #1
    rsbs	r3, r9, fp, lsl #1
    bmi.w .Lcode_17864
    lsls	r7, r3
.Lcode_17592:
    rsb	ip, r7, r0
    asrs.w ip, ip, #16
    beq.w .Lcode_17842
    adds	r7, r0, r7
    asrs	r7, r7, #16
.Lcode_17597:
    ldr	r2, [sp, #28]
    ldr	r3, [sp, #208]
    mul.w	r1, ip, r2
    sbfx	r1, r1, #15, #16
    mul.w	r1, r1, r3
    lsls	r1, r1, #1
    mov	sl, r1
    cmp.w	sl, #0
    it	lt
    mvnlt.w	sl, sl
    itt	ne
    clzne	sl, sl
    subne.w	sl, sl, #1
    sxth.w	sl, sl
    lsl.w	r1, r1, sl
    movw	r0, #5033
    asrs	r1, r1, #16
    movt	r0, #15568
    bl vocoder_code_6534
    uxth	r0, r0
    sxth	r2, r0
    mul.w	r2, r7, r2
    lsls	r2, r2, #1
    mov	r3, r2
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    add.w	r1, r6, r9
    lsls	r2, r3
    rsb	r1, r1, #4
    asrs	r2, r2, #16
    add	sl, r1
    str	r2, [sp, #16]
    bne.w .Lcode_17826
    negs	r7, r3
.Lcode_17636:
    ldr	r1, [sp, #28]
    mul.w	ip, r8, r1
    ubfx	ip, ip, #15, #16
    smulbb	ip, r0, ip
    sbfx	ip, ip, #15, #16
    mov.w	ip, ip, lsl #15
    mov	r3, ip
    cmp	r3, #0
    it	lt
    mvnlt	r3, r3
    itt	ne
    clzne	r3, r3
    subne	r3, #1
    sxth	r3, r3
    lsl.w	ip, ip, r3
    asrs.w ip, ip, #16
    bne.w .Lcode_17821
    rsb	fp, r3, #0
.Lcode_17654:
    ldr	r3, [r5, #0]
    ldr	r2, [sp, #212]
    str.w	ip, [sp, #12]
    asrs	r6, r2, #3
    adds	r2, r3, #1
    str	r2, [sp, #0]
    mov	r0, r4
    mov	r1, r4
    mov	r2, r6
    bl vocoder_code_7784
    cmp	r7, fp
    ldr.w	ip, [sp, #12]
    ble.w .Lcode_17836
    ldr	r3, [sp, #16]
    adds	r2, r7, #1
    mov.w	lr, r3, lsl #16
    rsb	sl, r2, fp
    mov.w	r3, #4294967295
.Lcode_17672:
    lsls	r1, r6, #1
    rsb	r8, r7, r2
    add	r7, sp, #44
    add.w	r9, r7, r1
    ldr	r7, [sp, #212]
    str	r4, [sp, #36]
    rsb	fp, fp, r2
    lsl.w	r0, lr, r3
    asr.w	r8, lr, r8
    subs	r6, r7, r6
    str.w	fp, [sp, #32]
    adds	r7, r4, r1
    ands.w	r8, r8, r3, asr #32
    it	cc
    movcc	r8, r0
    mov.w	fp, #1
    str	r2, [sp, #16]
    mov	r4, ip
    b.n .Lcode_17708
.Lcode_17691:
    mov	r3, r0
.Lcode_17692:
    rsb	r3, r3, #1
    asrs	r0, r3
.Lcode_17694:
    asrs	r0, r0, #16
    movw	r1, #19661
    cmp	r0, r1
    str.w	fp, [sp, #160]
    bgt.w .Lcode_17819
    cmp.w	r0, #8192
    it	lt
    movlt.w	r0, #8192
.Lcode_17702:
    ldrsh.w	r3, [r7]
    mul.w	r0, r0, r3
    sbfx	r0, r0, #15, #16
    subs	r6, #1
    strh.w	r0, [r7], #2
    beq.n .Lcode_17742
.Lcode_17708:
    ldrsh.w	r0, [r9], #2
    mul.w	r0, r4, r0
    lsls	r0, r0, #1
    cmp.w	sl, #0
    itet	lt
    ldrlt	r2, [sp, #32]
    lslge.w	r0, r0, sl
    asrlt	r0, r2
    subs.w	r0, r8, r0
    beq.n .Lcode_17691
    ldr	r3, [sp, #16]
    add	r1, sp, #160
    str	r3, [sp, #160]
    bl vocoder_code_6932
    ldrh	r2, [r7, #0]
    ldr	r3, [r5, #0]
    smultb	r0, r0, r2
    ldr	r2, [sp, #160]
    lsls	r0, r0, #1
    adds	r3, r2, r3
    add	r1, sp, #160
    str	r3, [sp, #160]
    bl vocoder_code_6932
    ldr	r3, [sp, #160]
    cmp	r3, #1
    ble.w .Lcode_17874
    ldrsh.w	r3, [r7]
    str.w	fp, [sp, #160]
    movw	r0, #19661
    mul.w	r0, r0, r3
    sbfx	r0, r0, #15, #16
    subs	r6, #1
    strh.w	r0, [r7], #2
    bne.n .Lcode_17708
.Lcode_17742:
    ldr	r2, [r5, #0]
    ldr	r4, [sp, #36]
    ldr	r7, [sp, #156]
    adds	r2, #1
    mov	r3, r5
    str	r2, [r5, #0]
    mov	r0, r4
    mov	r1, r4
    ldr	r2, [sp, #212]
    bl vocoder_code_7675
    ldr	r2, [sp, #212]
    add	r0, sp, #160
    mov	r1, r4
    bl vocoder_code_7556
    ldr	r2, [r5, #0]
    ldr	r3, [sp, #160]
    add.w	r3, r3, r2, lsl #1
    asrs	r2, r0, #16
    str	r3, [sp, #160]
    beq.n .Lcode_17788
    ldr	r1, [sp, #28]
    add.w	r8, sp, #164
    lsls	r0, r1, #16
    mov	r1, r7
    str.w	r8, [sp]
    bl vocoder_code_6721
    mov	r1, r8
    lsls	r0, r0, #16
    bl vocoder_code_6932
    ldr	r2, [sp, #212]
    asrs	r3, r0, #16
    mov	r1, r4
    mov	r0, r4
    bl vocoder_code_7545
    ldr	r2, [r5, #0]
    ldr	r3, [sp, #164]
    adds	r3, r2, r3
    cmp	r3, #0
    str	r3, [r5, #0]
    ble.n .Lcode_17788
    mov	r0, r4
    mov	r1, r4
    ldr	r2, [sp, #212]
    str	r6, [sp, #0]
    bl vocoder_code_7806
    str	r6, [r5, #0]
.Lcode_17788:
    add	sp, #172
    ldmia.w	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc}
.Lcode_17790:
    mov.w	ip, r0, asr #16
    mov	r7, ip
    b.n .Lcode_17597
.Lcode_17793:
    lsrs	r2, r2, #16
    uxth	r1, r1
    b.n .Lcode_17572
.Lcode_17796:
    ldrh.w	r7, [sp, #20]
    sxth	r3, r7
    cmp	r3, #0
    ldr	r7, [sp, #16]
    blt.n .Lcode_17861
    ldr	r1, [sp, #16]
    mov	r2, r3
.Lcode_17803:
    clz	r2, r2
    cmp	r2, #32
    itt	eq
    clzeq	r2, r1
    addeq	r2, #32
    subs.w	r1, r2, #33
    bmi.n .Lcode_17867
    lsls	r7, r1
    rsb	fp, r1, fp
.Lcode_17812:
    movw	r3, #32769
    mov.w	r8, r7, asr #16
    movt	r3, #65535
    cmn.w	r8, #32768
    it	eq
    moveq	r8, r3
    b.n .Lcode_17535
.Lcode_17819:
    movw	r0, #19661
    b.n .Lcode_17702
.Lcode_17821:
    add	fp, r6
    add	fp, sl
    add.w	fp, fp, #2
    rsb	fp, r3, fp
    b.n .Lcode_17654
.Lcode_17826:
    add.w	r7, r9, sl
    subs	r7, r7, r3
    b.n .Lcode_17636
.Lcode_17829:
    rsb	r1, r6, #4
    adds	r1, r3, r1
    asrs	r7, r1
    b.n .Lcode_17556
.Lcode_17833:
    subs	r2, r3, r2
    asrs.w	r2, r1, r2
    b.n .Lcode_17549
.Lcode_17836:
    ldr	r1, [sp, #16]
    add.w	r2, fp, #1
    mov.w	lr, r1, lsl #16
    subs	r3, r7, r2
    mov.w	sl, #4294967295
    b.n .Lcode_17672
.Lcode_17842:
    ldr	r7, [sp, #212]
    ldr	r3, [r5, #0]
    str.w	ip, [sp, #12]
    asrs	r6, r7, #3
    mov	r2, r6
    adds	r7, r3, #1
    mov	r0, r4
    mov	r1, r4
    str	r7, [sp, #0]
    bl vocoder_code_7784
    ldr.w	ip, [sp, #12]
    mov.w	lr, #0
    mov.w	r3, #4294967295
    movt	lr, #32767
    movs	r2, #1
    mov	fp, ip
    mov	r7, ip
    mov.w	sl, #4294967295
    b.n .Lcode_17672
.Lcode_17861:
    mvns	r2, r3
    mvns	r1, r7
    b.n .Lcode_17803
.Lcode_17864:
    sub.w	r3, r9, fp, lsl #1
    asrs	r7, r3
    b.n .Lcode_17592
.Lcode_17867:
    subs	r2, #1
    rsb	ip, r1, #0
    lsls	r3, r2
    lsr.w	r7, r7, ip
    orrs	r7, r3
    rsb	fp, r1, fp
    b.n .Lcode_17812
.Lcode_17874:
    beq.w .Lcode_17694
    b.n .Lcode_17692
    .byte 0x00
    .byte 0xbf
.Lliteral_17876:
    .word vocoder_data_19902


.section .text.vocoder_rx_168,"ax",%progbits
.balign 4
.global vocoder_code_19831
.hidden vocoder_code_19831
vocoder_code_19831:
.global asm_copy_frame_state
.type asm_copy_frame_state, %function
.thumb_func
asm_copy_frame_state:
    push	{r4, r5, r6, r7}
    mov	r3, r0
    ldrh	r2, [r1, #14]
    ldrh	r0, [r1, #0]
    ldrh.w	ip, [r1, #2]
    ldrh	r7, [r1, #4]
    ldrh	r6, [r1, #6]
    ldr	r5, [r1, #8]
    ldrh	r4, [r1, #12]
    strh	r0, [r3, #0]
    strh	r2, [r3, #14]
    adds	r1, #16
    add.w	r0, r3, #16
    movs	r2, #56
    strh.w	ip, [r3, #2]
    strh	r7, [r3, #4]
    strh	r6, [r3, #6]
    str	r5, [r3, #8]
    strh	r4, [r3, #12]
    pop	{r4, r5, r6, r7}
    b.w vocoder_code_2496
    .byte 0x00
    .byte 0xbf


.section .text.vocoder_rx_217,"ax",%progbits
.balign 4
.global vocoder_data_20766
.hidden vocoder_data_20766
vocoder_data_20766:
    .byte 0xeb
    .byte 0x47
    .byte 0xf9
    .byte 0x17
    .word 0x0a460e62
    .word 0x068a07fe
    .word 0x04cb0588
    .word 0x03c9043b


.section .note.GNU-stack,"",%progbits
