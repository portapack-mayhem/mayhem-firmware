// Host harness that runs the FIRMWARE's SF11 streaming state machine VERBATIM,
// fed with a real 250 kHz capture into win11 (bypassing decim8), to catch porting
// bugs in execute_sf11 / win indexing / fine-align independent of the decimation.
// Build: g++ -O2 -std=c++17 fw_sf11_stream_host.cpp -o x ; ./x lf_b0_os1.iq16
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
typedef std::complex<float> cf;
struct complex8_t { int8_t i_, q_; int8_t real() const { return i_; } int8_t imag() const { return q_; } };

// ---- config (mirror firmware) ----
static const int SF = 11, N = 2048, CR_CW = 5;   // coding_rate (cw_len) = 5 for CR4/5
static const int SF11_SPS = 2048;
static const int SF11_WIN = 3 * SF11_SPS;
static const int SF11_DA = 2, SF11_NDA = 2*SF11_DA+1;
static const int SF11_FINE_SYMS = 14, SF11_PRE_MIN = 6;
static const float SF11_SHARP_THR = 0.02f;
static const int chips_per_symbol = N, coding_rate = CR_CW;
static const int spreading_factor = SF;
static const int MAX_PAYLOAD = 255;

// ---- buffers (win11/ref11/fft11 like the firmware union region) ----
static cf ref11[SF11_SPS], fft11[SF11_SPS];
static complex8_t win11[SF11_WIN];
static uint32_t win_w_ = 0, sf11_read_ = 0;

// ---- FFT verbatim ----
static void lora_fft_inplace(cf* d, int n){
    for(int i=1,j=0;i<n;++i){int b=n>>1;for(;j&b;b>>=1)j^=b;j^=b;if(i<j)std::swap(d[i],d[j]);}
    for(int len=2;len<=n;len<<=1){float a=-2.0f*(float)M_PI/len;cf w1(cosf(a),sinf(a));
        for(int i=0;i<n;i+=len){cf w(1,0);for(int j=0;j<len/2;++j){auto u=d[i+j];auto v=d[i+j+len/2]*w;d[i+j]=u+v;d[i+j+len/2]=u-v;w*=w1;}}}}

// ---- decode chain (verbatim from firmware sf11) ----
static uint8_t WSEQ[256];
static void init_wseq(){uint8_t s=0xFF;for(int i=0;i<256;i++){WSEQ[i]=s;uint8_t fb=((s>>7)^(s>>5)^(s>>4)^(s>>3))&1;s=(uint8_t)((s<<1)|fb);}}
static uint8_t sf11_hamn(uint16_t cw,int cw_len){int b3=(cw>>(cw_len-1))&1,b2=(cw>>(cw_len-2))&1,b1=(cw>>(cw_len-3))&1,b0=(cw>>(cw_len-4))&1;return (uint8_t)((b0<<3)|(b1<<2)|(b2<<1)|b3);}
static void sf11_deint(const uint16_t*sy,int sfa,int cwl,uint16_t*cw){
    for(int c=0;c<sfa;c++)cw[c]=0;
    for(int i=0;i<cwl;i++)for(int j=0;j<sfa;j++){int bit=(sy[i]>>(sfa-1-j))&1;int c=((i-j-1)%sfa+sfa)%sfa;cw[c]=(uint16_t)(cw[c]|(bit<<(cwl-1-i)));}}

// firmware members used by the decode
static uint16_t hdr_sym_[8]; static bool header_valid_; static uint8_t hdr_extra_nib_[8], hdr_extra_cnt_;
static uint16_t sym_block_[8]; static uint8_t sym_in_block_; static bool nibble_lo_valid_; static uint8_t nibble_lo_, whiten_state_;
static uint8_t payload_buf[256], decoded_len_, payload_len_target_;
static uint8_t lora_whiten_step(){uint8_t fb=((whiten_state_>>7)^(whiten_state_>>5)^(whiten_state_>>4)^(whiten_state_>>3))&1u;uint8_t o=whiten_state_;whiten_state_=(uint8_t)((whiten_state_<<1)|fb);return o;}
static void feed_nibble(uint8_t nib){if(!nibble_lo_valid_){nibble_lo_=nib;nibble_lo_valid_=true;}else{uint8_t b=(uint8_t)(((nib<<4)|nibble_lo_)^lora_whiten_step());if(decoded_len_<MAX_PAYLOAD)payload_buf[decoded_len_++]=b;nibble_lo_valid_=false;}}
static void process_sym_block(){
    const int cpb=spreading_factor,cr=coding_rate; uint8_t cw[12]={};
    for(int i=0;i<cr;i++)for(int j=0;j<cpb;j++){int c=((i-j-1)%cpb+cpb)%cpb;int bit=(sym_block_[i]>>(cpb-1-j))&1u;cw[c]=(uint8_t)(cw[c]|(bit<<(cr-1-i)));}
    for(int i=0;i<cpb;i++){uint8_t b0=(cw[i]>>(cr-1))&1,b1=(cw[i]>>(cr-2))&1,b2=(cw[i]>>(cr-3))&1,b3=(cw[i]>>(cr-4))&1;feed_nibble((uint8_t)((b3<<3)|(b2<<2)|(b1<<1)|b0));}}
static uint8_t decode_header(){
    const int sf_app=spreading_factor-2,cw_len=8; uint8_t cw[12]={};
    for(int i=0;i<cw_len;i++)for(int j=0;j<sf_app;j++){int c=((i-j-1)%sf_app+sf_app)%sf_app;int bit=(hdr_sym_[i]>>(sf_app-1-j))&1u;cw[c]=(uint8_t)(cw[c]|(bit<<(cw_len-1-i)));}
    uint8_t nib[12]={};
    for(int i=0;i<sf_app;i++){int d0=(cw[i]>>4)&1,d1=(cw[i]>>5)&1,d2=(cw[i]>>6)&1,d3=(cw[i]>>7)&1;int rp0=(cw[i]>>3)&1,rp1=(cw[i]>>2)&1,rp2=(cw[i]>>1)&1,rp3=cw[i]&1;int s0=rp0^(d3^d2^d1),s1=rp1^(d2^d1^d0),s2=rp2^(d3^d2^d0),s3=rp3^(d3^d1^d0);int syn=(s0<<3)|(s1<<2)|(s2<<1)|s3;if(syn==11)d3^=1;else if(syn==14)d2^=1;else if(syn==13)d1^=1;else if(syn==7)d0^=1;nib[i]=(uint8_t)((d0<<3)|(d1<<2)|(d2<<1)|d3);}
    hdr_extra_cnt_=(sf_app>5)?(uint8_t)(sf_app-5):0u; for(uint8_t k=0;k<hdr_extra_cnt_;++k)hdr_extra_nib_[k]=nib[5+k];
    uint8_t h0=nib[0],h1=nib[1],h2=nib[2];
    uint8_t c4=((h0>>3)&1)^((h0>>2)&1)^((h0>>1)&1)^(h0&1);
    uint8_t c3=((h0>>3)&1)^((h1>>3)&1)^((h1>>2)&1)^((h1>>1)&1)^(h2&1);
    uint8_t c2=((h0>>2)&1)^((h1>>3)&1)^(h1&1)^((h2>>3)&1)^((h2>>1)&1);
    uint8_t c1=((h0>>1)&1)^((h1>>2)&1)^(h1&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    uint8_t c0=(h0&1)^((h1>>1)&1)^((h2>>3)&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    header_valid_=((nib[3]&1)==c4 && (nib[4]&0xF)==((c3<<3)|(c2<<2)|(c1<<1)|c0));
    return (uint8_t)((nib[0]<<4)|nib[1]);
}

// ---- sf11 DSP (verbatim from firmware) ----
static float sf11_last_sharp_; static cf sf11_last_pk_;
static void sf11_recompute_ref(float cfo_ramp){
    const float sps=(float)SF11_SPS;
    for(int n=0;n<SF11_SPS;++n){float fn=(float)n;float ph=(float)M_PI*(fn*fn/sps-fn)+cfo_ramp*fn;ref11[n]=cf(cosf(ph),-sinf(ph));}}
static int32_t sf11_demod(uint32_t ws,bool is_up){
    for(int n=0;n<SF11_SPS;++n){auto&s=win11[(ws+n)%SF11_WIN];cf x((float)s.real(),(float)s.imag());fft11[n]=is_up?x*ref11[n]:x*std::conj(ref11[n]);}
    lora_fft_inplace(fft11,SF11_SPS);
    float pm=0,tot=0;uint32_t pb=0;
    for(int k=0;k<SF11_SPS;++k){float m=fft11[k].real()*fft11[k].real()+fft11[k].imag()*fft11[k].imag();tot+=m;if(m>pm){pm=m;pb=k;}}
    sf11_last_sharp_=tot>0?pm/tot:0;sf11_last_pk_=fft11[pb];return (int32_t)pb;}
static uint8_t sf11_decode_bins(const int16_t*bins,uint8_t nbins,uint32_t off,uint8_t*out,uint8_t out_cap){
    const int Nn=chips_per_symbol,SFv=spreading_factor;
    uint16_t g[8];for(int i=0;i<8;i++){int a=(((int)bins[i]-(int)off)%Nn+Nn)%Nn;a>>=2;g[i]=(uint16_t)(a^(a>>1));}
    uint16_t cw[16];sf11_deint(g,SFv-2,8,cw);uint8_t nib[12]={};
    for(int i=0;i<SFv-2;i++){int d0=(cw[i]>>4)&1,d1=(cw[i]>>5)&1,d2=(cw[i]>>6)&1,d3=(cw[i]>>7)&1;int rp0=(cw[i]>>3)&1,rp1=(cw[i]>>2)&1,rp2=(cw[i]>>1)&1,rp3=cw[i]&1;int s0=rp0^(d3^d2^d1),s1=rp1^(d2^d1^d0),s2=rp2^(d3^d2^d0),s3=rp3^(d3^d1^d0);int syn=(s0<<3)|(s1<<2)|(s2<<1)|s3;if(syn==11)d3^=1;else if(syn==14)d2^=1;else if(syn==13)d1^=1;else if(syn==7)d0^=1;nib[i]=(uint8_t)((d0<<3)|(d1<<2)|(d2<<1)|d3);}
    uint8_t h0=nib[0],h1=nib[1],h2=nib[2];
    uint8_t c4=((h0>>3)&1)^((h0>>2)&1)^((h0>>1)&1)^(h0&1);
    uint8_t c3=((h0>>3)&1)^((h1>>3)&1)^((h1>>2)&1)^((h1>>1)&1)^(h2&1);
    uint8_t c2=((h0>>2)&1)^((h1>>3)&1)^(h1&1)^((h2>>3)&1)^((h2>>1)&1);
    uint8_t c1=((h0>>1)&1)^((h1>>2)&1)^(h1&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    uint8_t c0=(h0&1)^((h1>>1)&1)^((h2>>3)&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    if((nib[3]&1)!=c4||(nib[4]&0xF)!=((c3<<3)|(c2<<2)|(c1<<1)|c0))return 0;
    uint8_t L=(uint8_t)((nib[0]<<4)|nib[1]);if(L<4||L>MAX_PAYLOAD)return 0;
    uint8_t pnib[64];int pc=0;for(int c=5;c<SFv-2&&pc<(int)sizeof(pnib);c++)pnib[pc++]=nib[c];
    const int cw_len=coding_rate;
    for(int bi=8;bi+cw_len<=nbins&&pc+SFv<=(int)sizeof(pnib);bi+=cw_len){uint16_t pg[16];for(int i=0;i<cw_len;i++){int a=(((int)bins[bi+i]-(int)off)%Nn+Nn)%Nn;pg[i]=(uint16_t)(a^(a>>1));}uint16_t pcw[16];sf11_deint(pg,SFv,cw_len,pcw);for(int c=0;c<SFv;c++)pnib[pc++]=sf11_hamn(pcw[c],cw_len);}
    uint8_t st=0xFF;int nbytes=pc/2;for(int i=0;i<nbytes&&i<out_cap;i++){uint8_t bv=(uint8_t)((pnib[2*i+1]<<4)|pnib[2*i]);uint8_t wb=st;uint8_t fb=((st>>7)^(st>>5)^(st>>4)^(st>>3))&1u;st=(uint8_t)((st<<1)|fb);out[i]=(uint8_t)(bv^wb);}
    return L;
}

// sf11 streaming state (mirror firmware)
enum St{HUNT,FINE,PAYLOAD}; static St sf11_state_;
static uint8_t sf11_pre_run_,sf11_sfd_step_,sf11_fine_sym_,sf11_fine_da_,sf11_confirmed_flag_; static int32_t sf11_prev_bin_; static uint32_t sf11_up_bin_,sf11_pre_pos_,sf11_off0_,sf11_h0_,sf11_win_off_; static cf sf11_cfo_acc_,sf11_prev_pk_; static bool sf11_confirmed_; static float sf11_up_sharp_cache_; static int16_t sf11_bins_[SF11_NDA][SF11_FINE_SYMS]; static uint16_t sf11_pay_have_; static int sf11_win_da_; static bool g_decoded=false; static uint8_t g_frame[300]; static int g_L=0;
static const int PAYLOAD_SYM_LIMIT=220;

static bool sf11_try_decode(){
    const int Nn=chips_per_symbol;
    for(int rad=0;rad<=SF11_DA;++rad)for(int s=0;s<(rad==0?1:2);++s){int da=(s==0)?-rad:rad;int di=da+SF11_DA;
        for(int od=-2;od<=2;++od){uint32_t off=(uint32_t)(((int)sf11_off0_+od)%Nn+Nn)%Nn;uint8_t fr[8];
            uint8_t L=sf11_decode_bins(sf11_bins_[di],SF11_FINE_SYMS,off,fr,sizeof(fr));
            if(L<4||fr[0]!=0xFF||fr[1]!=0xFF||fr[2]!=0xFF||fr[3]!=0xFF)continue;
            sf11_win_da_=da;sf11_win_off_=off;
            for(int i=0;i<8;i++){int a=(((int)sf11_bins_[di][i]-(int)off)%Nn+Nn)%Nn;a>>=2;hdr_sym_[i]=(uint16_t)(a^(a>>1));}
            uint8_t len=decode_header();if(!header_valid_||len<4||len>MAX_PAYLOAD)continue;
            payload_len_target_=len;sym_in_block_=0;nibble_lo_valid_=false;whiten_state_=0xFF;decoded_len_=0;
            for(uint8_t k=0;k<hdr_extra_cnt_;++k)feed_nibble(hdr_extra_nib_[k]);
            for(uint8_t i=8;i<SF11_FINE_SYMS;i++){uint32_t sv=(uint32_t)(((int)sf11_bins_[di][i]-(int)off)%Nn+Nn)%Nn;sym_block_[sym_in_block_++]=(uint16_t)(sv^(sv>>1));if(sym_in_block_>=coding_rate){process_sym_block();sym_in_block_=0;if(payload_len_target_&&decoded_len_>=payload_len_target_){g_decoded=true;g_L=payload_len_target_;memcpy(g_frame,payload_buf,g_L);return true;}}}
            sf11_read_=(uint32_t)((long)sf11_h0_+da+(long)SF11_FINE_SYMS*SF11_SPS);sf11_pay_have_=SF11_FINE_SYMS-8;sf11_state_=PAYLOAD;
            printf("[FINE win] da=%d off=%u L=%u\n",da,off,len);return true;}}
    return false;
}
static void sf11_reset(){sf11_state_=HUNT;sf11_pre_run_=0;sf11_prev_bin_=-99;sf11_confirmed_=false;sf11_sfd_step_=0;sf11_cfo_acc_=cf(0,0);sf11_prev_pk_=cf(0,0);sf11_fine_sym_=0;sf11_fine_da_=0;sf11_pay_have_=0;}

// run the state machine (budget-per-call like execute_sf11, but we call it repeatedly)
static void sf11_step(uint32_t budget){
    const int Nn=chips_per_symbol;
    while(budget>0){
        if(sf11_state_==HUNT){
            if(win_w_-sf11_read_<(uint32_t)SF11_SPS)break;
            if(!sf11_confirmed_){
                int32_t ub=sf11_demod(sf11_read_,true);budget--;float ush=sf11_last_sharp_;cf upk=sf11_last_pk_;
                int bd=(ub>sf11_prev_bin_)?(ub-sf11_prev_bin_):(sf11_prev_bin_-ub);
                if(ush>SF11_SHARP_THR&&sf11_prev_bin_>=0&&bd<=1){sf11_pre_run_++;sf11_up_bin_=ub;sf11_pre_pos_=sf11_read_;sf11_cfo_acc_+=upk*std::conj(sf11_prev_pk_);}
                else if(ush>SF11_SHARP_THR){sf11_pre_run_=1;sf11_up_bin_=ub;sf11_pre_pos_=sf11_read_;sf11_cfo_acc_=cf(0,0);}
                else sf11_pre_run_=0;
                sf11_prev_bin_=ub;sf11_prev_pk_=upk;
                if(sf11_pre_run_>=SF11_PRE_MIN){sf11_confirmed_=true;sf11_sfd_step_=0;}
                sf11_read_+=SF11_SPS;
            } else if(sf11_sfd_step_==0){sf11_demod(sf11_read_,true);budget--;sf11_up_sharp_cache_=sf11_last_sharp_;sf11_sfd_step_=1;}
            else{int32_t db=sf11_demod(sf11_read_,false);budget--;float dsh=sf11_last_sharp_;sf11_sfd_step_=0;
                if(dsh>SF11_SHARP_THR&&dsh>sf11_up_sharp_cache_){int diff=((int)sf11_up_bin_-db)%Nn;if(diff<0)diff+=Nn;if(diff>=Nn/2)diff-=Nn;int tau=diff/2;sf11_recompute_ref(std::arg(sf11_cfo_acc_)/(float)SF11_SPS);sf11_off0_=(uint32_t)(((int)sf11_up_bin_-tau)%Nn+Nn)%Nn;sf11_h0_=(uint32_t)((long)sf11_read_+(long)llroundf(2.25f*SF11_SPS)-tau);sf11_state_=FINE;sf11_fine_sym_=0;sf11_fine_da_=0;printf("[SFD] up=%u down=%d tau=%d off0=%u h0=%u\n",sf11_up_bin_,db,tau,sf11_off0_,sf11_h0_);}
                else{sf11_read_+=SF11_SPS;if(sf11_read_-sf11_pre_pos_>24u*SF11_SPS)sf11_reset();}}
        } else if(sf11_state_==FINE){
            int da=(int)sf11_fine_da_-SF11_DA;long ws=(long)sf11_h0_+da+(long)sf11_fine_sym_*SF11_SPS;
            if(ws<0||(long)win_w_-ws<(long)SF11_SPS)break;
            sf11_bins_[sf11_fine_da_][sf11_fine_sym_]=(int16_t)sf11_demod((uint32_t)ws,true);budget--;
            if(++sf11_fine_da_>=SF11_NDA){sf11_fine_da_=0;if(++sf11_fine_sym_>=SF11_FINE_SYMS){if(!sf11_try_decode())sf11_reset();}}
        } else { // PAYLOAD
            if(win_w_-sf11_read_<(uint32_t)SF11_SPS)break;
            int32_t raw=sf11_demod(sf11_read_,true);budget--;sf11_read_+=SF11_SPS;
            uint32_t sv=(uint32_t)(((int)raw-(int)sf11_win_off_)%Nn+Nn)%Nn;sym_block_[sym_in_block_++]=(uint16_t)(sv^(sv>>1));
            if(sym_in_block_>=coding_rate){process_sym_block();sym_in_block_=0;if(payload_len_target_&&decoded_len_>=payload_len_target_){g_decoded=true;g_L=payload_len_target_;memcpy(g_frame,payload_buf,g_L);return;}}
            if(++sf11_pay_have_>=PAYLOAD_SYM_LIMIT){sf11_reset();break;}
        }
    }
}

int main(int argc,char**argv){
    const char*fn=argc>1?argv[1]:"lf_b0_os1.iq16";
    FILE*f=fopen(fn,"rb");if(!f){printf("no file\n");return 1;}
    std::vector<complex8_t> samp; int16_t iq[2];
    // input is int16 @250kHz; scale down to int8 like the firmware win11 (test a shift range in main2)
    int shift=argc>2?atoi(argv[2]):5;   // emulate SF11_STORE_SHIFT (iq16 was scaled to ~6000 peak)
    while(fread(iq,2,2,f)==2){int a=iq[0]>>shift,b=iq[1]>>shift;if(a>127)a=127;if(a<-127)a=-127;if(b>127)b=127;if(b<-127)b=-127;samp.push_back({(int8_t)a,(int8_t)b});}
    fclose(f);
    init_wseq();sf11_reset();sf11_read_=0;win_w_=0;sf11_recompute_ref(0.0f);
    printf("loaded %zu samples (shift=%d, peak check)\n",samp.size(),shift);
    // feed in 256-sample chunks (like decim8 output per execute call), stepping the state machine
    size_t i=0;
    while(i<samp.size()&&!g_decoded){
        for(int k=0;k<256&&i<samp.size();k++,i++){win11[win_w_%SF11_WIN]=samp[i];win_w_++;}
        sf11_step(1);   // SF11_BUDGET=1
    }
    // drain
    for(int r=0;r<50&&!g_decoded;r++)sf11_step(1);
    if(g_decoded){printf("DECODED L=%d: ",g_L);for(int k=0;k<g_L&&k<16;k++)printf("%02x ",g_frame[k]);printf("\n dest=%02x%02x%02x%02x src=%02x%02x%02x%02x\n",g_frame[3],g_frame[2],g_frame[1],g_frame[0],g_frame[7],g_frame[6],g_frame[5],g_frame[4]);}
    else printf("NO DECODE (state=%d pre_run=%u confirmed=%d)\n",sf11_state_,sf11_pre_run_,sf11_confirmed_);
    return 0;
}
