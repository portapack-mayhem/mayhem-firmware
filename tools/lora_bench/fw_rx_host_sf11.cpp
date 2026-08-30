// Host harness: LONG_FAST (SF11/BW250) RX decode of a REAL captured burst.
// Implements the SFD-based CFO/timing separation and the SF>7 payload-chain fix
// (the LoRa header block carries SF-2 nibbles = 5 header + SF-7 payload nibbles).
// Validated byte-exact against a real Heltec LONG_FAST capture (broadcast + src).
//
// Input: interleaved int16 IQ, already mixed to DC and decimated to dec_fs.
//   OS=2  -> dec_fs=500 kHz, sps=4096   OS=4 -> dec_fs=1 MHz, sps=8192.
// Build: g++ -O2 -std=c++17 fw_rx_host_sf11.cpp -o fw_rx_host_sf11
// Run:   ./fw_rx_host_sf11 lf_b0_os2.iq16 4096
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

// ---- LONG_FAST config ----
static const int      SF = 11;
static const int      N  = 1 << SF;          // 2048 chips
static const int      CR = 1;                // 4/5 (Meshtastic default)
static int            SPS = 4096;            // samples/symbol (set from argv)
static int            OS  = 2;               // oversampling = SPS/N
static const double   BW  = 250000.0;

typedef std::complex<float> cf;
static std::vector<cf> g_sig;                // whole capture (DC-mixed, decimated)

// ---- FFT (verbatim from fw_rx_host.cpp) ----
static void fft_inplace(cf* data, int n){
    for(int i=1,j=0;i<n;++i){int bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(data[i],data[j]);}
    for(int len=2;len<=n;len<<=1){const float ang=-2.0f*(float)M_PI/(float)len;const cf wlen(cosf(ang),sinf(ang));
        for(int i=0;i<n;i+=len){cf w(1,0);for(int j=0;j<len/2;++j){auto u=data[i+j];auto v=data[i+j+len/2]*w;data[i+j]=u+v;data[i+j+len/2]=u-v;w*=wlen;}}}}

// dechirp (is_up=true -> multiply by conj upchirp; false -> multiply by upchirp) then
// FFT(sps) and fold os aliases -> N bins.  Returns folded complex spectrum in out[N].
static std::vector<cf> g_fft;
static void dechirp_fold(long start, bool is_up, cf* out){
    if((int)g_fft.size()<SPS) g_fft.resize(SPS);
    const float inv_os=1.0f/(float)OS;
    for(int n=0;n<SPS;++n){
        const float ph=(float)M_PI*inv_os*((float)((long)n*n)/(float)SPS-(float)n);
        const float cq=cosf(ph),sq=sinf(ph);
        const cf x=g_sig[start+n];
        const float sr=x.real(),si=x.imag();
        if(is_up) g_fft[n]=cf(sr*cq+si*sq, si*cq-sr*sq);   // x*exp(-j ph)
        else      g_fft[n]=cf(sr*cq-si*sq, sr*sq+si*cq);   // x*exp(+j ph)
    }
    fft_inplace(g_fft.data(),SPS);
    for(int k=0;k<N;++k){cf acc(0,0);for(int m=0;m<OS;++m)acc+=g_fft[k+m*N];out[k]=acc;}
}
// up/down demod: returns argmax bin; fills mag ratio (peak/median) in *sharp; peak complex in *pk
static std::vector<cf> g_fold;
static int demod(long start, bool is_up, float* sharp=nullptr, cf* pk=nullptr){
    if((int)g_fold.size()<N) g_fold.resize(N);
    dechirp_fold(start,is_up,g_fold.data());
    float best=-1; int bb=0; std::vector<float> mags(N);
    for(int k=0;k<N;++k){float m=std::abs(g_fold[k]);mags[k]=m;if(m>best){best=m;bb=k;}}
    if(sharp){std::nth_element(mags.begin(),mags.begin()+N/2,mags.end());float med=mags[N/2];*sharp=best/(med+1e-9f);}
    if(pk)*pk=g_fold[bb];
    return bb;
}

// ---- decode chain (gr-lora_sdr; matches lora_gold.py) ----
static uint8_t WSEQ[256];
static void init_wseq(){uint8_t s=0xFF;for(int i=0;i<256;i++){WSEQ[i]=s;uint8_t fb=((s>>7)^(s>>5)^(s>>4)^(s>>3))&1;s=(uint8_t)((s<<1)|fb);}}
static inline int gray(int v){return v^(v>>1);}
// deinterleave cw_len symbols of sf_app bits -> sf_app codewords of cw_len bits
static void deinterleave(const int* syms,int sf_app,int cw_len,int* cw){
    for(int c=0;c<sf_app;c++)cw[c]=0;
    for(int i=0;i<cw_len;i++)for(int j=0;j<sf_app;j++){
        int bit=(syms[i]>>(sf_app-1-j))&1;
        int c=((i-j-1)%sf_app+sf_app)%sf_app;
        cw[c]|=bit<<(cw_len-1-i);
    }
}
static inline int ham_nibble(int cw,int cw_len){   // reversed top-4 bits
    int b3=(cw>>(cw_len-1))&1,b2=(cw>>(cw_len-2))&1,b1=(cw>>(cw_len-3))&1,b0=(cw>>(cw_len-4))&1;
    return (b0<<3)|(b1<<2)|(b2<<1)|b3;
}
// header block: 8 symbols, sf_app=SF-2, reduced-rate (>>2) -> SF-2 nibbles
static void decode_header_nibs(const int* rawsyms,int off,int* nib){
    int g[8];
    for(int i=0;i<8;i++){int a=((rawsyms[i]-off-1)%N+N)%N;a>>=2;g[i]=gray(a);}
    int cw[16]; deinterleave(g,SF-2,8,cw);
    for(int c=0;c<SF-2;c++)nib[c]=ham_nibble(cw[c],8);
}
static bool hdr_checksum_ok(const int* nib){
    int h0=nib[0],h1=nib[1],h2=nib[2];
    int c4=((h0>>3)&1)^((h0>>2)&1)^((h0>>1)&1)^(h0&1);
    int c3=((h0>>3)&1)^((h1>>3)&1)^((h1>>2)&1)^((h1>>1)&1)^(h2&1);
    int c2=((h0>>2)&1)^((h1>>3)&1)^(h1&1)^((h2>>3)&1)^((h2>>1)&1);
    int c1=((h0>>1)&1)^((h1>>2)&1)^(h1&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    int c0=(h0&1)^((h1>>1)&1)^((h2>>3)&1)^((h2>>2)&1)^((h2>>1)&1)^(h2&1);
    int clo=(c3<<3)|(c2<<2)|(c1<<1)|c0;
    return (nib[3]&1)==c4 && (nib[4]&0xF)==clo;
}
// full decode -> de-whitened frame bytes; returns length
static int decode_frame(const int* rawsyms,int nsyms,int off,uint8_t* out){
    int hnib[16]; decode_header_nibs(rawsyms,off,hnib);
    if(!hdr_checksum_ok(hnib)) return -1;
    int L=(hnib[0]<<4)|hnib[1];
    // payload nibbles: first the (SF-2-5) extra header-block nibbles, then payload blocks
    static int pnib[1024]; int pc=0;
    for(int c=5;c<SF-2;c++)pnib[pc++]=hnib[c];        // <<< the SF>7 chain fix
    const int cw_len=CR+4;
    for(int bi=8; bi+cw_len<=nsyms; bi+=cw_len){
        int g[16];
        for(int i=0;i<cw_len;i++){int a=((rawsyms[bi+i]-off-1)%N+N)%N;g[i]=gray(a);}
        int cw[16]; deinterleave(g,SF,cw_len,cw);
        for(int c=0;c<SF;c++)pnib[pc++]=ham_nibble(cw[c],cw_len);
        if(pc>= (L+2)*2 ) break;
    }
    int nbytes=pc/2; if(nbytes>L+2)nbytes=L+2;
    for(int i=0;i<nbytes;i++){
        uint8_t b=(uint8_t)((pnib[2*i+1]<<4)|pnib[2*i]);
        out[i]=b^WSEQ[i];
    }
    return L;
}

int main(int argc,char**argv){
    const char* fn=argc>1?argv[1]:"lf_b0_os2.iq16";
    if(argc>2){SPS=atoi(argv[2]);OS=SPS/N;}
    init_wseq();
    FILE* f=fopen(fn,"rb"); if(!f){printf("cannot open %s\n",fn);return 1;}
    int16_t iq[2];
    while(fread(iq,sizeof(int16_t),2,f)==2)g_sig.push_back(cf((float)iq[0],(float)iq[1]));
    fclose(f);
    const int total=(int)g_sig.size();
    printf("loaded %d samples (SF%d OS%d sps%d N%d) = %.1f symbols\n",total,SF,OS,SPS,N,total/(double)SPS);

    // 1) fractional CFO from preamble phase slope (windows 3..15), remove it
    cf acc(0,0),prev(0,0);
    for(int k=3;k<16;k++){cf pk;demod((long)k*SPS,true,nullptr,&pk);if(k>3)acc+=pk*std::conj(prev);prev=pk;}
    float Tsym=(float)SPS/ (float)(OS*BW);      // = N/BW
    float f_frac=std::arg(acc)/(2.0f*(float)M_PI*Tsym);
    printf("fractional CFO %.1f Hz (%.3f bins)\n",f_frac,f_frac/(float)(BW/N));
    for(int n=0;n<total;n++){float ph=-2.0f*(float)M_PI*f_frac*n/(float)(OS*BW);g_sig[n]*=cf(cosf(ph),sinf(ph));}

    // 2) preamble run (stable up-bin) + SFD (strong down-bin)
    int nwin=total/SPS;
    std::vector<int> ub(nwin); std::vector<float> us(nwin);
    for(int k=0;k<nwin;k++)ub[k]=demod((long)k*SPS,true,&us[k]);
    int p0=-1,p1=-1;
    for(int i=0;i<nwin;){
        if(us[i]>8){int j=i;while(j<nwin&&abs(ub[j]-ub[i])<=1&&us[j]>8)j++;if(p0<0||j-i>p1-p0){p0=i;p1=j;}i=std::max(j,i+1);}else i++;
    }
    // median up_bin over preamble
    std::vector<int> pb(ub.begin()+p0,ub.begin()+p1); std::sort(pb.begin(),pb.end());
    int up_bin=pb[pb.size()/2];
    // SFD: strongest down-ref window in the few after the preamble run
    int sfd_k=-1,down_bin=0; float best_ds=-1;
    for(int k=p1;k<std::min(p1+6,nwin);k++){float ds;int db=demod((long)k*SPS,false,&ds);if(ds>best_ds&&ds>us[k]){best_ds=ds;sfd_k=k;down_bin=db;}}
    printf("preamble sym[%d..%d] up_bin=%d ; SFD sym %d down_bin=%d\n",p0,p1-1,up_bin,sfd_k,down_bin);

    // 3) CFO/timing separation: tau=(up-down)/2 (bins), cfo=up-tau
    auto wrap=[&](int x){int m=((x%N)+N)%N;return m>=N/2?m-N:m;};
    int tau_bins=wrap(up_bin-down_bin)/2;
    int tau_samp=tau_bins*OS;
    printf("tau=%d bins (%d samp)\n",tau_bins,tau_samp);

    // 4) header anchor = (preamble_end + 2 sync + 2.25 SFD) - timing snap; off from snapped preamble
    long HDR=(long)llround((p1+4.25)*SPS)-tau_samp;
    long presnap=(long)p0*SPS-tau_samp+3*SPS;
    int off0=demod(presnap,true)-1;

    // 5) small fine-timing sweep; accept a valid-checksum broadcast frame
    uint8_t frame[300]; int bestL=-1; long bestda=0; uint8_t bestframe[300];
    bool got=false;
    for(int da=-6;da<=6 && !got;da++){
        long hs=HDR+da; if(hs<0||hs+34L*SPS>total)continue;
        int nsy=std::min(34,(int)((total-hs)/SPS));
        std::vector<int> sy(nsy);
        for(int k=0;k<nsy;k++)sy[k]=demod(hs+(long)k*SPS,true);
        for(int off=off0-1;off<=off0+1;off++){
            int L=decode_frame(sy.data(),nsy,off,frame);
            if(L>=8&&L<=64&&frame[0]==0xff&&frame[1]==0xff&&frame[2]==0xff&&frame[3]==0xff){
                got=true;bestL=L;bestda=da;memcpy(bestframe,frame,L);
                printf("DECODE off=%d da=%d L=%d\n",off,da,L);break;
            }
        }
    }
    if(bestL>0){
        printf("frame (%d B): ",bestL);for(int i=0;i<bestL;i++)printf("%02x ",bestframe[i]);printf("\n");
        printf("dest=%02x%02x%02x%02x src=%02x%02x%02x%02x id=%02x%02x%02x%02x flags=%02x chanHash=%02x\n",
            bestframe[3],bestframe[2],bestframe[1],bestframe[0],
            bestframe[7],bestframe[6],bestframe[5],bestframe[4],
            bestframe[11],bestframe[10],bestframe[9],bestframe[8],bestframe[12],bestframe[13]);
    } else printf("no valid broadcast frame decoded\n");
    return 0;
}
