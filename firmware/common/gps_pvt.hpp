/*
 * Copyright (C) 2026 PortaPack Mayhem Contributors
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

#ifndef __GPS_PVT_H__
#define __GPS_PVT_H__

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>

#include "gps_scan_types.hpp"

namespace gps_scan {

// Exact IS-GPS-200 / WGS-84 constants
static constexpr double kGpsPi = 3.1415926535898;
static constexpr double kGpsMu = 3.986005e14;
static constexpr double kGpsOmegaE = 7.2921151467e-5;
static constexpr double kGpsC = 299792458.0;
static constexpr double kGpsFRel = -4.442807633e-10;
static constexpr double kWgs84A = 6378137.0;
static constexpr double kWgs84F = 1.0 / 298.257223563;
static constexpr double kWgs84E2 = kWgs84F * (2.0 - kWgs84F);

static constexpr double kP2_m5 = 1.0 / 32.0;
static constexpr double kP2_m19 = 1.0 / 524288.0;
static constexpr double kP2_m29 = 1.0 / 536870912.0;
static constexpr double kP2_m31 = 1.0 / 2147483648.0;
static constexpr double kP2_m33 = 1.0 / 8589934592.0;
static constexpr double kP2_m43 = 1.0 / 8796093022208.0;
static constexpr double kP2_m55 = 1.0 / 36028797018963968.0;

static inline int32_t sign_ext(uint32_t v, int bits) {
    const uint32_t m = 1U << (bits - 1);
    return static_cast<int32_t>((v ^ m) - m);
}

static inline double wrap_week_sec(double dt) {
    if (dt > 302400.0) dt -= 604800.0;
    if (dt < -302400.0) dt += 604800.0;
    return dt;
}

struct Ephemeris {
    uint8_t prn{0};
    uint8_t sf_valid_mask{0};  // bits 0..2 => SF1, SF2, SF3 received
    uint16_t gps_week{0};
    uint8_t sv_health{0xFF};
    uint8_t ura{0xFF};
    uint16_t iodc{0};
    uint8_t iode2{0xFF};
    uint8_t iode3{0xFE};

    // SF1 clock
    double t_gd{0.0};
    double t_oc{0.0};
    double a_f0{0.0};
    double a_f1{0.0};
    double a_f2{0.0};

    // SF2 orbit
    double c_rs{0.0};
    double delta_n{0.0};
    double m_0{0.0};
    double c_uc{0.0};
    double e{0.0};
    double c_us{0.0};
    double sqrt_a{0.0};
    double t_oe{0.0};

    // SF3 orbit
    double c_ic{0.0};
    double omega_0{0.0};
    double c_is{0.0};
    double i_0{0.0};
    double c_rc{0.0};
    double omega{0.0};
    double omega_dot{0.0};
    double idot{0.0};

    bool is_complete() const {
        return (sf_valid_mask & 0x07U) == 0x07U &&
               (iode2 == iode3) &&
               ((iodc & 0xFFU) == iode2) &&
               (sqrt_a > 4000.0);
    }

    bool ingest_subframe(uint8_t in_prn, uint8_t sf_id, const uint32_t w[10]) {
        if (in_prn < 1 || in_prn > NUM_PRNS) return false;
        if (prn != in_prn) {
            *this = Ephemeris{};
            prn = in_prn;
        }

        if (sf_id == 1) {
            gps_week = static_cast<uint16_t>(2048U + ((w[2] >> 14) & 0x3FFU));
            ura = static_cast<uint8_t>((w[2] >> 8) & 0x0FU);
            sv_health = static_cast<uint8_t>((w[2] >> 2) & 0x3FU);
            const uint16_t iodc_hi = static_cast<uint16_t>(w[2] & 0x03U);
            const uint16_t iodc_lo = static_cast<uint16_t>((w[7] >> 16) & 0xFFU);
            iodc = static_cast<uint16_t>((iodc_hi << 8) | iodc_lo);

            t_gd = static_cast<double>(sign_ext(w[6] & 0xFFU, 8)) * kP2_m31;
            t_oc = static_cast<double>(w[7] & 0xFFFFU) * 16.0;
            a_f2 = static_cast<double>(sign_ext((w[8] >> 16) & 0xFFU, 8)) * kP2_m55;
            a_f1 = static_cast<double>(sign_ext(w[8] & 0xFFFFU, 16)) * kP2_m43;
            a_f0 = static_cast<double>(sign_ext((w[9] >> 2) & 0x3FFFFFU, 22)) * kP2_m31;
            sf_valid_mask |= 0x01U;
            return true;
        }

        if (sf_id == 2) {
            iode2 = static_cast<uint8_t>((w[2] >> 16) & 0xFFU);
            c_rs = static_cast<double>(sign_ext(w[2] & 0xFFFFU, 16)) * kP2_m5;
            delta_n = static_cast<double>(sign_ext((w[3] >> 8) & 0xFFFFU, 16)) * kP2_m43 * kGpsPi;
            const uint32_t m0_u = ((w[3] & 0xFFU) << 24) | (w[4] & 0xFFFFFFU);
            m_0 = static_cast<double>(static_cast<int32_t>(m0_u)) * kP2_m31 * kGpsPi;
            c_uc = static_cast<double>(sign_ext((w[5] >> 8) & 0xFFFFU, 16)) * kP2_m29;
            const uint32_t e_u = ((w[5] & 0xFFU) << 24) | (w[6] & 0xFFFFFFU);
            e = static_cast<double>(e_u) * kP2_m33;
            c_us = static_cast<double>(sign_ext((w[7] >> 8) & 0xFFFFU, 16)) * kP2_m29;
            const uint32_t sqa_u = ((w[7] & 0xFFU) << 24) | (w[8] & 0xFFFFFFU);
            sqrt_a = static_cast<double>(sqa_u) * kP2_m19;
            t_oe = static_cast<double>((w[9] >> 8) & 0xFFFFU) * 16.0;
            sf_valid_mask |= 0x02U;
            return true;
        }

        if (sf_id == 3) {
            c_ic = static_cast<double>(sign_ext((w[2] >> 8) & 0xFFFFU, 16)) * kP2_m29;
            const uint32_t om0_u = ((w[2] & 0xFFU) << 24) | (w[3] & 0xFFFFFFU);
            omega_0 = static_cast<double>(static_cast<int32_t>(om0_u)) * kP2_m31 * kGpsPi;
            c_is = static_cast<double>(sign_ext((w[4] >> 8) & 0xFFFFU, 16)) * kP2_m29;
            const uint32_t i0_u = ((w[4] & 0xFFU) << 24) | (w[5] & 0xFFFFFFU);
            i_0 = static_cast<double>(static_cast<int32_t>(i0_u)) * kP2_m31 * kGpsPi;
            c_rc = static_cast<double>(sign_ext((w[6] >> 8) & 0xFFFFU, 16)) * kP2_m5;
            const uint32_t w_u = ((w[6] & 0xFFU) << 24) | (w[7] & 0xFFFFFFU);
            omega = static_cast<double>(static_cast<int32_t>(w_u)) * kP2_m31 * kGpsPi;
            omega_dot = static_cast<double>(sign_ext(w[8] & 0xFFFFFFU, 24)) * kP2_m43 * kGpsPi;
            iode3 = static_cast<uint8_t>((w[9] >> 16) & 0xFFU);
            idot = static_cast<double>(sign_ext((w[9] >> 2) & 0x3FFFU, 14)) * kP2_m43 * kGpsPi;
            sf_valid_mask |= 0x04U;
            return true;
        }

        return false;
    }
};

struct SvState {
    double x{0.0};
    double y{0.0};
    double z{0.0};
    double dt_sv{0.0};  // Satellite clock correction (seconds, including relativistic + T_GD)
};

// Compute SV ECEF position (meters) and clock correction (seconds) at raw transmit time t_tx (IS-GPS-200 Table 20-IV).
static inline SvState compute_sv_state(const Ephemeris& ep, double t_tx) {
    SvState sv{};
    const double A = ep.sqrt_a * ep.sqrt_a;
    const double n_0 = sqrt(kGpsMu / (A * A * A));
    const double n = n_0 + ep.delta_n;

    // First-pass clock correction to refine t_sv
    double dt_oc = wrap_week_sec(t_tx - ep.t_oc);
    double dt_clk = ep.a_f0 + ep.a_f1 * dt_oc + ep.a_f2 * dt_oc * dt_oc - ep.t_gd;
    double t_sv = t_tx - dt_clk;

    double t_k = wrap_week_sec(t_sv - ep.t_oe);
    double M_k = ep.m_0 + n * t_k;

    // Solve Kepler's equation E_k - e*sin(E_k) = M_k via Newton-Raphson
    double E_k = M_k;
    for (int iter = 0; iter < 8; iter++) {
        const double f = E_k - ep.e * sin(E_k) - M_k;
        const double fp = 1.0 - ep.e * cos(E_k);
        E_k -= f / fp;
    }

    const double sin_E = sin(E_k);

    // Relativistic clock correction + refined t_sv
    const double dt_rel = kGpsFRel * ep.e * ep.sqrt_a * sin_E;
    sv.dt_sv = dt_clk + dt_rel;

    // Recompute t_k with relativistic term for millimeter precision
    t_k = wrap_week_sec((t_tx - sv.dt_sv) - ep.t_oe);
    M_k = ep.m_0 + n * t_k;
    E_k = M_k;
    for (int iter = 0; iter < 6; iter++) {
        E_k -= (E_k - ep.e * sin(E_k) - M_k) / (1.0 - ep.e * cos(E_k));
    }
    const double sE = sin(E_k);
    const double cE = cos(E_k);
    const double one_m_ec = 1.0 - ep.e * cE;

    const double sin_nu = (sqrt(1.0 - ep.e * ep.e) * sE) / one_m_ec;
    const double cos_nu = (cE - ep.e) / one_m_ec;
    const double phi_k = atan2(sin_nu, cos_nu) + ep.omega;

    const double sin_2phi = sin(2.0 * phi_k);
    const double cos_2phi = cos(2.0 * phi_k);

    const double du_k = ep.c_us * sin_2phi + ep.c_uc * cos_2phi;
    const double dr_k = ep.c_rs * sin_2phi + ep.c_rc * cos_2phi;
    const double di_k = ep.c_is * sin_2phi + ep.c_ic * cos_2phi;

    const double u_k = phi_k + du_k;
    const double r_k = A * one_m_ec + dr_k;
    const double i_k = ep.i_0 + di_k + ep.idot * t_k;

    const double xp = r_k * cos(u_k);
    const double yp = r_k * sin(u_k);

    const double om_k = ep.omega_0 + (ep.omega_dot - kGpsOmegaE) * t_k - kGpsOmegaE * ep.t_oe;
    const double cos_om = cos(om_k);
    const double sin_om = sin(om_k);
    const double cos_i = cos(i_k);
    const double sin_i = sin(i_k);

    sv.x = xp * cos_om - yp * cos_i * sin_om;
    sv.y = xp * sin_om + yp * cos_i * cos_om;
    sv.z = yp * sin_i;
    return sv;
}

struct PvtFix {
    bool valid{false};
    uint8_t num_used{0};
    uint8_t reject_reason{0};  // 0=none/ok, 1=<4ch, 2=dt, 3=sing, 4=conv, 5=san
    double lat_deg{0.0};
    double lon_deg{0.0};
    double alt_m{0.0};
    double clk_bias_m{0.0};
    double pdop{99.9};
    double hdop{99.9};
    double rms_res_m{999.9};
    uint32_t gps_tow_sec{0};
    uint16_t gps_week{0};
};

// Solve 4x4 linear system A * x = b and optionally compute A^-1 via Gauss-Jordan with partial pivoting.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline))
#endif
static bool invert_4x4(const double A[4][4], double inv[4][4]) {
    double M[4][8];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            M[i][j] = A[i][j];
            M[i][j + 4] = (i == j) ? 1.0 : 0.0;
        }
    }

    for (int col = 0; col < 4; col++) {
        int piv = col;
        double max_v = fabs(M[col][col]);
        for (int r = col + 1; r < 4; r++) {
            const double v = fabs(M[r][col]);
            if (v > max_v) {
                max_v = v;
                piv = r;
            }
        }
        if (max_v < 1e-12) return false;
        if (piv != col) {
            for (int c = 0; c < 8; c++) {
                const double tmp = M[col][c];
                M[col][c] = M[piv][c];
                M[piv][c] = tmp;
            }
        }

        const double diag = M[col][col];
        for (int c = 0; c < 8; c++) M[col][c] /= diag;

        for (int r = 0; r < 4; r++) {
            if (r == col) continue;
            const double f = M[r][col];
            for (int c = 0; c < 8; c++) M[r][c] -= f * M[col][c];
        }
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            inv[i][j] = M[i][j + 4];
        }
    }
    return true;
}

static inline void ecef_to_geodetic(double x, double y, double z, double& lat_deg, double& lon_deg, double& alt_m) {
    const double p = sqrt(x * x + y * y);
    const double lon = atan2(y, x);
    double lat = atan2(z, p * (1.0 - kWgs84E2));
    double N = kWgs84A;
    for (int iter = 0; iter < 6; iter++) {
        const double s = sin(lat);
        N = kWgs84A / sqrt(1.0 - kWgs84E2 * s * s);
        lat = atan2(z + kWgs84E2 * N * s, p);
    }
    const double c = cos(lat);
    const double s = sin(lat);
    double alt;
    if (fabs(c) > 0.1) {
        alt = p / c - N;
    } else {
        alt = z / s - N * (1.0 - kWgs84E2);
    }
    lat_deg = lat * (180.0 / kGpsPi);
    lon_deg = lon * (180.0 / kGpsPi);
    alt_m = alt;
}

// Compute 3D PVT fix from simultaneous NavObsPacket and cached Ephemerides.
static inline PvtFix solve_pvt(
    const NavObsPacket& obs,
    const Ephemeris* ephem_table,
    size_t ephem_count) {
    PvtFix fix{};

    double sv_x[MAX_NAV_CH], sv_y[MAX_NAV_CH], sv_z[MAX_NAV_CH];
    double rho[MAX_NAV_CH];
    double t_tx_arr[MAX_NAV_CH];
    double dt_sv_arr[MAX_NAV_CH];
    size_t n_valid = 0;
    double max_tx = -1.0;

    // 1. Collect all locked, frame-synced channels with complete ephemeris
    for (size_t i = 0; i < obs.num_ch && i < MAX_NAV_CH; i++) {
        const auto& c = obs.ch[i];
        if (c.state != static_cast<uint8_t>(TrackState::Locked) &&
            c.state != static_cast<uint8_t>(TrackState::WidePLL)) continue;
        if (!c.frame_synced || !c.bit_synced) continue;

        const Ephemeris* ep = nullptr;
        for (size_t e = 0; e < ephem_count; e++) {
            if (ephem_table[e].prn == c.prn && ephem_table[e].is_complete()) {
                ep = &ephem_table[e];
                break;
            }
        }
        if (!ep) continue;

        const double t_tx = static_cast<double>(c.sf_start_tow) +
                            static_cast<double>(c.bit_in_sf) * 0.020 +
                            static_cast<double>(c.ms_in_bit) * 0.001 +
                            (static_cast<double>(c.chip_q20) / 1048576.0) / 1023000.0;

        const SvState sv = compute_sv_state(*ep, t_tx);
        sv_x[n_valid] = sv.x;
        sv_y[n_valid] = sv.y;
        sv_z[n_valid] = sv.z;
        t_tx_arr[n_valid] = t_tx;
        dt_sv_arr[n_valid] = sv.dt_sv;
        if (ep->gps_week > 0) fix.gps_week = ep->gps_week;

        if (max_tx < 0.0 || wrap_week_sec(t_tx - max_tx) > 0.0) {
            max_tx = t_tx;
        }
        n_valid++;
    }

    fix.num_used = static_cast<uint8_t>(n_valid);
    if (n_valid < 4) {
        fix.reject_reason = 1;
        return fix;
    }

    // Reference nominal receiver time: 70 ms after latest transmit time (~21,000 km range)
    const double t_rx_est = max_tx + 0.070;
    for (size_t i = 0; i < n_valid; i++) {
        const double dt = wrap_week_sec(t_rx_est - t_tx_arr[i]);
        // Reject wild outliers (> 0.25s range difference)
        if (dt < 0.010 || dt > 0.250) {
            fix.reject_reason = 2;
            return fix;
        }
        rho[i] = kGpsC * (dt + dt_sv_arr[i]);
    }

    // 2. Gauss-Newton iterative 4x4 least squares (x, y, z, b)
    double ux = 0.0, uy = 0.0, uz = 0.0, ub = 0.0;
    double dr[MAX_NAV_CH];
    double HtH[4][4];
    double HtH_inv[4][4];
    bool solved = false;

    for (int iter = 0; iter < 10; iter++) {
        std::memset(HtH, 0, sizeof(HtH));
        double Htdr[4] = {0.0, 0.0, 0.0, 0.0};

        for (size_t i = 0; i < n_valid; i++) {
            // Sagnac Earth-rotation correction during signal flight time tau
            double tau = 0.070;
            if (iter > 0) {
                const double dx0 = sv_x[i] - ux;
                const double dy0 = sv_y[i] - uy;
                const double dz0 = sv_z[i] - uz;
                tau = sqrt(dx0 * dx0 + dy0 * dy0 + dz0 * dz0) / kGpsC;
            }
            const double ang = kGpsOmegaE * tau;
            const double ca = cos(ang);
            const double sa = sin(ang);
            const double sx = sv_x[i] * ca + sv_y[i] * sa;
            const double sy = -sv_x[i] * sa + sv_y[i] * ca;
            const double sz = sv_z[i];

            const double dx = sx - ux;
            const double dy = sy - uy;
            const double dz = sz - uz;
            const double range = sqrt(dx * dx + dy * dy + dz * dz);
            if (range < 1e3) return fix;

            const double hi[4] = {-dx / range, -dy / range, -dz / range, 1.0};
            dr[i] = rho[i] - (range + ub);

            for (int r = 0; r < 4; r++) {
                Htdr[r] += hi[r] * dr[i];
                for (int c = 0; c < 4; c++) {
                    HtH[r][c] += hi[r] * hi[c];
                }
            }
        }

        if (!invert_4x4(HtH, HtH_inv)) {
            fix.reject_reason = 3;
            return fix;
        }

        double step[4] = {0.0, 0.0, 0.0, 0.0};
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                step[r] += HtH_inv[r][c] * Htdr[c];
            }
        }

        ux += step[0];
        uy += step[1];
        uz += step[2];
        ub += step[3];

        const double step_norm = sqrt(step[0] * step[0] + step[1] * step[1] + step[2] * step[2]);
        if (step_norm < 1e-3 && iter >= 2) {
            solved = true;
            break;
        }
    }

    if (!solved) {
        fix.reject_reason = 4;
        return fix;
    }

    // 3. Residuals, PDOP, and Geodetic conversion
    double res_sq = 0.0;
    for (size_t i = 0; i < n_valid; i++) {
        res_sq += dr[i] * dr[i];
    }
    fix.rms_res_m = sqrt(res_sq / static_cast<double>(n_valid));

    const double trace_xyz = HtH_inv[0][0] + HtH_inv[1][1] + HtH_inv[2][2];
    fix.pdop = (trace_xyz > 0.0) ? sqrt(trace_xyz) : 99.9;

    double lat_deg = 0.0, lon_deg = 0.0, alt_m = 0.0;
    ecef_to_geodetic(ux, uy, uz, lat_deg, lon_deg, alt_m);

    // Compute local ENU HDOP from ECEF covariance
    const double lat_r = lat_deg * (kGpsPi / 180.0);
    const double lon_r = lon_deg * (kGpsPi / 180.0);
    const double sl = sin(lat_r), cl = cos(lat_r);
    const double so = sin(lon_r), co = cos(lon_r);
    // East unit vector: [-so, co, 0]; North unit vector: [-sl*co, -sl*so, cl]
    const double e_vec[3] = {-so, co, 0.0};
    const double n_vec[3] = {-sl * co, -sl * so, cl};
    double var_e = 0.0, var_n = 0.0;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            var_e += e_vec[r] * HtH_inv[r][c] * e_vec[c];
            var_n += n_vec[r] * HtH_inv[r][c] * n_vec[c];
        }
    }
    fix.hdop = ((var_e + var_n) > 0.0) ? sqrt(var_e + var_n) : 99.9;

    fix.lat_deg = lat_deg;
    fix.lon_deg = lon_deg;
    fix.alt_m = alt_m;
    fix.clk_bias_m = ub;

    // Corrected GPS receiver time of week
    double true_tow = t_rx_est - ub / kGpsC;
    while (true_tow < 0.0) true_tow += 604800.0;
    while (true_tow >= 604800.0) true_tow -= 604800.0;
    fix.gps_tow_sec = static_cast<uint32_t>(floor(true_tow + 0.5)) % 604800U;

    // Sanity check: on Earth's surface (-500m .. +20000m), reasonable DOP and residual
    if (alt_m > -500.0 && alt_m < 20000.0 && fix.pdop < 30.0 && fix.rms_res_m < 500.0) {
        fix.valid = true;
        fix.reject_reason = 0;
    } else {
        fix.reject_reason = 5;
    }

    return fix;
}

} /* namespace gps_scan */

#endif /* __GPS_PVT_H__ */
