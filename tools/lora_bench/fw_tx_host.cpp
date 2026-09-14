// Host harness: compiles the ACTUAL proc_lora_tx.cpp build_frame()+execute()
// synthesis math (copied verbatim) and writes cs8 IQ, to compare against the
// Python model and decode with the golden decoder. Isolates C++-port bugs.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>
#include <vector>

// ---- config (matches firmware @ SF7/BW500/CR4-5, TX_FS=2.5MHz) ----
static const uint8_t  spreading_factor = 7;
static const uint32_t bandwidth = 500000;
static const uint8_t  coding_rate = 5;
static const uint32_t TX_FS = 2500000;
static const int      FRAC_BITS = 16;

static uint32_t chips_per_symbol, samples_per_chip_, samples_per_symbol_, bw_phase_unit_;
static int64_t  dfreq_mag_fp_, dfreq_fp_, freq_fp_;
static uint32_t phase_acc_;
static uint8_t  whiten_state_;

#include "../../firmware/common/sine_table_int8.hpp"

// ---- namespace helpers (verbatim from proc_lora_tx.cpp) ----
static inline void int2bool(int v,int n,uint8_t*b){for(int i=0;i<n;i++)b[i]=(v>>(n-1-i))&1;}
static inline int  bool2int(const uint8_t*b,int n){int x=0;for(int i=0;i<n;i++)x=(x<<1)|b[i];return x;}
static inline int  gray_decode(int g){int v=g,s=g>>1;while(s){v^=s;s>>=1;}return v;}
static inline int hamming_enc(int nibble,int cr_app){
    uint8_t d[4];int2bool(nibble,4,d);
    if(cr_app!=1){int p0=d[3]^d[2]^d[1],p1=d[2]^d[1]^d[0],p2=d[3]^d[2]^d[0],p3=d[3]^d[1]^d[0];
        int full=(d[3]<<7)|(d[2]<<6)|(d[1]<<5)|(d[0]<<4)|(p0<<3)|(p1<<2)|(p2<<1)|p3;return full>>(4-cr_app);}
    int p4=d[0]^d[1]^d[2]^d[3];return (d[3]<<4)|(d[2]<<3)|(d[1]<<2)|(d[0]<<1)|p4;}
static inline void interleave_chips(const int*cw,int sf_app,int cw_len,bool ldro,int sf,int N,uint16_t*out){
    uint8_t cw_bin[12][12];
    for(int k=0;k<sf_app;k++)int2bool(cw[k],cw_len,cw_bin[k]);
    int nbits=sf_app+(ldro?1:0);
    for(int i=0;i<cw_len;i++){uint8_t inter[12]={0};
        for(int j=0;j<sf_app;j++)inter[j]=cw_bin[((i-j-1)%sf_app+sf_app)%sf_app][i];
        if(ldro){int s=0;for(int x=0;x<sf_app;x++)s+=inter[x];inter[sf_app]=s&1;}
        int g=bool2int(inter,nbits)<<(sf-nbits);
        out[i]=(uint16_t)((gray_decode(g)+1)%N);}}
static inline void hdr_checksum(const int*h,int*c4o,int*clo){
    int c4=((h[0]>>3)&1)^((h[0]>>2)&1)^((h[0]>>1)&1)^(h[0]&1);
    int c3=((h[0]>>3)&1)^((h[1]>>3)&1)^((h[1]>>2)&1)^((h[1]>>1)&1)^(h[2]&1);
    int c2=((h[0]>>2)&1)^((h[1]>>3)&1)^(h[1]&1)^((h[2]>>3)&1)^((h[2]>>1)&1);
    int c1=((h[0]>>1)&1)^((h[1]>>2)&1)^(h[1]&1)^((h[2]>>2)&1)^((h[2]>>1)&1)^(h[2]&1);
    int c0=(h[0]&1)^((h[1]>>1)&1)^((h[2]>>3)&1)^((h[2]>>2)&1)^((h[2]>>1)&1)^(h[2]&1);
    *c4o=c4;*clo=(c3<<3)|(c2<<2)|(c1<<1)|c0;}
static inline uint16_t crc16_step(uint16_t crc,uint8_t b){
    for(int i=0;i<8;i++){if(((crc&0x8000)>>8)^(b&0x80))crc=(uint16_t)((crc<<1)^0x1021);else crc=(uint16_t)(crc<<1);b=(uint8_t)(b<<1);}return crc;}
static inline uint16_t mesh_crc(const uint8_t*p,int len){uint16_t crc=0;for(int i=0;i<len-2;i++)crc=crc16_step(crc,p[i]);return (uint16_t)(crc^p[len-1]^(p[len-2]<<8));}

static uint8_t whiten_step(){
    const uint8_t fb=((whiten_state_>>7)^(whiten_state_>>5)^(whiten_state_>>4)^(whiten_state_>>3))&1u;
    const uint8_t out=whiten_state_;whiten_state_=(uint8_t)((whiten_state_<<1)|fb);return out;}

// ---- frame ----
struct TxSym{uint16_t chip;bool up;bool quarter;};
static std::array<TxSym,700> frame_;
static size_t frame_len_,sym_idx_;static uint32_t samp_in_sym_;static bool transmitting_;
static void push_sym(uint16_t chip,bool up,bool quarter=false){if(frame_len_<700)frame_[frame_len_++]={chip,up,quarter};}

static void build_frame(const uint8_t*payload,size_t payload_len){
    frame_len_=0;
    const int sf=spreading_factor,N=chips_per_symbol,cr_code=coding_rate-4,cw_len=coding_rate,len=(int)payload_len;
    for(int i=0;i<16;i++)push_sym(0,true);
    push_sym((uint16_t)((0x2u<<(sf-4))%N),true);
    push_sym((uint16_t)((0xBu<<(sf-4))%N),true);
    push_sym(0,false);push_sym(0,false);push_sym(0,false,true);
    static const int MAX_NIB=560;uint8_t nib[MAX_NIB];int nn=0;
    int h[3]={len>>4,len&0xF,(cr_code<<1)|1};int c4,clo;hdr_checksum(h,&c4,&clo);
    nib[nn++]=h[0];nib[nn++]=h[1];nib[nn++]=h[2];nib[nn++]=c4;nib[nn++]=clo;
    whiten_state_=0xFF;
    for(int i=0;i<len&&nn+2<MAX_NIB;i++){const uint8_t wb=(uint8_t)(payload[i]^whiten_step());nib[nn++]=wb&0xF;nib[nn++]=(wb>>4)&0xF;}
    const uint16_t crc=mesh_crc(payload,len);
    nib[nn++]=crc&0xF;nib[nn++]=(crc>>4)&0xF;nib[nn++]=(crc>>8)&0xF;nib[nn++]=(crc>>12)&0xF;
    int hcw[12];uint16_t chips8[8];
    for(int i=0;i<sf-2;i++)hcw[i]=hamming_enc(nib[i],4);
    interleave_chips(hcw,sf-2,8,true,sf,N,chips8);
    for(int j=0;j<8;j++)push_sym(chips8[j],true);
    int cw[12];uint16_t chipsP[12];
    for(int bi=sf-2;bi<nn;bi+=sf){
        for(int i=0;i<sf;i++)cw[i]=(bi+i<nn)?hamming_enc(nib[bi+i],cr_code):0;
        interleave_chips(cw,sf,cw_len,false,sf,N,chipsP);
        for(int j=0;j<cw_len;j++)push_sym(chipsP[j],true);}}

static void configure(){
    chips_per_symbol=1u<<spreading_factor;
    samples_per_chip_=TX_FS/bandwidth;
    samples_per_symbol_=chips_per_symbol*samples_per_chip_;
    bw_phase_unit_=(uint32_t)(((uint64_t)bandwidth<<32u)/(uint64_t)TX_FS);
    dfreq_mag_fp_=samples_per_symbol_?(((int64_t)bw_phase_unit_<<FRAC_BITS)/samples_per_symbol_):0;
    phase_acc_=0;sym_idx_=0;samp_in_sym_=0;frame_len_=0;}

// execute() inner synthesis — verbatim from proc_lora_tx.cpp
static int8_t out_i[4000000],out_q[4000000];static size_t out_n=0;
static void run(){
    transmitting_=true;sym_idx_=0;samp_in_sym_=0;phase_acc_=0;out_n=0;
    while(transmitting_){
        if(sym_idx_>=frame_len_){transmitting_=false;break;}
        const auto&sym=frame_[sym_idx_];
        const int32_t bw_i=(int32_t)bw_phase_unit_;
        const int32_t half_i=bw_i>>1;
        if(samp_in_sym_==0){
            if(sym.up){
                const int64_t step0=(((int64_t)sym.chip*bw_phase_unit_)>>spreading_factor)-half_i;
                freq_fp_=step0<<FRAC_BITS;dfreq_fp_=dfreq_mag_fp_;
            }else{freq_fp_=(int64_t)half_i<<FRAC_BITS;dfreq_fp_=-dfreq_mag_fp_;}}
        int32_t freq=(int32_t)(freq_fp_>>FRAC_BITS);
        freq=((freq+half_i)%bw_i+bw_i)%bw_i-half_i;
        phase_acc_+=(uint32_t)freq;
        freq_fp_+=dfreq_fp_;
        out_i[out_n]=sine_table_i8[(phase_acc_+0x40000000u)>>24u];
        out_q[out_n]=sine_table_i8[phase_acc_>>24u];out_n++;
        const uint32_t sym_samples=sym.quarter?(samples_per_symbol_>>2u):samples_per_symbol_;
        if(++samp_in_sym_>=sym_samples){samp_in_sym_=0;sym_idx_++;}}}

int main(int argc,char**argv){
    configure();
    // --chips <L>: dump the symbol/chip stream for a deterministic payload of
    // length L (byte i = i*7+3). Consumed by test_tx_lengths.py, which decodes
    // it with the golden decoder to prove build_frame() is correct at every
    // length (regression guard against interleaver/whitening block-boundary bugs
    // — long messages were suspected of a TX freeze; the encoder is provably OK).
    if(argc>2&&std::strcmp(argv[1],"--chips")==0){
        int L=atoi(argv[2]);
        static uint8_t payload[255];
        for(int i=0;i<L;i++)payload[i]=(uint8_t)(i*7+3);
        build_frame(payload,(size_t)L);
        printf("LEN %d NSYM %zu\nCHIPS",L,frame_len_);
        for(size_t k=21;k<frame_len_;k++)printf(" %u",frame_[k].chip);  // skip preamble+sync+SFD
        printf("\nCRC %04x\n",mesh_crc(payload,L));
        return 0;
    }
    uint8_t payload[30]={0xff,0xff,0xff,0xff,0x12,0x34,0x56,0x78};
    for(int i=0;i<22;i++)payload[8+i]=(uint8_t)(0x10+i);
    build_frame(payload,30);
    printf("frame_len=%zu\n",frame_len_);
    run();
    printf("samples=%zu\n",out_n);
    // write cs8 (single frame, repeated by the python wrapper)
    const char*out=argc>1?argv[1]:"fw_cpp.cs8";
    FILE*f=fopen(out,"wb");
    for(size_t i=0;i<out_n;i++){fputc(out_i[i],f);fputc(out_q[i],f);}
    fclose(f);
    printf("wrote %s\n",out);
    return 0;}
