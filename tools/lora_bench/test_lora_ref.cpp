// Does the chunk-by-chunk generated reference match the one computed in one pass?
#include <cstdio>
#include <cmath>
#include <vector>
static void stored(int sps, float ramp, std::vector<float>& out) {
    const float A = (float)M_PI / sps, B = -(float)M_PI + ramp;
    float zr = 1, zi = 0, dr = cosf(A + B), di = -sinf(A + B);
    const float Dr = cosf(2 * A), Di = -sinf(2 * A);
    out.resize(2 * sps);
    for (int n = 0; n < sps; n++) {
        out[2*n] = zr; out[2*n+1] = zi;
        float nzr = zr*dr - zi*di; zi = zr*di + zi*dr; zr = nzr;
        float ndr = dr*Dr - di*Di; di = dr*Di + di*Dr; dr = ndr;
        if ((n & 0xFF) == 0xFF) {
            float zs = 1/sqrtf(zr*zr+zi*zi); zr*=zs; zi*=zs;
            float ds = 1/sqrtf(dr*dr+di*di); dr*=ds; di*=ds;
        }
    }
}
static void chunked(int sps, float ramp, int parts, std::vector<float>& out) {
    out.resize(2 * sps);
    float zr = 1, zi = 0, dr = 1, di = 0;
    for (int part = 0; part < parts; part++) {
        int n0 = part * sps / parts, n1 = (part + 1) * sps / parts;
        if (part == 0) {
            const float A = (float)M_PI / sps, B = -(float)M_PI + ramp;
            zr = 1; zi = 0; dr = cosf(A + B); di = -sinf(A + B);
        }
        const float A2 = 2.0f * (float)M_PI / sps, Dr = cosf(A2), Di = -sinf(A2);
        for (int n = n0; n < n1; n++) {
            out[2*n] = zr; out[2*n+1] = zi;
            float nzr = zr*dr - zi*di; zi = zr*di + zi*dr; zr = nzr;
            float ndr = dr*Dr - di*Di; di = dr*Di + di*Dr; dr = ndr;
            if ((n & 0xFF) == 0xFF) {
                float zs = 1/sqrtf(zr*zr+zi*zi); zr*=zs; zi*=zs;
                float ds = 1/sqrtf(dr*dr+di*di); dr*=ds; di*=ds;
            }
        }
    }
}
int main() {
    int bad = 0;
    for (int sps : {1024, 2048, 4096})
        for (float ramp : {0.0f, 0.0013f, -0.0021f})
            for (int parts : {1, 2, 8, 32}) {
                std::vector<float> a, b;
                stored(sps, ramp, a); chunked(sps, ramp, parts, b);
                double worst = 0;
                for (int i = 0; i < 2 * sps; i++) worst = fmax(worst, fabs(a[i] - b[i]));
                if (worst > 1e-5) {
                    printf("FAIL sps=%d ramp=%g parts=%d worst=%g\n", sps, ramp, parts, worst);
                    bad++;
                }
            }
    printf("%s  generated reference matches the stored one\n", bad ? "FAILED" : "ok");
    return bad ? 1 : 0;
}
