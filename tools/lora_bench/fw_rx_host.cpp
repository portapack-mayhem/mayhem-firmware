// Host harness: compiles the ACTUAL proc_lora.cpp RX decode DSP (verbatim) and
// runs it against a known BW500 frame decimated to 625 kHz (= FS4Decim4 output),
// printing state transitions + decoded bytes. Isolates RX decode-chain bugs.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <complex>
#include <algorithm>
#include <array>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---- config (SF7/BW500/CR4-5, decimated dec_fs = 625 kHz) ----
static const uint8_t  spreading_factor = 7;
static const uint8_t  coding_rate = 5;
static uint32_t chips_per_symbol = 128;
static uint32_t samples_per_symbol = 256;  // OS=2 @1MHz
static const bool use_ldro_ = false;
static constexpr uint8_t  PREAMBLE_DETECT = 4;  // post-snap aligned-preamble confirmations before accepting sync
static constexpr uint8_t  NEW_PRE_DETECT = 8;
static constexpr uint16_t PAYLOAD_SYM_LIMIT = 220;
static constexpr size_t   MAX_FFT = 2048;
static constexpr size_t   MAX_PAYLOAD = 255;

struct complex16_t { int16_t i_, q_; int16_t real() const { return i_; } int16_t imag() const { return q_; } };

// ---- state (mirrors LoRaProcessor members) ----
enum class RxState : uint8_t { HUNT, PRE_END, SKIP, HEADER, PAYLOAD };
static RxState rx_state_ = RxState::HUNT;
static uint16_t hdr_sym_[8]={}; static uint8_t hdr_count_=0, payload_len_target_=0;
static uint8_t decode_header(){  // returns length, or 0 if the header checksum fails
    const int sf_app=spreading_factor-2,cw_len=8; uint8_t cw[12]={};
    for(int i=0;i<cw_len;i++)for(int j=0;j<sf_app;j++){const int c=((i-j-1)%sf_app+sf_app)%sf_app;const int bit=(hdr_sym_[i]>>(sf_app-1-j))&1u;cw[c]=(uint8_t)(cw[c]|(bit<<(cw_len-1-i)));}
    uint8_t nib[8]={};
    for(int i=0;i<sf_app;i++){
        // Hamming(8,4) SINGLE-ERROR CORRECT — the header block carries FEC; use it.
        int d0=(cw[i]>>4)&1,d1=(cw[i]>>5)&1,d2=(cw[i]>>6)&1,d3=(cw[i]>>7)&1;   // data d[0..3]
        int rp0=(cw[i]>>3)&1,rp1=(cw[i]>>2)&1,rp2=(cw[i]>>1)&1,rp3=cw[i]&1;    // received parities
        int s0=rp0^(d3^d2^d1),s1=rp1^(d2^d1^d0),s2=rp2^(d3^d2^d0),s3=rp3^(d3^d1^d0);
        int syn=(s0<<3)|(s1<<2)|(s2<<1)|s3;
        if(syn==11)d3^=1; else if(syn==14)d2^=1; else if(syn==13)d1^=1; else if(syn==7)d0^=1;
        nib[i]=(uint8_t)((d0<<3)|(d1<<2)|(d2<<1)|d3);
    }
    // Semtech header checksum (gr-lora header_impl) over nib[0..2]; nib[3]=c4, nib[4]=c3..c0.
    const uint8_t h0=nib[0],h1=nib[1],h2=nib[2];
    const uint8_t c4=((h0>>3)&1)^((h0>>2)&1)^((h0>>1)&1)^(h0&1);
    const uint8_t c3=((h0>>3)&1)^((h1>>3)&1)^((h1>>2)&1)^((h1>>1)&1)^(h2&1);
    const uint8_t c2=((h0>>2)&1)^((h1>>3)&1)^(h1&1)^((h2>>3)&1)^((h2>>1)&1);
    const uint8_t c1=((h0>>1)&1)^((h1>>2)&1)^(h1&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    const uint8_t c0=(h0&1)^((h1>>1)&1)^((h2>>3)&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    const uint8_t clo=(c3<<3)|(c2<<2)|(c1<<1)|c0;
    fprintf(stderr,"  HDR nib=[%u %u %u %u %u] rawlen=%u c4got=%u c4exp=%u clogot=%u cloexp=%u syms=[%u %u %u %u %u %u %u %u]\n",nib[0],nib[1],nib[2],nib[3],nib[4],(nib[0]<<4)|nib[1],nib[3]&1,c4,nib[4]&0xF,clo,hdr_sym_[0],hdr_sym_[1],hdr_sym_[2],hdr_sym_[3],hdr_sym_[4],hdr_sym_[5],hdr_sym_[6],hdr_sym_[7]);
    if((nib[3]&1)!=c4 || (nib[4]&0xF)!=clo) return 0;   // checksum mismatch → reject
    return (uint8_t)((nib[0]<<4)|nib[1]);
}
static uint8_t preamble_run_=0, skip_remain_=0, new_pre_run_=0;
static std::array<uint16_t,8> sym_block_{};
static uint8_t sym_in_block_=0; static uint16_t payload_sym_count_=0;
static bool nibble_lo_valid_=false; static uint8_t nibble_lo_=0;
static uint8_t whiten_state_=0xFF;
static std::array<uint8_t,MAX_PAYLOAD> payload_buf{}; static uint8_t decoded_len_=0;
static float last_peak_mag_=0, ref_peak_mag_=0; static uint8_t weak_sym_count_=0;
static size_t last_peak_bin_=0;
static float last_peak_frac_=0;
static uint32_t phase_offset_=0, timing_corr_=0;
static uint32_t raw_sample_count_=0;
static uint32_t realign_samples_=0;   // one-shot 0.25-symbol SFD grid shift
static bool snapped_=false;           // fine timing snap done
static std::array<std::complex<float>,MAX_FFT> fft_buf{};
static std::array<complex16_t,MAX_FFT> fft_accum{};
static int g_window=0;  // debug
static const complex16_t* g_buf=nullptr; static size_t g_count=0, g_pos=0;  // for fine-align look-ahead
static void lora_fft_inplace(std::complex<float>* data, size_t n);  // fwd decl

// dechirp+fold a buffer segment, return peak/total sharpness (no state change)
static uint32_t sharp_peak_bin_=0; static float sharp_peak_frac_=0;
static std::complex<float> sharp_peak_cplx_=0;   // folded peak (complex) for phase-based CFO
static float sharp_full_frac_=0;   // sub-bin CFO from the full (un-folded) FFT peak
static float cfo_frac_=0;   // fractional-bin CFO, mixed into the data dechirp
static float fold_sharpness(const complex16_t* p,size_t sps){
    const size_t N=chips_per_symbol,os=sps/N; const float inv_os=1.0f/(float)os;
    for(size_t n=0;n<sps;++n){
        const float ph=(float)M_PI*inv_os*((float)(n*n)/(float)sps-(float)n);
        const float cq=cosf(ph),sq=sinf(ph),sr=(float)p[n].real(),si=(float)p[n].imag();
        fft_buf[n]={sr*cq+si*sq,si*cq-sr*sq};
    }
    lora_fft_inplace(fft_buf.data(),sps);
    static float sy[2048]; static std::complex<float> sa[2048]; float pm=0,tot=0; size_t pb=0;
    for(size_t k=0;k<N;++k){std::complex<float> acc(0,0);for(size_t m=0;m<os;++m)acc+=fft_buf[k+m*N];
        sa[k]=acc; sy[k]=sqrtf(acc.real()*acc.real()+acc.imag()*acc.imag());tot+=sy[k]*sy[k];if(sy[k]>pm){pm=sy[k];pb=k;}}
    const float a=sy[(pb+N-1)%N],b=sy[pb],c=sy[(pb+1)%N],den=a-2*b+c;
    sharp_peak_frac_=(den!=0)?0.5f*(a-c)/den:0;
    sharp_peak_bin_=(uint32_t)pb;
    sharp_peak_cplx_=sa[pb];
    // Sub-bin CFO from the FULL sps-FFT (each alias is a clean sinc; the FOLD sums
    // aliases with differing phase → its magnitude is NOT parabolic → bad interp).
    float fm=0; size_t fb=0;
    for(size_t k=0;k<sps;++k){float m2=fft_buf[k].real()*fft_buf[k].real()+fft_buf[k].imag()*fft_buf[k].imag();if(m2>fm){fm=m2;fb=k;}}
    const float fa=std::abs(fft_buf[(fb+sps-1)%sps]),fbb=std::abs(fft_buf[fb]),fc=std::abs(fft_buf[(fb+1)%sps]),fden=fa-2*fbb+fc;
    float ffrac=(fden!=0)?0.5f*(fa-fc)/fden:0;
    float pos128=std::fmod((float)fb+ffrac,(float)N);          // alias-folded to 0..N
    sharp_full_frac_=pos128-std::round(pos128);                // sub-bin residual (frac CFO)
    return tot>0?pm*pm/tot:0;
}

// ---- FFT (verbatim) ----
static void lora_fft_inplace(std::complex<float>* data, size_t n){
    for(size_t i=1,j=0;i<n;++i){size_t bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(data[i],data[j]);}
    for(size_t len=2;len<=n;len<<=1){const float ang=-2.0f*(float)M_PI/(float)len;const std::complex<float> wlen(cosf(ang),sinf(ang));
        for(size_t i=0;i<n;i+=len){std::complex<float> w(1,0);for(size_t j=0;j<len/2;++j){auto u=data[i+j];auto v=data[i+j+len/2]*w;data[i+j]=u+v;data[i+j+len/2]=u-v;w*=wlen;}}}}

static uint8_t lora_whiten_step(){const uint8_t fb=((whiten_state_>>7)^(whiten_state_>>5)^(whiten_state_>>4)^(whiten_state_>>3))&1u;const uint8_t out=whiten_state_;whiten_state_=(uint8_t)((whiten_state_<<1)|fb);return out;}

static void reset_rx(){rx_state_=RxState::HUNT;preamble_run_=0;new_pre_run_=0;sym_in_block_=0;payload_sym_count_=0;nibble_lo_valid_=false;whiten_state_=0xFF;decoded_len_=0;last_peak_mag_=0;ref_peak_mag_=0;weak_sym_count_=0;phase_offset_=0;timing_corr_=0;snapped_=false;realign_samples_=0;}

static void send_packet(const uint8_t* d,size_t len){printf(">>> send_packet len=%zu | ",len);for(size_t i=0;i<len;i++)printf("%02x ",d[i]);printf("\n");}

static void process_sym_block(){
    const int cpb=use_ldro_?(spreading_factor-2):spreading_factor; const int cr=coding_rate;
    uint8_t cw[12]={};
    for(int i=0;i<cr;i++)for(int j=0;j<cpb;j++){const int c=((i-j-1)%cpb+cpb)%cpb;const int bit=(sym_block_[i]>>(cpb-1-j))&1u;cw[c]=(uint8_t)(cw[c]|(bit<<(cr-1-i)));}
    for(int i=0;i<cpb;i++){
        const uint8_t b0=(cw[i]>>(cr-1))&1u,b1=(cw[i]>>(cr-2))&1u,b2=(cw[i]>>(cr-3))&1u,b3=(cw[i]>>(cr-4))&1u;
        const uint8_t nib=(uint8_t)((b3<<3)|(b2<<2)|(b1<<1)|b0);
        if(!nibble_lo_valid_){nibble_lo_=nib;nibble_lo_valid_=true;}
        else{const uint8_t bv=(uint8_t)(((nib<<4)|nibble_lo_)^lora_whiten_step());if(decoded_len_<MAX_PAYLOAD)payload_buf[decoded_len_++]=bv;nibble_lo_valid_=false;}
    }
}

// FOLD demod (matches validated lora_rx.demod): full-rate dechirp over sps
// samples + FFT(sps) + sum os aliases -> N bins.  Uses the oversampling
// (robust to sub-sample timing) instead of discarding it via resample-to-N.
static int32_t dechirp_symbol(size_t sps,bool is_up){
    const size_t N=chips_per_symbol; const size_t os=sps/N;
    const float inv_os=1.0f/(float)os;
    const float cfo_ramp=2.0f*(float)M_PI*cfo_frac_/(float)N;   // sub-bin CFO correction
    for(size_t n=0;n<sps;++n){
        const float ph=(float)M_PI*inv_os*((float)(n*n)/(float)sps-(float)n)+cfo_ramp*(float)n;
        const float cq=cosf(ph),sq=sinf(ph);   // ref = exp(-j ph)
        const float sr=(float)fft_accum[n].real(),si=(float)fft_accum[n].imag();
        fft_buf[n]={sr*cq+si*sq, si*cq-sr*sq};  // x * exp(-j ph)
    }
    lora_fft_inplace(fft_buf.data(),sps);
    static float ym[2048];
    float pm=0;size_t pb=0;
    for(size_t k=0;k<N;++k){
        std::complex<float> acc(0,0);
        for(size_t m=0;m<os;++m)acc+=fft_buf[k+m*N];
        ym[k]=sqrtf(acc.real()*acc.real()+acc.imag()*acc.imag());
        if(ym[k]>pm){pm=ym[k];pb=k;}
    }
    // parabolic interpolation for sub-bin (sub-chip) peak position
    const float a=ym[(pb+N-1)%N],b=ym[pb],c=ym[(pb+1)%N];
    const float den=a-2.0f*b+c;
    last_peak_frac_=(den!=0.0f)?0.5f*(a-c)/den:0.0f;   // in (-0.5,0.5)
    last_peak_mag_=pm*pm;last_peak_bin_=pb;
    return (int32_t)pb;
}

// dechirp a DOWN-chirp (multiply fft_accum by exp(+j ph)) → peak bin (for the SFD)
static uint32_t down_peak_bin_=0; static float down_peak_mag_=0, down_peak_frac_=0;
static int32_t dechirp_down(size_t sps){
    const size_t N=chips_per_symbol,os=sps/N; const float inv_os=1.0f/(float)os;
    for(size_t n=0;n<sps;++n){
        const float ph=(float)M_PI*inv_os*((float)(n*n)/(float)sps-(float)n);
        const float cq=cosf(ph),sq=sinf(ph);
        const float sr=(float)fft_accum[n].real(),si=(float)fft_accum[n].imag();
        fft_buf[n]={sr*cq-si*sq, sr*sq+si*cq};  // x*exp(+j ph)
    }
    lora_fft_inplace(fft_buf.data(),sps);
    static float ym[2048]; float pm=0;size_t pb=0;
    for(size_t k=0;k<N;++k){std::complex<float> acc(0,0);for(size_t m=0;m<os;++m)acc+=fft_buf[k+m*N];
        ym[k]=sqrtf(acc.real()*acc.real()+acc.imag()*acc.imag()); if(ym[k]>pm){pm=ym[k];pb=k;}}
    const float a=ym[(pb+N-1)%N],b=ym[pb],c=ym[(pb+1)%N],den=a-2*b+c;
    down_peak_frac_=(den!=0)?0.5f*(a-c)/den:0;
    down_peak_mag_=pm*pm; down_peak_bin_=(uint32_t)pb; return (int32_t)pb;
}
static uint32_t up_bin_sfd_=0, down_bin_sfd_=0; static bool have_down_=false; static float best_down_mag_=0;
static float up_frac_sfd_=0, down_frac_sfd_=0;
static double up_acc_=0; static int up_cnt_=0;   // average preamble up-position (reduce noise)
static long g_preamble_=0;                        // first preamble window start (for sharpness fine-align)
static int  stored_realign_=0;                    // fine-align result computed at sync-detection (moved out of SKIP)

static const char* SN(RxState s){return s==RxState::HUNT?"HUNT":s==RxState::PRE_END?"PRE_END":s==RxState::SKIP?"SKIP":"PAYLOAD";}

static void process_symbol_window(size_t sps){
    const uint32_t thr=std::max(uint32_t{2},chips_per_symbol>>3u);
    const int32_t raw_up=dechirp_symbol(sps,true);
    if(raw_up<0)return;
    const bool nz=((uint32_t)raw_up<thr);
    if(g_window<60||rx_state_!=RxState::HUNT)
        printf("win%3d %-7s raw_up=%3d nz=%d peakbin=%zu mag=%.0f pre=%u skip=%u\n",g_window,SN(rx_state_),raw_up,nz,last_peak_bin_,sqrtf(last_peak_mag_),preamble_run_,skip_remain_);
    switch(rx_state_){
    case RxState::HUNT:
        if(nz){
            // MATCH FIRMWARE: use the MOST-RECENT preamble window (no averaging).
            up_bin_sfd_=(uint32_t)last_peak_bin_; up_frac_sfd_=last_peak_frac_;
            up_acc_=0; up_cnt_=0; g_preamble_=(long)(g_pos+1)-(long)samples_per_symbol;
            if(!snapped_){snapped_=true;preamble_run_=0;}
            else preamble_run_++;
        } else {
            if(snapped_&&preamble_run_>=PREAMBLE_DETECT){
                // FINE-ALIGN MOVED HERE (was the FFT burst at end-of-SKIP): the first
                // non-near-zero window = sync word 1.  g_preamble_ still points at the
                // LAST preamble window (set on the previous call), so compute dbest NOW —
                // 3 windows (~0.75 ms @BW500) before the header — leaving the M4 the SKIP
                // windows to drain any sample backlog before the delicate no-FEC header.
                // Center the ±2os sharpness sweep on the preamble BIN-SNAP (no SFD needed):
                // an aligned preamble peaks at bin 1+cfo, a window off by τ samples peaks at
                // 1+cfo+τ/os, so bin-snap c=-(up_bin-1)·os ≈ -τ (exact when cfo≈0).
                const int os=(int)(samples_per_symbol/chips_per_symbol);
                const int sps=(int)samples_per_symbol;
                const int c=-((int)up_bin_sfd_-1)*os;
                int dbest=c; float sbest=-1.0f;
                for(int dd=c-2*os; dd<=c+2*os; ++dd){
                    long pos=g_preamble_+dd; if(pos<0||pos+sps>(long)g_count)continue;
                    float s=fold_sharpness(&g_buf[pos],sps); if(s>sbest){sbest=s;dbest=dd;}
                }
                // re-measure CFO on the sample-aligned preamble → timing_corr_ (bin 1+cfo).
                if(g_preamble_+dbest>=0 && g_preamble_+dbest+sps<=(long)g_count){
                    fold_sharpness(&g_buf[g_preamble_+dbest],sps); timing_corr_=sharp_peak_bin_;
                }
                int realign=sps/4 + dbest; realign=((realign%sps)+sps)%sps;
                stored_realign_=realign;
                printf("SYNC-ALIGN pre=%ld up_bin=%u c=%d dbest=%d timing_corr=%u realign=%d\n",
                       g_preamble_,up_bin_sfd_,c,dbest,timing_corr_,realign);
                skip_remain_=2; rx_state_=RxState::SKIP;
            } else { preamble_run_=0; snapped_=false; }  // false alarm, re-scan
        }
        break;
    case RxState::SKIP: {
        // Pure counter now — dbest/timing_corr/realign were computed at sync-detection.
        // No dechirp_down, no sweep: SKIP is nearly free, so the M4 drains its sample
        // backlog across sync2 + the 2.25-symbol SFD before the header starts.
        if(skip_remain_>0){skip_remain_--; break;}
        realign_samples_=(uint32_t)stored_realign_; hdr_count_=0; rx_state_=RxState::HEADER;
        break;
    }
    case RxState::HEADER:{
        const uint32_t chip_corr=((uint32_t)raw_up+chips_per_symbol-timing_corr_)%chips_per_symbol;
        const uint32_t a=((chip_corr+chips_per_symbol-1u)%chips_per_symbol)>>2u;  // reduced-rate
        if(hdr_count_<8)hdr_sym_[hdr_count_++]=(uint16_t)(a^(a>>1u));
        if(hdr_count_>=8){uint8_t L=decode_header();payload_len_target_=(L>=4&&L<=255)?L:0;
            printf("HEADER decoded length=%u timing_corr=%u\n",L,timing_corr_);
            sym_in_block_=0;payload_sym_count_=0;nibble_lo_valid_=false;whiten_state_=0xFF;decoded_len_=0;new_pre_run_=0;rx_state_=RxState::PAYLOAD;}
        break;}
    case RxState::PAYLOAD:{
        if(payload_sym_count_==0){ref_peak_mag_=last_peak_mag_;weak_sym_count_=0;}
        else{if(last_peak_mag_>ref_peak_mag_)ref_peak_mag_=last_peak_mag_;
            if(ref_peak_mag_>0&&last_peak_mag_<ref_peak_mag_*0.10f){if(++weak_sym_count_>=3){if(decoded_len_>=4)send_packet(payload_buf.data(),decoded_len_);reset_rx();return;}}else weak_sym_count_=0;}
        if(nz){new_pre_run_++;if(new_pre_run_>=NEW_PRE_DETECT){if(decoded_len_>=4)send_packet(payload_buf.data(),decoded_len_);reset_rx();preamble_run_=NEW_PRE_DETECT;if(preamble_run_>=PREAMBLE_DETECT){rx_state_=RxState::PRE_END;preamble_run_=0;}return;}}
        else new_pre_run_=0;
        const uint32_t chip_corr=((uint32_t)raw_up+chips_per_symbol-timing_corr_)%chips_per_symbol;
        uint32_t s=(chip_corr+chips_per_symbol-1u)%chips_per_symbol; if(use_ldro_)s>>=2u;
        const uint16_t sv=(uint16_t)(s^(s>>1u));
        sym_block_[sym_in_block_++]=sv;
        if(sym_in_block_>=coding_rate){process_sym_block();sym_in_block_=0;
            if(payload_len_target_&&decoded_len_>=payload_len_target_){send_packet(payload_buf.data(),payload_len_target_);reset_rx();return;}}
        payload_sym_count_++;
        if(payload_sym_count_>=PAYLOAD_SYM_LIMIT){if(decoded_len_>=4)send_packet(payload_buf.data(),decoded_len_);reset_rx();}
        break;}
    }
}

// execute(): chip-resample loop (verbatim logic), fed pre-decimated 625 kHz samples
static void execute(const complex16_t* p,size_t count){
    const size_t sps=samples_per_symbol;
    g_buf=p; g_count=count;
    for(size_t i=0;i<count;++i){
        if(realign_samples_>0){realign_samples_--;continue;}  // skip 0.25-sym SFD grid shift
        if(raw_sample_count_<MAX_FFT)fft_accum[raw_sample_count_]=p[i];  // collect consecutively
        if(++raw_sample_count_>=sps){
            g_pos=i;
            process_symbol_window(sps); g_window++;
            raw_sample_count_=0;
            // HUNT phase scan: shift window by sps/8 samples until a near-zero is found
            if(rx_state_==RxState::HUNT&&preamble_run_==0&&!snapped_&&realign_samples_==0)realign_samples_=sps/8u;
        }
    }
}

int main(int argc,char**argv){
    const char* in=argc>1?argv[1]:"rx625.iq16";
    FILE* f=fopen(in,"rb"); if(!f){printf("cannot open %s\n",in);return 1;}
    std::vector<complex16_t> samp; int16_t iq[2];
    while(fread(iq,sizeof(int16_t),2,f)==2) samp.push_back({iq[0],iq[1]});
    fclose(f);
    printf("loaded %zu samples @625kHz (=%.1f symbols)\n",samp.size(),samp.size()/(double)samples_per_symbol);
    reset_rx();
    execute(samp.data(),samp.size());
    printf("FINAL: decoded_len=%u state=%s\n",decoded_len_,SN(rx_state_));
    if(decoded_len_){printf("payload: ");for(int i=0;i<decoded_len_;i++)printf("%02x ",payload_buf[i]);printf("\n");}
    return 0;
}
