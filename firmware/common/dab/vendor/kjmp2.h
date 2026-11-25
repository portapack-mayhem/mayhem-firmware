/******************************************************************************
** kjmp2 -- a minimal MPEG-1/2 Audio Layer II decoder library                **
** version 1.1                                                               **
*******************************************************************************
** Copyright (C) 2006-2013 Martin J. Fiedler <martin.fiedler@gmx.net>        **
**                                                                           **
** This software is provided 'as-is', without any express or implied         **
** warranty. In no event will the authors be held liable for any damages     **
** arising from the use of this software.                                    **
**                                                                           **
** Permission is granted to anyone to use this software for any purpose,     **
** including commercial applications, and to alter it and redistribute it    **
** freely, subject to the following restrictions:                            **
**   1. The origin of this software must not be misrepresented; you must not **
**      claim that you wrote the original software. If you use this software **
**      in a product, an acknowledgment in the product documentation would   **
**      be appreciated but is not required.                                  **
**   2. Altered source versions must be plainly marked as such, and must not **
**      be misrepresented as being the original software.                    **
**   3. This notice may not be removed or altered from any source            **
**      distribution.                                                        **
******************************************************************************/

#ifndef __KJMP2_H__
#define __KJMP2_H__

#define KJMP2_MAX_FRAME_SIZE    1440  // the maximum size of a frame
#define KJMP2_SAMPLES_PER_FRAME 1152  // the number of samples per frame

// GENERAL WARNING: kjmp2 is *NOT* threadsafe!

// kjmp2_context_t: A kjmp2 context record. You don't need to use or modify
// any of the values inside this structure; just pass the whole structure
// to the kjmp2_* functions
typedef struct _kjmp2_context {
    int id;
    int V[2][1024];
    int Voffs;
} kjmp2_context_t;


// kjmp2_init: This function must be called once to initialize each kjmp2
// decoder instance.
extern void kjmp2_init(kjmp2_context_t *mp2);


// kjmp2_get_sample_rate: Returns the sample rate of a MP2 stream.
// frame: Points to at least the first three bytes of a frame from the
//        stream.
// return value: The sample rate of the stream in Hz, or zero if the stream
//               isn't valid.
extern int kjmp2_get_sample_rate(const unsigned char *frame);


// kjmp2_decode_frame: Decode one frame of audio.
// mp2: A pointer to a context record that has been initialized with
//      kjmp2_init before.
// frame: A pointer to the frame to decode. It *must* be a complete frame,
//        because no error checking is done!
// pcm: A pointer to the output PCM data. kjmp2_decode_frame() will always
//      return 1152 (=KJMP2_SAMPLES_PER_FRAME) interleaved stereo samples
//      in a native-endian 16-bit signed format. Even for mono streams,
//      stereo output will be produced.
// return value: The number of bytes in the current frame. In a valid stream,
//               frame + kjmp2_decode_frame(..., frame, ...) will point to
//               the next frame, if frames are consecutive in memory.
// Note: pcm may be NULL. In this case, kjmp2_decode_frame() will return the
//       size of the frame without actually decoding it.
extern unsigned long kjmp2_decode_frame(
    kjmp2_context_t *mp2,
    const unsigned char *frame,
    signed short *pcm
);

#endif//__KJMP2_H__
