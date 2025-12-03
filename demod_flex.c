/*
 *      demod_flex.c
 *
 *      Copyright 2004,2006,2010 Free Software Foundation, Inc.
 *      Copyright (C) 2015 Craig Shelley (craig@microtron.org.uk)
 *
 *      Copyright (C) 2024
 *          Jason Lingohr (jason@lucid.net.au)
 *
 *      FLEX Radio Paging Decoder - Adapted from GNURadio for use with Multimon
 *
 *      GNU Radio is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 3, or (at your option)
 *      any later version.
 *
 *      GNU Radio is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with GNU Radio; see the file COPYING.  If not, write to
 *      the Free Software Foundation, Inc., 51 Franklin Street,
 *      Boston, MA 02110-1301, USA.
 */
/*
 *  Modifications made by Ramon Smits (ramon@ramonsmits.com)
 *   - PR #223 Method `decode_phase` field `flex_groupmessage` moved into for, as it seems that it needs to be determined per page
 *   - PR #222 Suppress timestamp with argument `--flex-no-ts` which is useful when comparing output of this and future versions based on the same input file
 *   - PR #221 Estimate the total offset based on the estimated passed full cycles
 *  Modification made by bierviltje and implemented by Bruce Quinton (Zanoroy@gmail.com)
 *   - Issue #123 created by bierviltje (https://github.com/bierviltje) - Feature request: FLEX: put group messages in an array/list
 *   - This also changed the delimiter to a | rather than a space
 *  Version 0.9.2v (03 Apr 2019)
 *  Modification made by Bruce Quinton (Zanoroy@gmail.com)
 *   - Issue #120 created by PimHaarsma - Flex Tone-Only messages with short numeric body Bug fixed using code documented in the ticket system
 *  Version 0.9.1v (10 Jan 2019)
 *  Modification (to this file) made by Rob0101
 *   Fixed marking messages with K,F,C - One case had a 'C' marked as a 'K' 
 *  Version 0.9.0v (22 May 2018)
 *  Modification (to this file) made by Bruce Quinton (zanoroy@gmail.com)
 *    - Addded Define at top of file to modify the way missed group messages are reported in the debug output (default is 1; report missed capcodes on the same line)
 *                           REPORT_GROUP_CODES   1             // Report each cleared faulty group capcode : 0 = Each on a new line; 1 = All on the same line;
 *  Version 0.8.9 (20 Mar 2018)
 *  Modification (to this file) made by Bruce Quinton (zanoroy@gmail.com)
 *     - Issue #101 created by bertinhollan (https://github.com/bertinholland): Bug flex: Wrong split up group message after a data corruption frame.
 *     - Added logic to the FIW decoding that checks for any 'Group Messages' and if the frame has past them remove the group message and log output
 *     - The following settings (at the top of this file, just under these comments) have changed from:
 *                              PHASE_LOCKED_RATE    0.150
 *                              PHASE_UNLOCKED_RATE  0.150
 *       these new settings appear to work better when attempting to locate the Sync lock in the message preamble.
 *  Version 0.8.8v (20 APR 2018)
 *  Modification (to this file) made by Bruce Quinton (zanoroy@gmail.com)
 *     - Issue #101 created by bertinhollan (https://github.com/bertinholland): Bug flex: Wrong split up group message after a data corruption frame. 
 *  Version 0.8.7v (11 APR 2018)
 *  Modification (to this file) made by Bruce Quinton (zanoroy@gmail.com) and Rob0101 (as seen on github: https://github.com/rob0101)
 *     - Issue *#95 created by rob0101: '-a FLEX dropping first character of some message on regular basis'
 *     - Implemented Rob0101's suggestion of K, F and C flags to indicate the message fragmentation: 
 *         'K' message is complete and O'K' to display to the world.
 *         'F' message is a 'F'ragment and needs a 'C'ontinuation message to complete it. Message = Fragment + Continuation
 *         'C' message is a 'C'ontinuation of another fragmented message
 *  Version 0.8.6v (18 Dec 2017)
 *  Modification (to this file) made by Bruce Quinton (Zanoroy@gmail.com) on behalf of bertinhollan (https://github.com/bertinholland)
 *     - Issue #87 created by bertinhollan: Reported issue is that the flex period timeout was too short and therefore some group messages were not being processed correctly
 *                                          After some testing bertinhollan found that increasing the timeout period fixed the issue in his area. I have done further testing in my local
 *                                          area and found the change has not reduced my success rate. I think the timeout is a localisation setting and I have added "DEMOD_TIMEOUT" 
 *                                          to the definitions in the top of this file (the default value is 100 bertinhollan's prefered value, changed up from 50)
 *  Version 0.8.5v (08 Sep 2017)
 *  Modification made by Bruce Quinton (Zanoroy@gmail.com)
 *     - Issue #78 - Found a problem in the length detection sequence, modified the if statement to ensure the message length is 
 *       only checked for Aplha messages, the other types calculate thier length while decoding
 *  Version 0.8.4v (05 Sep 2017)
 *  Modification made by Bruce Quinton (Zanoroy@gmail.com)
 *     - Found a bug in the code that was not handling multiple group messages within the same frame, 
 *       and the long address bit was being miss treated in the same cases. Both issue have been fixed but further testing will help.
 *  Version 0.8.3v (22 Jun 2017)
 *  Modification made by Bruce Quinton (Zanoroy@gmail.com)
 *     - I had previously tagged Group Messages as GPN message types, 
 *       this was my own identification rather than a Flex standard type. 
 *       Now that I have cleaned up all identified (so far) issues I have changed back to the correct Flex message type of ALN (Alpha).
 *  Version 0.8.2v (21 Jun 2017)
 *  Modification made by Bruce Quinton (Zanoroy@gmail.com)
 *     - Fixed group messaging capcode issue - modified the Capcode Array to be int64_t rather than int (I was incorrectly casting the long to an int) 
 *  Version 0.8.1v (16 Jun 2017)
 *  Modification made by Bruce Quinton (Zanoroy@gmail.com)
 *     - Added Debugging to help track the group messaging issues
 *     - Improved Alpha output and removed several loops to improve CPU cycles
 *  Version 0.8v (08 Jun 2017)
 *  Modification made by Bruce Quinton (Zanoroy@gmail.com)
 *     - Added Group Messaging
 *     - Fixed Phase adjustments (phasing as part of Symbol identification)
 *     - Fixed Alpha numeric length adjustments to stop "Invalid Vector" errors
 *     - Fixed numeric message treatment
 *     - Fixed invalid identification of "unknown" messages
 *     - Added 3200 2 fsk identification to all more message types to be processed (this was a big deal for NZ)
 *     - Changed uint to int variables
 *      
 */

/* ---------------------------------------------------------------------- */

#include "multimon.h"
#include "filter.h"
#include "BCHCode.h"
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "cJSON.h"

/* ---------------------------------------------------------------------- */

#define FREQ_SAMP            22050
#define FILTLEN              1
#define REPORT_GROUP_CODES   1       // Report each cleared faulty group capcode : 0 = Each on a new line; 1 = All on the same line;

#define FLEX_SYNC_MARKER     0xA6C6AAAAul  // Synchronisation code marker for FLEX
#define SLICE_THRESHOLD      0.667         // For 4 level code, levels 0 and 3 have 3 times the amplitude of levels 1 and 2, so quantise at 2/3
#define DC_OFFSET_FILTER     0.010         // DC Offset removal IIR filter response (seconds)
#define PHASE_LOCKED_RATE    0.045         // Correction factor for locked state
#define PHASE_UNLOCKED_RATE  0.050         // Correction factor for unlocked state
#define LOCK_LEN             24            // Number of symbols to check for phase locking (max 32)
#define IDLE_THRESHOLD       0             // Number of idle codewords allowed in data section
#define CAPCODES_INDEX       0
#define DEMOD_TIMEOUT        100           // Maximum number of periods with no zero crossings before we decide that the system is not longer within a Timing lock.

int flex_disable_timestamp = 0;


enum Flex_PageTypeEnum {
  FLEX_PAGETYPE_SECURE,
  FLEX_PAGETYPE_SHORT_INSTRUCTION,
  FLEX_PAGETYPE_TONE,
  FLEX_PAGETYPE_STANDARD_NUMERIC,
  FLEX_PAGETYPE_SPECIAL_NUMERIC,
  FLEX_PAGETYPE_ALPHANUMERIC,
  FLEX_PAGETYPE_BINARY,
  FLEX_PAGETYPE_NUMBERED_NUMERIC
};


enum Flex_StateEnum {
  FLEX_STATE_SYNC1,
  FLEX_STATE_FIW,
  FLEX_STATE_SYNC2,
  FLEX_STATE_DATA
};

struct Flex_Demodulator {
  unsigned int                sample_freq;
  double                      sample_last;
  int                         locked;
  int                         phase;
  unsigned int                sample_count;
  unsigned int                symbol_count;
  double                      envelope_sum;
  int                         envelope_count;
  uint64_t                    lock_buf;
  int                         symcount[4];
  int                         timeout;
  int                         nonconsec;
  unsigned int                baud;          // Current baud rate
};

struct Flex_GroupHandler {
  int64_t                     GroupCodes[17][1000];
  int                         GroupCycle[17];
  int                         GroupFrame[17];
};

struct Flex_Modulation {
  double                      symbol_rate;
  double                      envelope;
  double                      zero;
};


struct Flex_State {
  unsigned int                sync2_count;
  unsigned int                data_count;
  unsigned int                fiwcount;
  enum Flex_StateEnum         Current;
  enum Flex_StateEnum         Previous;
};


struct Flex_Sync {
  unsigned int                sync;          // Outer synchronization code
  unsigned int                baud;          // Baudrate of SYNC2 and DATA
  unsigned int                levels;        // FSK encoding of SYNC2 and DATA
  unsigned int                polarity;      // 0=Positive (Normal) 1=Negative (Inverted)
  uint64_t                    syncbuf;
};


struct Flex_FIW {
  unsigned int                rawdata;
  unsigned int                checksum;
  unsigned int                cycleno;
  unsigned int                frameno;
  unsigned int                fix3;
};


struct Flex_Phase {
  unsigned int                buf[88];
  int                         idle_count;
};


struct Flex_Data {
  int                         phase_toggle;
  unsigned int                data_bit_counter;
  struct Flex_Phase           PhaseA;
  struct Flex_Phase           PhaseB;
  struct Flex_Phase           PhaseC;
  struct Flex_Phase           PhaseD;
};


struct Flex_Decode {
  enum Flex_PageTypeEnum      type;
  int                         long_address;
  int64_t                     capcode;
  struct BCHCode *            BCHCode;
};


struct Flex {
  struct Flex_Demodulator     Demodulator;
  struct Flex_Modulation      Modulation;
  struct Flex_State           State;
  struct Flex_Sync            Sync;
  struct Flex_FIW             FIW;
  struct Flex_Data            Data;
  struct Flex_Decode          Decode;
  struct Flex_GroupHandler    GroupHandler;
};

extern int json_mode;

static int is_alphanumeric_page(struct Flex * flex) {
  if (flex==NULL) return 0;
  return (flex->Decode.type == FLEX_PAGETYPE_ALPHANUMERIC ||
      flex->Decode.type == FLEX_PAGETYPE_SECURE);
}


static int is_numeric_page(struct Flex * flex) {
  if (flex==NULL) return 0;
  return (flex->Decode.type == FLEX_PAGETYPE_STANDARD_NUMERIC ||
      flex->Decode.type == FLEX_PAGETYPE_SPECIAL_NUMERIC  ||
      flex->Decode.type == FLEX_PAGETYPE_NUMBERED_NUMERIC);
}


static int is_tone_page(struct Flex * flex) {
  if (flex==NULL) return 0;
  return (flex->Decode.type == FLEX_PAGETYPE_TONE);
}


static unsigned int count_bits(struct Flex * flex, unsigned int data) {
  if (flex==NULL) return 0;
#ifdef USE_BUILTIN_POPCOUNT
  return __builtin_popcount(data);
#else
  unsigned int n = (data >> 1) & 0x77777777;
  data = data - n;
  n = (n >> 1) & 0x77777777;
  data = data - n;
  n = (n >> 1) & 0x77777777;
  data = data - n;
  data = (data + (data >> 4)) & 0x0f0f0f0f;
  data = data * 0x01010101;
  return data >> 24;
#endif
}

static int bch3121_fix_errors(struct Flex * flex, uint32_t * data_to_fix, char PhaseNo) {
  if (flex==NULL) return -1;
  int i=0;
  int recd[31];

  /*Convert the data pattern into an array of coefficients*/
  unsigned int data=*data_to_fix;
  for (i=0; i<31; i++) {
    recd[i] = (data>>30)&1;
    data<<=1;
  }

  /*Decode and correct the coefficients*/
  int decode_error=BCHCode_Decode(flex->Decode.BCHCode, recd);

  /*Decode successful?*/
  if (!decode_error) {
    /*Convert the coefficient array back to a bit pattern*/
    data=0;
    for (i=0; i<31; i++) {
      data<<=1;
      data|=recd[i];
    }
    /*Count the number of fixed errors*/
    int fixed=count_bits(flex, (*data_to_fix & 0x7FFFFFFF) ^ data);
    if (fixed>0) {
      verbprintf(3, "FLEX: Phase %c Fixed %i errors @ 0x%08x  (0x%08x -> 0x%08x)\n", PhaseNo, fixed, (*data_to_fix&0x7FFFFFFF) ^ data, (*data_to_fix&0x7FFFFFFF), data );
    }

    /*Write the fixed data back to the caller*/
    *data_to_fix=data;

  } else {
    verbprintf(3, "FLEX: Phase %c Data corruption - Unable to fix errors.\n", PhaseNo);
  }

  return decode_error;
}

static unsigned int flex_sync_check(struct Flex * flex, uint64_t buf) {
  if (flex==NULL) return 0;
  // 64-bit FLEX sync code:
  // AAAA:BBBBBBBB:CCCC
  //
  // Where BBBBBBBB is always 0xA6C6AAAA
  // and AAAA^CCCC is 0xFFFF
  //
  // Specific values of AAAA determine what bps and encoding the
  // packet is beyond the frame information word
  //
  // First we match on the marker field with a hamming distance < 4
  // Then we match on the outer code with a hamming distance < 4

  unsigned int marker =      (buf & 0x0000FFFFFFFF0000ULL) >> 16;
  unsigned short codehigh =  (buf & 0xFFFF000000000000ULL) >> 48;
  unsigned short codelow  = ~(buf & 0x000000000000FFFFULL);

  int retval=0;
  if (count_bits(flex, marker ^ FLEX_SYNC_MARKER) < 4  && count_bits(flex, codelow ^ codehigh) < 4 ) {
    retval=codehigh;
  } else {
    retval=0;
  }

  return retval;
}


static unsigned int flex_sync(struct Flex * flex, unsigned char sym) {
  if (flex==NULL) return 0;
  int retval=0;
  flex->Sync.syncbuf = (flex->Sync.syncbuf << 1) | ((sym < 2)?1:0);

  retval=flex_sync_check(flex, flex->Sync.syncbuf);
  if (retval!=0) {
    flex->Sync.polarity=0;
  } else {
    /*If a positive sync pattern was not found, look for a negative (inverted) one*/
    retval=flex_sync_check(flex, ~flex->Sync.syncbuf);
    if (retval!=0) {
      flex->Sync.polarity=1;
    }
  }

  return retval;
}


static void decode_mode(struct Flex * flex, unsigned int sync_code) {
  if (flex==NULL) return;

  struct {
    int sync;
    unsigned int baud;
    unsigned int levels;
  } flex_modes[] = {
    { 0x870C, 1600, 2 },
    { 0xB068, 1600, 4 },
    { 0x7B18, 3200, 2 },
    { 0xDEA0, 3200, 4 },
    { 0x4C7C, 3200, 4 },
    {0,0,0}
  };
  
  int x=0;
  int i=0;
  for (i=0; flex_modes[i].sync!=0; i++) {
    if (count_bits(flex, flex_modes[i].sync ^ sync_code) < 4) {
      flex->Sync.sync   = sync_code;
      flex->Sync.baud   = flex_modes[i].baud;
      flex->Sync.levels = flex_modes[i].levels;
      x = 1;
      break;
    }
  }
  
  if(x==0){
    verbprintf(3, "FLEX: Sync Code not found, defaulting to 1600bps 2FSK\n");
  }
}


static void read_2fsk(struct Flex * flex, unsigned int sym, unsigned int * dat) {
  if (flex==NULL) return;
  *dat = (*dat >> 1) | ((sym > 1)?0x80000000:0);
}

int estimatedPassedHourlySeconds = 0;
int lastTimeseconds = -1;

static int decode_fiw(struct Flex * flex) {
  if (flex==NULL) return -1;
  unsigned int fiw = flex->FIW.rawdata;
  int decode_error = bch3121_fix_errors(flex, &fiw, 'F');

  if (decode_error) {
    verbprintf(3, "FLEX: Unable to decode FIW, too much data corruption\n");
    return 1;
  }

  // The only relevant bits in the FIW word for the purpose of this function
  // are those masked by 0x001FFFFF.
  flex->FIW.checksum = fiw & 0xF;
  flex->FIW.cycleno = (fiw >> 4) & 0xF;
  flex->FIW.frameno = (fiw >> 8) & 0x7F;
  flex->FIW.fix3 = (fiw >> 15) & 0x3F;

  unsigned int checksum = (fiw & 0xF);
  checksum += ((fiw >> 4) & 0xF);
  checksum += ((fiw >> 8) & 0xF);
  checksum += ((fiw >> 12) & 0xF);
  checksum += ((fiw >> 16) & 0xF);
  checksum += ((fiw >> 20) & 0x01);

  checksum &= 0xF;

  if (checksum == 0xF) {
    int timeseconds = flex->FIW.cycleno*4*60 + flex->FIW.frameno*4*60/128;

    if(lastTimeseconds == -1) // Initialize
    {
      // Estimate the offset based on first detected frame, calculate hour offset of first frame
      estimatedPassedHourlySeconds -= timeseconds;
    }

    bool isHourlyRollover = lastTimeseconds > timeseconds;
    lastTimeseconds = timeseconds;
    if(isHourlyRollover) estimatedPassedHourlySeconds += 3600;

    int estimatedOffset = estimatedPassedHourlySeconds + timeseconds;

    int seconds = estimatedOffset;
    int days = seconds/86400; seconds -= 86400*days;
    int hours = seconds/3600; seconds -= 3600*hours;
    int minutes = seconds/60; seconds -= 60*minutes;

    verbprintf(2, "FLEX: FrameInfoWord: cycleno=%02i frameno=%03i fix3=0x%02x time=%02i:%02i offset=%i.%02i:%02i:%02i\n",
        flex->FIW.cycleno,
        flex->FIW.frameno,
        flex->FIW.fix3,
        timeseconds/60,
        timeseconds%60,
        days,
        hours,
        minutes,
        seconds
        );

    // Lets check the FrameNo against the expected group message frames, if we have 'Missed a group message' tell the user and clear the Cap Codes
                for(int g = 0; g < 17 ;g++)
                {
      // Do we have a group message pending for this groupbit?
      if(flex->GroupHandler.GroupFrame[g] >= 0)
      {
        int Reset = 0;
        verbprintf(4, "Flex: GroupBit %i, FrameNo: %i, Cycle No: %i target Cycle No: %i\n", g, flex->GroupHandler.GroupFrame[g], flex->GroupHandler.GroupCycle[g], (int)flex->FIW.cycleno); 
        // Now lets check if its expected in this frame..
        if((int)flex->FIW.cycleno == flex->GroupHandler.GroupCycle[g])
        {
          if(flex->GroupHandler.GroupFrame[g] < (int)flex->FIW.frameno)
          {
            Reset = 1;
          }
        }
                                // Check if we should have sent a group message in the previous cycle 
        else if(flex->FIW.cycleno == 0) 
        {
          if(flex->GroupHandler.GroupCycle[g] == 15)
          {
            Reset = 1;
          }
        }
                                // If we are waiting for the cycle to roll over then move onto the next for loop item 
        else if(flex->FIW.cycleno == 15 && flex->GroupHandler.GroupCycle[g] == 0)
        {
          continue;
        } 
        // Otherwise if the target cycle is less than the current cycle, reset the data
        else if(flex->GroupHandler.GroupCycle[g] < (int)flex->FIW.cycleno)
        {
          Reset = 1;
        }
      

        if(Reset == 1)
        {
                              
                      int endpoint = flex->GroupHandler.GroupCodes[g][CAPCODES_INDEX];
          if(REPORT_GROUP_CODES > 0)
          {
            verbprintf(3,"FLEX: Group messages seem to have been missed; Groupbit: %i; Total Capcodes: %i; Clearing Data; Capcodes: ", g, endpoint);
          }
          
                      for(int capIndex = 1; capIndex <= endpoint; capIndex++)
          {
            if(REPORT_GROUP_CODES == 0)
            {
              verbprintf(3,"FLEX: Group messages seem to have been missed; Groupbit: %i; Clearing data; Capcode: [%09lld]\n", g, flex->GroupHandler.GroupCodes[g][capIndex]);
            }
            else
            {
              if(capIndex > 1)
              {
                verbprintf(3,",");
              }
              verbprintf(3,"[%09lld]", flex->GroupHandler.GroupCodes[g][capIndex]);
            }
          }

          if(REPORT_GROUP_CODES > 0)
                                        {
                                                verbprintf(3,"\n");
                                        }

                      // reset the value
                      flex->GroupHandler.GroupCodes[g][CAPCODES_INDEX] = 0;
                      flex->GroupHandler.GroupFrame[g] = -1;
                      flex->GroupHandler.GroupCycle[g] = -1;
        }
      }
                }
    return 0;
  } else {
    verbprintf(3, "FLEX: Bad Checksum 0x%x\n", checksum);

    return 1;
  }
}

static void parse_alphanumeric(struct Flex * flex, unsigned int * phaseptr, char PhaseNo, int mw1, int mw2, int flex_groupmessage) {
        if (flex==NULL) return;
        verbprintf(3, "FLEX: Parse Alpha Numeric\n");

        int i;
        time_t now=time(NULL);
        struct tm * gmt=gmtime(&now);
        // char buf[1024], *message;
        char message[1024];
        int  currentChar = 0; 
        char frag_flag = '?';
        cJSON *json_output = cJSON_CreateObject();
        static char json_temp[100];

        int frag = (phaseptr[mw1] >> 11) & 0x03;
        int cont = (phaseptr[mw1] >> 0x0A) & 0x01;

        if (cont == 0 && frag == 3) frag_flag = 'K'; // complete, ready to send
        if (cont == 0 && frag != 3) frag_flag = 'C'; // incomplete until appended to 1 or more 'F's
        if (cont == 1             ) frag_flag = 'F'; // incomplete until a 'C' fragment is appended
    
  mw1++;
        
        for (i = mw1; i <= mw2; i++) {
            unsigned int dw =  phaseptr[i];
            unsigned char ch;

            if (i > mw1 || frag != 0x03) {
                    ch = dw & 0x7F;
                    if (ch != 0x03) {
                        message[currentChar] = ch;      
                        currentChar++;
                    }
            }

            ch = (dw >> 7) & 0x7F;
            if (ch != 0x03) {
                message[currentChar] = ch;      
                currentChar++;
            }

            ch = (dw >> 14) & 0x7F;
            if (ch != 0x03) {
                message[currentChar] = ch;      
                currentChar++;
            }
        }

        message[currentChar] = '\0';

/*
        verbprintf(0,  "FLEX: %04i-%02i-%02i %02i:%02i:%02i %i/%i/%c/%c %02i.%03i [%09lld] ALN ", 
            gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
                        flex->Sync.baud, flex->Sync.levels, frag_flag, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);

        verbprintf(0, "%s\n", message);

        if(flex_groupmessage == 1) {
                int groupbit = flex->Decode.capcode-2029568;
                if(groupbit < 0) return;

                int endpoint = flex->GroupHandler.GroupCodes[groupbit][CAPCODES_INDEX];
                for(int g = 1; g <= endpoint;g++)
                {
                        verbprintf(1, "FLEX Group message output: Groupbit: %i Total Capcodes; %i; index %i; Capcode: [%09lld]\n", groupbit, endpoint, g, flex->GroupHandler.GroupCodes[groupbit][g]);

                        verbprintf(0,  "FLEX: %04i-%02i-%02i %02i:%02i:%02i %i/%i/%c/%c %02i.%03i [%09lld] ALN ", gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
                                        flex->Sync.baud, flex->Sync.levels, frag_flag, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->GroupHandler.GroupCodes[groupbit][g]);

                        verbprintf(0, "%s\n", message);
                }
                // reset the value
                flex->GroupHandler.GroupCodes[groupbit][CAPCODES_INDEX] = 0;
    flex->GroupHandler.GroupFrame[groupbit] = -1;
    flex->GroupHandler.GroupCycle[groupbit] = -1;
        }
*/

        static char pt_out[4096] = { 0 };

        int pt_offset;

        if (!json_mode) {
          if(flex_disable_timestamp)
          {
            pt_offset = sprintf(pt_out, "FLEX|%i/%i/%c/%c|%02i.%03i|%09lld",
                          flex->Sync.baud, flex->Sync.levels, frag_flag, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
          }
          else
          {
            pt_offset = sprintf(pt_out, "FLEX|%04i-%02i-%02i %02i:%02i:%02i|%i/%i/%c/%c|%02i.%03i|%09lld",
                          gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
                          flex->Sync.baud, flex->Sync.levels, frag_flag, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
          }
        }
        else {
            // Lets just always send timestamp, the user can discard
            sprintf(json_temp,  "%04i-%02i-%02i %02i:%02i:%02i",
                    gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
            cJSON_AddStringToObject(json_output, "timestamp", json_temp);
            cJSON_AddNumberToObject(json_output, "sync_baud", flex->Sync.baud);
            cJSON_AddNumberToObject(json_output, "sync_level", flex->Sync.levels);
            char buffer[2] = "";  // Buffer to hold the string representation of PhaseNo.
            snprintf(buffer, sizeof(buffer), "%c", PhaseNo);  // Convert PhaseNo into a C-string in buffer.
            cJSON_AddStringToObject(json_output, "phase_number", buffer);
            cJSON_AddNumberToObject(json_output, "cycle_number", flex->FIW.cycleno);
            cJSON_AddNumberToObject(json_output, "frame_number", flex->FIW.frameno);
            cJSON_AddNumberToObject(json_output, "capcode", flex->Decode.capcode);
        }

        // Implemented bierviltje code from ticket: https://github.com/EliasOenal/multimon-ng/issues/123# 
        if(flex_groupmessage == 1) {
                int groupbit = flex->Decode.capcode-2029568;
                if(groupbit < 0) return;

                int endpoint = flex->GroupHandler.GroupCodes[groupbit][CAPCODES_INDEX];
                for(int g = 1; g <= endpoint;g++)
                {
                        verbprintf(1, "FLEX Group message output: Groupbit: %i Total Capcodes; %i; index %i; Capcode: [%09lld]\n", groupbit, endpoint, g, flex->GroupHandler.GroupCodes[groupbit][g]);
                        pt_offset += sprintf(pt_out + pt_offset, " %09lld", flex->GroupHandler.GroupCodes[groupbit][g]);
                }

                // reset the value
                flex->GroupHandler.GroupCodes[groupbit][CAPCODES_INDEX] = 0;
                flex->GroupHandler.GroupFrame[groupbit] = -1;
                flex->GroupHandler.GroupCycle[groupbit] = -1;
        } 
        if (!json_mode) {
          pt_offset += sprintf(pt_out + pt_offset, "|ALN|%s", message);
          verbprintf(0, "%s\n", pt_out);
        }
        else {
            cJSON_AddStringToObject(json_output, "demod_name", "flex_alphanumeric");
            cJSON_AddStringToObject(json_output, "message", message);
            fprintf(stdout, "%s\n", cJSON_PrintUnformatted(json_output));
        }
  verbprintf(1, "Delete json_output\n");
  cJSON_Delete(json_output);
}

static void parse_numeric(struct Flex * flex, unsigned int * phaseptr, char PhaseNo, int j) {
  if (flex==NULL) return;
  unsigned const char flex_bcd[17] = "0123456789 U -][";

  int w1 = phaseptr[j] >> 7;
  int w2 = w1 >> 7;
  w1 = w1 & 0x7f;
  w2 = (w2 & 0x07) + w1;  // numeric message is 7 words max

  time_t now=time(NULL);
  struct tm * gmt=gmtime(&now);
  cJSON *json_output = cJSON_CreateObject();
  static char json_temp[100];

  if (!json_mode) {
    if(flex_disable_timestamp)
    {
      verbprintf(0,  "FLEX|%i/%i/%c|%02i.%03i|[%09lld]|NUM ",
                 flex->Sync.baud, flex->Sync.levels, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
    }
    else {
      verbprintf(0,  "FLEX: %04i-%02i-%02i %02i:%02i:%02i %i/%i/%c %02i.%03i [%09lld] NUM ",
                 gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
                 flex->Sync.baud, flex->Sync.levels, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
    }
  }
  else {
      // Lets just always send timestamp, the user can discard
      sprintf(json_temp,  "%04i-%02i-%02i %02i:%02i:%02i",
              gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
      cJSON_AddStringToObject(json_output, "timestamp", json_temp);
      cJSON_AddNumberToObject(json_output, "sync_baud", flex->Sync.baud);
      cJSON_AddNumberToObject(json_output, "sync_level", flex->Sync.levels);
      char buffer[2] = "";  // Buffer to hold the string representation of PhaseNo.
      snprintf(buffer, sizeof(buffer), "%c", PhaseNo);  // Convert PhaseNo into a C-string in buffer.
      cJSON_AddStringToObject(json_output, "phase_number", buffer);
      cJSON_AddNumberToObject(json_output, "cycle_number", flex->FIW.cycleno);
      cJSON_AddNumberToObject(json_output, "frame_number", flex->FIW.frameno);
      cJSON_AddNumberToObject(json_output, "capcode", flex->Decode.capcode);
  }

  // Get first dataword from message field or from second
  // vector word if long address
  int dw;
  if(!flex->Decode.long_address) {
    dw = phaseptr[w1];
    w1++;
    w2++;
  } else {
    dw = phaseptr[j+1];
  }

  unsigned char digit = 0;
  int count = 4;
  if(flex->Decode.type == FLEX_PAGETYPE_NUMBERED_NUMERIC) {
    count += 10;        // Skip 10 header bits for numbered numeric pages
  } else {
    count += 2;        // Otherwise skip 2
  }
  int i;
  json_temp[0] = '\0';
  for(i = w1; i <= w2; i++) {
    int k;
    for(k = 0; k < 21; k++) {
      // Shift LSB from data word into digit
      digit = (digit >> 1) & 0x0F;
      if(dw & 0x01) {
        digit ^= 0x08;
      }
      dw >>= 1;
      if(--count == 0) {
        // The following if statement removes spaces between the numbers
        if(digit != 0x0C) {// Fill
          if (!json_mode) {
            verbprintf(0, "%c", flex_bcd[digit]);
          }
          else {
            strncat(json_temp, (char*)&flex_bcd[digit], 1);
          }
        }
        count = 4;
      }
    }
    dw = phaseptr[i];
  }
  if (!json_mode) {
    verbprintf(0, "\n");
  }
  else {
    cJSON_AddStringToObject(json_output, "demod_name", "flex_numeric");
    cJSON_AddStringToObject(json_output, "message", json_temp);
    fprintf(stdout, "%s\n", cJSON_PrintUnformatted(json_output));
    cJSON_Delete(json_output);
  }
}


//static void parse_tone_only(struct Flex * flex, char PhaseNo) {
//  if (flex==NULL) return;
//  time_t now=time(NULL);
//  struct tm * gmt=gmtime(&now);
//  verbprintf(0,  "FLEX: %04i-%02i-%02i %02i:%02i:%02i %i/%i/%c %02i.%03i [%09lld] TON\n", gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
//      flex->Sync.baud, flex->Sync.levels, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
//}

static void parse_tone_only(struct Flex * flex, unsigned int * phaseptr, char PhaseNo, int j) {
  if (flex==NULL) return;
  unsigned const char flex_bcd[17] = "0123456789 U -][";
  
  time_t now=time(NULL);
  struct tm * gmt=gmtime(&now);
  cJSON *json_output = cJSON_CreateObject();
  static char json_temp[100];

  if (!json_mode) {
    if(flex_disable_timestamp)
    {
      verbprintf(0,  "FLEX|%i/%i/%c|%02i.%03i|[%09lld]|TON ",
                 flex->Sync.baud, flex->Sync.levels, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
    }
    else
    {
      verbprintf(0,  "FLEX: %04i-%02i-%02i %02i:%02i:%02i %i/%i/%c %02i.%03i [%09lld] TON ",
                 gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
                 flex->Sync.baud, flex->Sync.levels, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
    }
  }
  else {
      // Lets just always send timestamp, the user can discard
      sprintf(json_temp,  "%04i-%02i-%02i %02i:%02i:%02i",
              gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
      cJSON_AddStringToObject(json_output, "timestamp", json_temp);
      cJSON_AddNumberToObject(json_output, "sync_baud", flex->Sync.baud);
      cJSON_AddNumberToObject(json_output, "sync_level", flex->Sync.levels);
      char buffer[2] = "";  // Buffer to hold the string representation of PhaseNo.
      snprintf(buffer, sizeof(buffer), "%c", PhaseNo);  // Convert PhaseNo into a C-string in buffer.
      cJSON_AddStringToObject(json_output, "phase_number", buffer);
      cJSON_AddNumberToObject(json_output, "cycle_number", flex->FIW.cycleno);
      cJSON_AddNumberToObject(json_output, "frame_number", flex->FIW.frameno);
      cJSON_AddNumberToObject(json_output, "capcode", flex->Decode.capcode);
  }

  // message type
  // 1=tone-only, 0=short numeric
  int w1 = phaseptr[j] >> 7 & 0x03; 
  if(!w1)
  {
    unsigned char digit = 0;
    int i;
    json_temp[0] = '\0';
    for (i=9; i<=17; i+=4)
    {
      digit = (phaseptr[j] >> i) & 0x0f;
      if (!json_mode) {
        verbprintf(0, "%c", flex_bcd[digit]);
      }
      else {
        strncat(json_temp, (char*)&flex_bcd[digit], 1);
      }
    }
    if(json_mode) {
      cJSON_AddStringToObject(json_output, "tone", json_temp);
      json_temp[0] = '\0';
    }

    if (flex->Decode.long_address)
    {
      for (i=0; i<=16; i+=4)
      {
        digit = (phaseptr[j+1] >> i) & 0x0f;
        if (!json_mode) {
          verbprintf(0, "%c", flex_bcd[digit]);
        }
        else {
          strncat(json_temp, (char*)&flex_bcd[digit], 1);
        }
      }
      if(json_mode) {
        cJSON_AddStringToObject(json_output, "tone_long", json_temp);
      }
    }
  }
  if (!json_mode) {
    verbprintf(0, "\n");
  }
  else {
    cJSON_AddStringToObject(json_output, "demod_name", "flex_tone_only");
    fprintf(stdout, "%s\n", cJSON_PrintUnformatted(json_output));
    cJSON_Delete(json_output);
  }
}

static void parse_unknown(struct Flex * flex, unsigned int * phaseptr, char PhaseNo, int mw1, int mw2) {
  if (flex==NULL) return;
  time_t now=time(NULL);
  struct tm * gmt=gmtime(&now);
  cJSON *json_output = cJSON_CreateObject();
  static char json_temp[100];

  if (!json_mode) {
    if(flex_disable_timestamp)
    {
      verbprintf(0,  "FLEX|%i/%i/%c|%02i.%03i|[%09lld]|UNK",
          flex->Sync.baud, flex->Sync.levels, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
    }
    else
    {
      verbprintf(0,  "FLEX: %04i-%02i-%02i %02i:%02i:%02i %i/%i/%c %02i.%03i [%09lld] UNK",
          gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
          flex->Sync.baud, flex->Sync.levels, PhaseNo, flex->FIW.cycleno, flex->FIW.frameno, flex->Decode.capcode);
    }
  }
  else {
      // Lets just always send timestamp, the user can discard
      sprintf(json_temp,  "%04i-%02i-%02i %02i:%02i:%02i",
              gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
      cJSON_AddStringToObject(json_output, "timestamp", json_temp);
      cJSON_AddNumberToObject(json_output, "sync_baud", flex->Sync.baud);
      cJSON_AddNumberToObject(json_output, "sync_level", flex->Sync.levels);
      char buffer[2] = "";  // Buffer to hold the string representation of PhaseNo.
      snprintf(buffer, sizeof(buffer), "%c", PhaseNo);  // Convert PhaseNo into a C-string in buffer.
      cJSON_AddStringToObject(json_output, "phase_number", buffer);
      cJSON_AddNumberToObject(json_output, "cycle_number", flex->FIW.cycleno);
      cJSON_AddNumberToObject(json_output, "frame_number", flex->FIW.frameno);
      cJSON_AddNumberToObject(json_output, "capcode", flex->Decode.capcode);
  }

  int i;
  json_temp[0] = '\0';
  for (i = mw1; i <= mw2; i++) {
    if (!json_mode) {
      verbprintf(0, " %08x", phaseptr[i]);
    }
    else {
      strncat(json_temp, (char*)&phaseptr[i], 1);
    }
  }
  if (!json_mode) {
    verbprintf(0, "\n");
  }
  else {
    cJSON_AddStringToObject(json_output, "demod_name", "flex_unknown");
    cJSON_AddStringToObject(json_output, "message", json_temp);
    fprintf(stdout, "%s\n", cJSON_PrintUnformatted(json_output));
    cJSON_Delete(json_output);
  }
}


//static void parse_capcode(struct Flex * flex, uint32_t aw1, uint32_t aw2) {
static void parse_capcode(struct Flex * flex, uint32_t aw1) {
  if (flex==NULL) return;

  flex->Decode.long_address = (aw1 < 0x008001L) ||
    (aw1 > 0x1E0000L) ||
    (aw1 > 0x1E7FFEL);

  ///if (flex->Decode.long_address)
  //  flex->Decode.capcode = (int64_t)aw1+((int64_t)(aw2^0x001FFFFFul)<<15)+0x1F9000ull;  // Don't ask
  //else
  flex->Decode.capcode = aw1-0x8000;
}


static void decode_phase(struct Flex * flex, char PhaseNo) {
  if (flex==NULL) return;

  uint32_t *phaseptr=NULL;
  int i, j;

  switch (PhaseNo) {
    case 'A': phaseptr=flex->Data.PhaseA.buf; break;
    case 'B': phaseptr=flex->Data.PhaseB.buf; break;
    case 'C': phaseptr=flex->Data.PhaseC.buf; break;
    case 'D': phaseptr=flex->Data.PhaseD.buf; break;
  }

  for (i=0; i<88; i++) {
    int decode_error=bch3121_fix_errors(flex, &phaseptr[i], PhaseNo);

    if (decode_error) {
      verbprintf(3, "FLEX: Garbled message at block %i\n", i);

                        // If the previous frame was a short message then we need to Null out the Group Message pointer
                        // this issue and sugested resolution was presented by 'bertinholland'


      return;
    }

    /*Extract just the message bits*/
    phaseptr[i]&=0x001FFFFF;
  }

  // Block information word is the first data word in frame
  uint32_t biw = phaseptr[0];

  // Nothing to see here, please move along
  if (biw == 0 || biw == 0x001FFFFF) {
    verbprintf(3, "FLEX: Nothing to see here, please move along\n");
    return;
  }

  // Vector start index is bits 15-10
  // Address start address is bits 9-8, plus one for offset
  int voffset = (biw >> 10) & 0x3f;
  int aoffset = ((biw >> 8) & 0x03) + 1;

  verbprintf(3, "FLEX: BlockInfoWord: (Phase %c) BIW:%08X AW:%02i-%02i (%i pages)\n", PhaseNo, biw, aoffset, voffset, voffset-aoffset);


  // Iterate through pages and dispatch to appropriate handler
  for (i = aoffset; i < voffset; i++) {
    int flex_groupmessage = 0;

    j = voffset+i-aoffset;    // Start of vector field for address @ i

    if (phaseptr[i] == 0x00000000 ||
        phaseptr[i] == 0x001FFFFF) {
      verbprintf(3, "FLEX: Idle codewords, invalid address\n");
      continue;       // Idle codewords, invalid address
    }

    parse_capcode(flex, phaseptr[i]);
    // parse_capcode(flex, phaseptr[i], phaseptr[i+1]); // Older version maybe still needed so I'm not removing it (yet)
    if (flex->Decode.long_address)
    {
      verbprintf(4, "FLEX: Found 'Long Address' bit, ignoring as I think this is handled incorrectly at the moment issue#79\n");
      // i++;
    }

          if ((flex->Decode.capcode >= 2029568) && (flex->Decode.capcode <= 2029583)) {
             flex_groupmessage = 1;
          }

    if (flex->Decode.capcode > 4297068542ll || flex->Decode.capcode < 0) {    // Invalid address (by spec, maximum address)
      verbprintf(3, "FLEX: Invalid address\n");
      continue;
    }

    verbprintf(3, "FLEX: CAPCODE:%016lx\n", flex->Decode.capcode);

    // Parse vector information word for address @ offset 'i'
    uint32_t viw = phaseptr[j];
    flex->Decode.type = ((viw >> 4) & 0x00000007);
    int mw1 = (viw >> 7) & 0x00000007F;
    int len = (viw >> 14) & 0x0000007F;

                int w1 = (int)(viw >> 7);
                int w2 = w1 >> 7;
                w1 = w1 & 0x7f;
                w2 = (w2 & 0x7f) + w1 - 1;
                // int wL = w2 - w1;

    if (flex->Decode.type == FLEX_PAGETYPE_SHORT_INSTRUCTION)
                {
                    // if (flex_groupmessage == 1) continue;
                    unsigned int iAssignedFrame = (int)((viw >> 10) & 0x7f);  // Frame with groupmessage
                    int groupbit = (int)((viw >> 17) & 0x7f);    // Listen to this groupcode
                    
        ////////#############################################################################                 
        ////////#############################################################################                 
                    flex->GroupHandler.GroupCodes[groupbit][CAPCODES_INDEX]++;
                    int CapcodePlacement = flex->GroupHandler.GroupCodes[groupbit][CAPCODES_INDEX];
                    verbprintf(1, "FLEX: Found Short Instruction, Group bit: %i capcodes in group so far %i, adding Capcode: [%09lld]\n", groupbit, CapcodePlacement, flex->Decode.capcode);

                    flex->GroupHandler.GroupCodes[groupbit][CapcodePlacement] = flex->Decode.capcode;
                    flex->GroupHandler.GroupFrame[groupbit] = iAssignedFrame;

        // Ok, so the cycle and frame can be used to make sure we haven't missed the message frame.
        // but the cycle is 0 - 15 and the frame is 0 - 127
        if(iAssignedFrame > flex->FIW.frameno)
        {
      flex->GroupHandler.GroupCycle[groupbit] = (int)flex->FIW.cycleno;
      verbprintf(4, "FLEX: Message frame is in this cycle: %i\n", flex->GroupHandler.GroupCycle[groupbit]);

        }
        else
        {
      if(flex->FIW.cycleno == 15)
                        {
        flex->GroupHandler.GroupCycle[groupbit] = 0;
      }
      else
      {
        flex->GroupHandler.GroupCycle[groupbit] = (int)flex->FIW.cycleno++;
          }
      verbprintf(4, "FLEX: Message frame is in the next cycle: %i\n", flex->GroupHandler.GroupCycle[groupbit]);
        }


                    // Nothing else to do with this word.. move on!!
                    continue;
                }

    int mw2 = mw1+(len - 1);

    if (mw1 == 0 && mw2 == 0){
      verbprintf(3, "FLEX: Invalid VIW\n");
      continue;  // Invalid VIW
    }

    if (is_tone_page(flex))
      mw1 = mw2 = 0;


                // Check if this is an alpha message
                if (is_alphanumeric_page(flex)) { 
          if (mw1 > 87 || mw2 > 87){
        verbprintf(3, "FLEX: Invalid Offsets\n");
        continue;       // Invalid offsets
      }
      parse_alphanumeric(flex, phaseptr, PhaseNo, mw1, mw2, flex_groupmessage);
                }
    else if (is_numeric_page(flex))
      parse_numeric(flex, phaseptr, PhaseNo, j);
    else if (is_tone_page(flex))
      parse_tone_only(flex, phaseptr, PhaseNo, j); // parse_tone_only(flex, PhaseNo);
    else
      parse_unknown(flex, phaseptr, PhaseNo, mw1, mw2);
  }
}


static void clear_phase_data(struct Flex * flex) {
  if (flex==NULL) return;
  int i;
  for (i=0; i<88; i++) {
    flex->Data.PhaseA.buf[i]=0;
    flex->Data.PhaseB.buf[i]=0;
    flex->Data.PhaseC.buf[i]=0;
    flex->Data.PhaseD.buf[i]=0;
  }

  flex->Data.PhaseA.idle_count=0;
  flex->Data.PhaseB.idle_count=0;
  flex->Data.PhaseC.idle_count=0;
  flex->Data.PhaseD.idle_count=0;

  flex->Data.phase_toggle=0;
  flex->Data.data_bit_counter=0;

}


static void decode_data(struct Flex * flex) {
  if (flex==NULL) return;

  if (flex->Sync.baud == 1600) {
    if (flex->Sync.levels==2) {
      decode_phase(flex, 'A');
    } else {
      decode_phase(flex, 'A');
      decode_phase(flex, 'B');
    }
  } else {
    if (flex->Sync.levels==2) {
      decode_phase(flex, 'A');
      decode_phase(flex, 'C');
    } else {
      decode_phase(flex, 'A');
      decode_phase(flex, 'B');
      decode_phase(flex, 'C');
      decode_phase(flex, 'D');
    }
  }
}


static int read_data(struct Flex * flex, unsigned char sym) {
  if (flex==NULL) return -1;
  // Here is where we output a 1 or 0 on each phase according
  // to current FLEX mode and symbol value.  Unassigned phases
  // are zero from the enter_idle() initialization.
  //
  // FLEX can transmit the data portion of the frame at either
  // 1600 bps or 3200 bps, and can use either two- or four-level
  // FSK encoding.
  //
  // At 1600 bps, 2-level, a single "phase" is transmitted with bit
  // value '0' using level '3' and bit value '1' using level '0'.
  //
  // At 1600 bps, 4-level, a second "phase" is transmitted, and the
  // di-bits are encoded with a gray code:
  //
  // Symbol Phase 1  Phase 2
  // ------   -------  -------
  //   0         1        1
  //   1         1        0
  //   2         0        0
  //   3         0        1
  //
  // At 1600 bps, 4-level, these are called PHASE A and PHASE B.
  //
  // At 3200 bps, the same 1 or 2 bit encoding occurs, except that
  // additionally two streams are interleaved on alternating symbols.
  // Thus, PHASE A (and PHASE B if 4-level) are decoded on one symbol,
  // then PHASE C (and PHASE D if 4-level) are decoded on the next.

  int bit_a=0; //Received data bit for Phase A
  int bit_b=0; //Received data bit for Phase B

  bit_a = (sym > 1);
  if (flex->Sync.levels == 4) {
    bit_b = (sym == 1) || (sym == 2);
  }

  if (flex->Sync.baud == 1600) {
    flex->Data.phase_toggle=0;
  }

  //By making the index scan the data words in this way, the data is deinterlaced
  //Bits 0, 1, and 2 map straight through to give a 0-7 sequence that repeats 32 times before moving to 8-15 repeating 32 times
  unsigned int idx= ((flex->Data.data_bit_counter>>5)&0xFFF8) |  (flex->Data.data_bit_counter&0x0007);

  if (flex->Data.phase_toggle==0) {
    flex->Data.PhaseA.buf[idx] = (flex->Data.PhaseA.buf[idx]>>1) | (bit_a?(0x80000000):0);
    flex->Data.PhaseB.buf[idx] = (flex->Data.PhaseB.buf[idx]>>1) | (bit_b?(0x80000000):0);
    flex->Data.phase_toggle=1;

    if ((flex->Data.data_bit_counter & 0xFF) == 0xFF) {
      if (flex->Data.PhaseA.buf[idx] == 0x00000000 || flex->Data.PhaseA.buf[idx] == 0xffffffff) flex->Data.PhaseA.idle_count++;
      if (flex->Data.PhaseB.buf[idx] == 0x00000000 || flex->Data.PhaseB.buf[idx] == 0xffffffff) flex->Data.PhaseB.idle_count++;
    }
  } else {
    flex->Data.PhaseC.buf[idx] = (flex->Data.PhaseC.buf[idx]>>1) | (bit_a?(0x80000000):0);
    flex->Data.PhaseD.buf[idx] = (flex->Data.PhaseD.buf[idx]>>1) | (bit_b?(0x80000000):0);
    flex->Data.phase_toggle=0;

    if ((flex->Data.data_bit_counter & 0xFF) == 0xFF) {
      if (flex->Data.PhaseC.buf[idx] == 0x00000000 || flex->Data.PhaseC.buf[idx] == 0xffffffff) flex->Data.PhaseC.idle_count++;
      if (flex->Data.PhaseD.buf[idx] == 0x00000000 || flex->Data.PhaseD.buf[idx] == 0xffffffff) flex->Data.PhaseD.idle_count++;
    }
  }

  if (flex->Sync.baud == 1600 || flex->Data.phase_toggle==0) {
    flex->Data.data_bit_counter++;
  }

  /*Report if all active phases have gone idle*/
  int idle=0;
  if (flex->Sync.baud == 1600) {
    if (flex->Sync.levels==2) {
      idle=(flex->Data.PhaseA.idle_count>IDLE_THRESHOLD);
    } else {
      idle=((flex->Data.PhaseA.idle_count>IDLE_THRESHOLD) && (flex->Data.PhaseB.idle_count>IDLE_THRESHOLD));
    }
  } else {
    if (flex->Sync.levels==2) {
      idle=((flex->Data.PhaseA.idle_count>IDLE_THRESHOLD) && (flex->Data.PhaseC.idle_count>IDLE_THRESHOLD));
    } else {
      idle=((flex->Data.PhaseA.idle_count>IDLE_THRESHOLD) && (flex->Data.PhaseB.idle_count>IDLE_THRESHOLD) && (flex->Data.PhaseC.idle_count>IDLE_THRESHOLD) && (flex->Data.PhaseD.idle_count>IDLE_THRESHOLD));
    }
  }

  return idle;
}


static void report_state(struct Flex * flex) {
  if (flex->State.Current != flex->State.Previous) {
    flex->State.Previous = flex->State.Current;

    char * state="Unknown";
    switch (flex->State.Current) {
      case FLEX_STATE_SYNC1:
        state="SYNC1";
        break;
      case FLEX_STATE_FIW:
        state="FIW";
        break;
      case FLEX_STATE_SYNC2:
        state="SYNC2";
        break;
      case FLEX_STATE_DATA:
        state="DATA";
        break;
      default:
        break;

    }
    verbprintf(1, "FLEX: State: %s\n", state);
  }
}

//Called for each received symbol
static void flex_sym(struct Flex * flex, unsigned char sym) {
  if (flex==NULL) return;
  /*If the signal has a negative polarity, the symbols must be inverted*/
  /*Polarity is determined during the IDLE/sync word checking phase*/
  unsigned char sym_rectified;
  if (flex->Sync.polarity) {
    sym_rectified=3-sym;
  } else {
    sym_rectified=sym;
  }

  switch (flex->State.Current) {
    case FLEX_STATE_SYNC1:
      {
        // Continually compare the received symbol stream
        // against the known FLEX sync words.
        unsigned int sync_code=flex_sync(flex, sym); //Unrectified version of the symbol must be used here
        if (sync_code!=0) {
          decode_mode(flex,sync_code);

          if (flex->Sync.baud!=0 && flex->Sync.levels!=0) {
            flex->State.Current=FLEX_STATE_FIW;

            verbprintf(2, "FLEX: SyncInfoWord: sync_code=0x%04x baud=%i levels=%i polarity=%s zero=%f envelope=%f symrate=%f\n",
                sync_code, flex->Sync.baud, flex->Sync.levels, flex->Sync.polarity?"NEG":"POS", flex->Modulation.zero, flex->Modulation.envelope, flex->Modulation.symbol_rate);
          } else {
            verbprintf(2, "FLEX: Unknown Sync code = 0x%04x\n", sync_code);
            flex->State.Current=FLEX_STATE_SYNC1;
          }
        } else {
          flex->State.Current=FLEX_STATE_SYNC1;
        }

        flex->State.fiwcount=0;
        flex->FIW.rawdata=0;
        break;
      }
    case FLEX_STATE_FIW:
      {
        // Skip 16 bits of dotting, then accumulate 32 bits
        // of Frame Information Word.
        // FIW is accumulated, call BCH to error correct it
        flex->State.fiwcount++;
        if (flex->State.fiwcount>=16) {
          read_2fsk(flex, sym_rectified, &flex->FIW.rawdata);
        }

        if (flex->State.fiwcount==48) {
          if (decode_fiw(flex)==0) {
            flex->State.sync2_count=0;
            flex->Demodulator.baud = flex->Sync.baud;
            flex->State.Current=FLEX_STATE_SYNC2;
          } else {
            flex->State.Current=FLEX_STATE_SYNC1;
          }
        }
        break;
      }
    case FLEX_STATE_SYNC2:
      {
        // This part and the remainder of the frame are transmitted
        // at either 1600 bps or 3200 bps based on the received
        // FLEX sync word. The second SYNC header is 25ms of idle bits
        // at either speed.
        // Skip 25 ms = 40 bits @ 1600 bps, 80 @ 3200 bps
        if (++flex->State.sync2_count == flex->Sync.baud*25/1000) {
          flex->State.data_count=0;
          clear_phase_data(flex);
          flex->State.Current=FLEX_STATE_DATA;
        }

        break;
      }
    case FLEX_STATE_DATA:
      {
        // The data portion of the frame is 1760 ms long at either
        // baudrate.  This is 2816 bits @ 1600 bps and 5632 bits @ 3200 bps.
        // The output_symbol() routine decodes and doles out the bits
        // to each of the four transmitted phases of FLEX interleaved codes.
        int idle=read_data(flex, sym_rectified);
        if (++flex->State.data_count == flex->Sync.baud*1760/1000 || idle) {
          decode_data(flex);
          flex->Demodulator.baud = 1600;
          flex->State.Current=FLEX_STATE_SYNC1;
          flex->State.data_count=0;
        }
        break;
      }
  }
}

static int buildSymbol(struct Flex * flex, double sample) {
        if (flex == NULL) return 0;

        const int64_t phase_max = 100 * flex->Demodulator.sample_freq;                           // Maximum value for phase (calculated to divide by sample frequency without remainder)
        const int64_t phase_rate = phase_max*flex->Demodulator.baud / flex->Demodulator.sample_freq;      // Increment per baseband sample
        const double phasepercent = 100.0 *  flex->Demodulator.phase / phase_max;

        /*Update the sample counter*/
        flex->Demodulator.sample_count++;

        /*Remove DC offset (FIR filter)*/
        if (flex->State.Current == FLEX_STATE_SYNC1) {
                flex->Modulation.zero = (flex->Modulation.zero*(FREQ_SAMP*DC_OFFSET_FILTER) + sample) / ((FREQ_SAMP*DC_OFFSET_FILTER) + 1);
        }
        sample -= flex->Modulation.zero;

        if (flex->Demodulator.locked) {
                /*During the synchronisation period, establish the envelope of the signal*/
                if (flex->State.Current == FLEX_STATE_SYNC1) {
                        flex->Demodulator.envelope_sum += fabs(sample);
                        flex->Demodulator.envelope_count++;
                        flex->Modulation.envelope = flex->Demodulator.envelope_sum / flex->Demodulator.envelope_count;
                }
        }
        else {
                /*Reset and hold in initial state*/
                flex->Modulation.envelope = 0;
                flex->Demodulator.envelope_sum = 0;
                flex->Demodulator.envelope_count = 0;
                flex->Demodulator.baud = 1600;
                flex->Demodulator.timeout = 0;
                flex->Demodulator.nonconsec = 0;
                flex->State.Current = FLEX_STATE_SYNC1;
        }

        /* MID 80% SYMBOL PERIOD */
        if (phasepercent > 10 && phasepercent <90) {
                /*Count the number of occurrences of each symbol value for analysis at end of symbol period*/
                if (sample > 0) {
                        if (sample > flex->Modulation.envelope*SLICE_THRESHOLD)
                                flex->Demodulator.symcount[3]++;
                        else
                                flex->Demodulator.symcount[2]++;
                }
                else {
                        if (sample < -flex->Modulation.envelope*SLICE_THRESHOLD)
                                flex->Demodulator.symcount[0]++;
                        else
                                flex->Demodulator.symcount[1]++;
                }
        }

        /* ZERO CROSSING */
        if ((flex->Demodulator.sample_last<0 && sample >= 0) || (flex->Demodulator.sample_last >= 0 && sample<0)) {
                /*The phase error has a direction towards the closest symbol boundary*/
                double phase_error = 0.0;
                if (phasepercent<50) {
                        phase_error = flex->Demodulator.phase;
                }
                else {
                        phase_error = flex->Demodulator.phase - phase_max;
                }

                /*Phase lock with the signal*/
                if (flex->Demodulator.locked) {
                        flex->Demodulator.phase -= phase_error * PHASE_LOCKED_RATE;
                }
                else {
                        flex->Demodulator.phase -= phase_error * PHASE_UNLOCKED_RATE;
                }

                /*If too many zero crossing occur within the mid 80% then indicate lock has been lost*/
                if (phasepercent > 10 && phasepercent < 90) {
                        flex->Demodulator.nonconsec++;
                        if (flex->Demodulator.nonconsec>20 && flex->Demodulator.locked) {
                                verbprintf(1, "FLEX: Synchronisation Lost\n");
                                flex->Demodulator.locked = 0;
                        }
                }
                else {
                        flex->Demodulator.nonconsec = 0;
                }

                flex->Demodulator.timeout = 0;
        }
        flex->Demodulator.sample_last = sample;

  /* END OF SYMBOL PERIOD */
  flex->Demodulator.phase += phase_rate;

  if (flex->Demodulator.phase > phase_max) {
    flex->Demodulator.phase -= phase_max;
    return 1;
  } else {
    return 0;
  }

}

static void Flex_Demodulate(struct Flex * flex, double sample) {
  if(flex == NULL) return;

  if (buildSymbol(flex, sample) == 1) {
                flex->Demodulator.nonconsec = 0;
    flex->Demodulator.symbol_count++;
    flex->Modulation.symbol_rate = 1.0 * flex->Demodulator.symbol_count*flex->Demodulator.sample_freq / flex->Demodulator.sample_count;

    /*Determine the modal symbol*/
    int j;
    int decmax = 0;
    int modal_symbol = 0;
    for (j = 0; j<4; j++) {
      if (flex->Demodulator.symcount[j] > decmax) {
        modal_symbol = j;
        decmax = flex->Demodulator.symcount[j];
      }
    }
    flex->Demodulator.symcount[0] = 0;
    flex->Demodulator.symcount[1] = 0;
    flex->Demodulator.symcount[2] = 0;
    flex->Demodulator.symcount[3] = 0;


    if (flex->Demodulator.locked) {
      /*Process the symbol*/
      flex_sym(flex, modal_symbol);
    }
    else {
      /*Check for lock pattern*/
      /*Shift symbols into buffer, symbols are converted so that the max and min symbols map to 1 and 2 i.e each contain a single 1 */
      flex->Demodulator.lock_buf = (flex->Demodulator.lock_buf << 2) | (modal_symbol ^ 0x1);
      uint64_t lock_pattern = flex->Demodulator.lock_buf ^ 0x6666666666666666ull;
      uint64_t lock_mask = (1ull << (2 * LOCK_LEN)) - 1;
      if ((lock_pattern&lock_mask) == 0 || ((~lock_pattern)&lock_mask) == 0) {
        verbprintf(1, "FLEX: Locked\n");
        flex->Demodulator.locked = 1;
        /*Clear the syncronisation buffer*/
        flex->Demodulator.lock_buf = 0;
        flex->Demodulator.symbol_count = 0;
        flex->Demodulator.sample_count = 0;
      }
    }

    /*Time out after X periods with no zero crossing*/
    flex->Demodulator.timeout++;
    if (flex->Demodulator.timeout>DEMOD_TIMEOUT) {
      verbprintf(1, "FLEX: Timeout\n");
      flex->Demodulator.locked = 0;
    }
  }

  report_state(flex);
}

static void Flex_Delete(struct Flex * flex) {
  if (flex==NULL) return;

  if (flex->Decode.BCHCode!=NULL) {
    BCHCode_Delete(flex->Decode.BCHCode);
    flex->Decode.BCHCode=NULL;
  }

  free(flex);
}


static struct Flex * Flex_New(unsigned int SampleFrequency) {
  struct Flex *flex=(struct Flex *)malloc(sizeof(struct Flex));
  if (flex!=NULL) {
    memset(flex, 0, sizeof(struct Flex));

    flex->Demodulator.sample_freq=SampleFrequency;
    // The baud rate of first syncword and FIW is always 1600, so set that
    // rate to start.
    flex->Demodulator.baud = 1600;

    /*Generator polynomial for BCH3121 Code*/
    int p[6];
    p[0] = p[2] = p[5] = 1; p[1] = p[3] = p[4] =0;
    flex->Decode.BCHCode=BCHCode_New( p, 5, 31, 21, 2);
    if (flex->Decode.BCHCode == NULL) {
      Flex_Delete(flex);
      flex=NULL;
    }

    for(int g = 0; g < 17; g++)
    {
      flex->GroupHandler.GroupFrame[g] = -1;
          flex->GroupHandler.GroupCycle[g] = -1;
    }
  }

  return flex;
}


static void flex_demod(struct demod_state *s, buffer_t buffer, int length) {
  if (s==NULL) return;
  if (s->l1.flex==NULL) return;
  int i;
  for (i=0; i<length; i++) {
    Flex_Demodulate(s->l1.flex, buffer.fbuffer[i]);
  }
}


static void flex_init(struct demod_state *s) {
  if (s==NULL) return;
  s->l1.flex=Flex_New(FREQ_SAMP);
}


static void flex_deinit(struct demod_state *s) {
  if (s==NULL) return;
  if (s->l1.flex==NULL) return;

  Flex_Delete(s->l1.flex);
  s->l1.flex=NULL;
}


const struct demod_param demod_flex = {
  "FLEX", true, FREQ_SAMP, FILTLEN, flex_init, flex_demod, flex_deinit
};