// Host test for MeshCrypto AES-CTR: verifies multi-block keystream matches the
// Meshtastic nonce/counter convention (4-byte big-endian counter in nonce[12..15]).
// --dump <len> prints hex of the ciphertext of a known plaintext so a reference
// AES-CTR (python) can confirm the round-trip. No args = self round-trip check.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../firmware/application/apps/mesh/mesh_crypto.hpp"

using namespace meshtastic;

int main(int argc, char** argv) {
    const uint32_t pid = 0x11223344, from = 0xDC63A709;
    MeshCrypto c;
    c.set_key(DEFAULT_PSK, sizeof(DEFAULT_PSK));

    if (argc > 2 && strcmp(argv[1], "--dump") == 0) {
        int L = atoi(argv[2]);
        uint8_t buf[256];
        for (int i = 0; i < L; i++) buf[i] = (uint8_t)i;   // plaintext 00 01 02 ...
        c.crypt(buf, L, pid, from);
        printf("PID %08x FROM %08x\nCT ", pid, from);
        for (int i = 0; i < L; i++) printf("%02x", buf[i]);
        printf("\n");
        return 0;
    }

    // self round-trip across a block boundary
    bool ok = true;
    for (int L : {1, 15, 16, 17, 40, 100, 200}) {
        uint8_t a[256], b[256];
        for (int i = 0; i < L; i++) a[i] = b[i] = (uint8_t)(i * 3 + 1);
        c.crypt(b, L, pid, from);            // encrypt
        bool changed = memcmp(a, b, L) != 0;
        c.crypt(b, L, pid, from);            // decrypt (CTR is symmetric)
        bool rt = memcmp(a, b, L) == 0;
        printf("L=%3d encrypt-changes=%d round-trip=%s\n", L, changed, rt ? "OK" : "FAIL");
        ok = ok && changed && rt;
    }
    printf("\n%s\n", ok ? "SELF-TEST PASS" : "SELF-TEST FAIL");
    return ok ? 0 : 1;
}
