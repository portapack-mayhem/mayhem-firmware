// Host test: gr-lora_sdr TX encode chain in C++ (to be ported into proc_lora_tx.cpp).
// Prints the chip sequence for a fixed payload; must match the validated Python encoder.
#include <cstdio>
#include <cstdint>
#include <cstring>

static const int SF = 7;
static const int N = 1 << SF;

static void int2bool(int v, int n, int* bits) { for (int i = 0; i < n; i++) bits[i] = (v >> (n - 1 - i)) & 1; }
static int bool2int(const int* bits, int n) { int x = 0; for (int i = 0; i < n; i++) x = (x << 1) | bits[i]; return x; }
static int gray_decode(int g) { int v = g, s = g >> 1; while (s) { v ^= s; s >>= 1; } return v; }

static uint8_t whiten_step(uint8_t* s) {
    uint8_t fb = ((*s >> 7) ^ (*s >> 5) ^ (*s >> 4) ^ (*s >> 3)) & 1u;
    uint8_t out = *s; *s = (uint8_t)((*s << 1) | fb); return out;
}

// gr-lora hamming_enc; cr_app=4 for header (cw_len=8), =1 for payload CR4/5 (cw_len=5)
static int hamming_enc(int nibble, int cr_app) {
    int d[4]; int2bool(nibble, 4, d);            // d[0]=MSB
    if (cr_app != 1) {
        int p0 = d[3]^d[2]^d[1], p1 = d[2]^d[1]^d[0], p2 = d[3]^d[2]^d[0], p3 = d[3]^d[1]^d[0];
        int full = (d[3]<<7)|(d[2]<<6)|(d[1]<<5)|(d[0]<<4)|(p0<<3)|(p1<<2)|(p2<<1)|p3;
        return full >> (4 - cr_app);
    } else {
        int p4 = d[0]^d[1]^d[2]^d[3];
        return (d[3]<<4)|(d[2]<<3)|(d[1]<<2)|(d[0]<<1)|p4;
    }
}

// gr-lora interleaver: sf_app codewords of cw_len bits -> cw_len symbols; chip=(gray_decode(g)+1)%N
static void interleave_chips(const int* codewords, int sf_app, int cw_len, bool ldro, int* out_chips) {
    int cw_bin[12][12];
    for (int k = 0; k < sf_app; k++) int2bool(codewords[k], cw_len, cw_bin[k]);
    int nbits = sf_app + (ldro ? 1 : 0);
    for (int i = 0; i < cw_len; i++) {
        int inter[12] = {0};
        for (int j = 0; j < sf_app; j++) {
            int idx = ((i - j - 1) % sf_app + sf_app) % sf_app;
            inter[j] = cw_bin[idx][i];
        }
        if (ldro) { int s = 0; for (int x = 0; x < sf_app; x++) s += inter[x]; inter[sf_app] = s & 1; }
        int g = bool2int(inter, nbits) << (SF - nbits);
        out_chips[i] = (gray_decode(g) + 1) % N;
    }
}

static void hdr_checksum(const int* h, int* c4out, int* cloout) {
    int c4 = ((h[0]>>3)&1)^((h[0]>>2)&1)^((h[0]>>1)&1)^(h[0]&1);
    int c3 = ((h[0]>>3)&1)^((h[1]>>3)&1)^((h[1]>>2)&1)^((h[1]>>1)&1)^(h[2]&1);
    int c2 = ((h[0]>>2)&1)^((h[1]>>3)&1)^(h[1]&1)^((h[2]>>3)&1)^((h[2]>>1)&1);
    int c1 = ((h[0]>>1)&1)^((h[1]>>2)&1)^(h[1]&1)^((h[2]>>2)&1)^((h[2]>>1)&1)^(h[2]&1);
    int c0 = (h[0]&1)^((h[1]>>1)&1)^((h[2]>>3)&1)^((h[2]>>2)&1)^((h[2]>>1)&1)^(h[2]&1);
    *c4out = c4; *cloout = (c3<<3)|(c2<<2)|(c1<<1)|c0;
}

static uint16_t crc16_step(uint16_t crc, uint8_t b) {
    for (int i = 0; i < 8; i++) {
        if (((crc & 0x8000) >> 8) ^ (b & 0x80)) crc = (uint16_t)((crc << 1) ^ 0x1021);
        else crc = (uint16_t)(crc << 1);
        b = (uint8_t)(b << 1);
    }
    return crc;
}
static uint16_t mesh_crc(const uint8_t* p, int len) {
    uint16_t crc = 0; for (int i = 0; i < len - 2; i++) crc = crc16_step(crc, p[i]);
    return (uint16_t)(crc ^ p[len-1] ^ (p[len-2] << 8));
}

// Build the data-symbol chip sequence (header(8) + payload), cr=1 (4/5), has_crc=1
static int build_chips(const uint8_t* payload, int len, int* chips) {
    int nib[600]; int nn = 0;
    int h[3] = { len >> 4, len & 0xF, (1 << 1) | 1 };   // cr_code=1, crc=1
    int c4, clo; hdr_checksum(h, &c4, &clo);
    nib[nn++] = h[0]; nib[nn++] = h[1]; nib[nn++] = h[2]; nib[nn++] = c4; nib[nn++] = clo;
    uint8_t ws = 0xFF;
    for (int i = 0; i < len; i++) { uint8_t wb = (uint8_t)(payload[i] ^ whiten_step(&ws)); nib[nn++] = wb & 0xF; nib[nn++] = (wb >> 4) & 0xF; }
    uint16_t crc = mesh_crc(payload, len);
    nib[nn++] = crc & 0xF; nib[nn++] = (crc>>4)&0xF; nib[nn++] = (crc>>8)&0xF; nib[nn++] = (crc>>12)&0xF;

    int nc = 0;
    // header block: first SF-2 nibbles, cr_app=4, sf_app=SF-2, cw_len=8, ldro
    int hcw[12]; for (int i = 0; i < SF-2; i++) hcw[i] = hamming_enc(nib[i], 4);
    interleave_chips(hcw, SF-2, 8, true, chips + nc); nc += 8;
    // payload blocks: nibbles[SF-2:], cr_app=1, sf_app=SF, cw_len=5
    for (int bi = SF-2; bi < nn; bi += SF) {
        int cw[12]; for (int i = 0; i < SF; i++) cw[i] = (bi+i < nn) ? hamming_enc(nib[bi+i], 1) : 0;
        interleave_chips(cw, SF, 5, false, chips + nc); nc += 5;
    }
    return nc;
}

int main() {
    uint8_t payload[30];
    uint8_t pre[8] = {0xff,0xff,0xff,0xff,0x12,0x34,0x56,0x78};
    memcpy(payload, pre, 8);
    for (int i = 0; i < 22; i++) payload[8+i] = (uint8_t)(0x10 + i);
    int chips[256];
    int nc = build_chips(payload, 30, chips);
    printf("nsyms=%d\nCHIPS:", nc);
    for (int i = 0; i < nc; i++) printf(" %d", chips[i]);
    printf("\n");
    return 0;
}
