/* 主机端测试：验证 FOC 数学库。gcc 编译，失败时返回非零。 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "foc_math.h"
#include "foc_pi.h"

static int fails = 0;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static void test_sincos(void)
{
    int maxerr = 0;
    for (uint32_t a = 0; a < 65536; a += 7) {
        foc_trig_t t = foc_sincos((uint16_t)a);
        double th = 2.0 * M_PI * a / 65536.0;
        int es = abs((int)lround(sin(th) * 32767) - t.sin);
        int ec = abs((int)lround(cos(th) * 32767) - t.cos);
        if (es > maxerr) maxerr = es;
        if (ec > maxerr) maxerr = ec;
    }
    printf("sincos max error = %d LSB (%.4f%%)\n", maxerr, maxerr / 327.67);
    CHECK(maxerr < 40, "sincos error too large: %d", maxerr);
}

static void test_clarke_park_roundtrip(void)
{
    /* 三相对称电流 → Clarke → Park(同角度) 应得到恒定 d,q */
    double amp = 20000.0;
    int maxdev = 0;
    for (int k = 0; k < 360; k += 3) {
        double th = k * M_PI / 180.0;
        q15_t ia = (q15_t)lround(amp * cos(th));
        q15_t ib = (q15_t)lround(amp * cos(th - 2.0 * M_PI / 3.0));
        foc_ab_t ab = foc_clarke(ia, ib);
        foc_trig_t t = foc_sincos((uint16_t)lround(th / (2 * M_PI) * 65536.0) & 0xFFFF);
        foc_dq_t dq = foc_park(ab, t);
        /* 预期：d = amp, q ≈ 0 */
        int dd = abs(dq.d - (int)amp), dqv = abs(dq.q);
        if (dd > maxdev) maxdev = dd;
        if (dqv > maxdev) maxdev = dqv;
    }
    printf("Clarke+Park max deviation = %d LSB\n", maxdev);
    CHECK(maxdev < 250, "Clarke/Park deviation %d", maxdev);
}

static void test_inv_park_roundtrip(void)
{
    int maxerr = 0;
    for (uint32_t a = 0; a < 65536; a += 211) {
        foc_trig_t t = foc_sincos((uint16_t)a);
        foc_dq_t in = { 12000, -7000 };
        foc_ab_t ab = foc_inv_park(in, t);
        foc_dq_t out = foc_park(ab, t);
        int e = abs(out.d - in.d); if (abs(out.q - in.q) > e) e = abs(out.q - in.q);
        if (e > maxerr) maxerr = e;
    }
    printf("InvPark->Park roundtrip max error = %d LSB\n", maxerr);
    CHECK(maxerr < 80, "park roundtrip error %d", maxerr);
}

static void test_svpwm(void)
{
    const uint16_t period = 1799;
    /* 1) 零矢量 → 三相都 50% */
    uint16_t c[3];
    foc_ab_t z = { 0, 0 };
    foc_svpwm(z, period, c);
    CHECK(abs((int)c[0] - period / 2) <= 1 && abs((int)c[1] - period / 2) <= 1 && abs((int)c[2] - period / 2) <= 1,
          "zero vector duty %u %u %u", c[0], c[1], c[2]);

    /* 2) 线性极限幅值转一圈：占空比必须在 [0,period]，相间电压差应符合 α/β 期望 */
    int maxerr = 0;
    for (uint32_t a = 0; a < 65536; a += 97) {
        foc_trig_t t = foc_sincos((uint16_t)a);
        foc_dq_t v = { 0, FOC_VMAX_LINEAR };
        foc_ab_t ab = foc_inv_park(v, t);
        foc_svpwm(ab, period, c);
        CHECK(c[0] <= period && c[1] <= period && c[2] <= period, "duty out of range");

        /* 线电压（占空比差）应等于 反Clarke 相电压之差，与零序注入无关 */
        double va = ab.alpha;
        double vb = -0.5 * ab.alpha + 0.8660254 * ab.beta;
        double vc = -0.5 * ab.alpha - 0.8660254 * ab.beta;
        double e_ab = ((va - vb) / 32768.0) * period - ((double)c[0] - (double)c[1]);
        double e_bc = ((vb - vc) / 32768.0) * period - ((double)c[1] - (double)c[2]);
        int e = (int)ceil(fmax(fabs(e_ab), fabs(e_bc)));
        if (e > maxerr) maxerr = e;
    }
    printf("SVPWM line-voltage error max = %d counts (of %u)\n", maxerr, period);
    CHECK(maxerr <= 3, "svpwm line voltage error %d", maxerr);
}

static void test_limit_vector(void)
{
    foc_dq_t v = { 20000, 20000 };
    foc_dq_t o = foc_limit_vector(v, 10000);
    double mag = sqrt((double)o.d * o.d + (double)o.q * o.q);
    printf("limit_vector magnitude = %.1f (target 10000)\n", mag);
    CHECK(fabs(mag - 10000.0) < 5.0, "limit magnitude %.1f", mag);
    CHECK(abs(o.d - o.q) <= 1, "direction changed");

    foc_dq_t small = { 100, -200 };
    foc_dq_t o2 = foc_limit_vector(small, 10000);
    CHECK(o2.d == 100 && o2.q == -200, "small vector should be unchanged");
}

static void test_pi(void)
{
    /* 一阶惯性对象 y' = (u - y)/tau，PI 应无静差地跟上阶跃 */
    foc_pi_t pi;
    foc_pi_init(&pi, 2048 /*kp=2.0*/, 64 /*ki≈0.0625*/, -32767, 32767);
    double y = 0.0, tau = 20.0;
    q15_t ref = 10000;
    for (int i = 0; i < 2000; i++) {
        q15_t u = foc_pi_run(&pi, ref, (q15_t)lround(y));
        y += ((double)u - y) / tau;
    }
    printf("PI steady-state y = %.1f (ref %d)\n", y, ref);
    CHECK(fabs(y - ref) < 20.0, "PI steady state error %.1f", y - ref);

    /* 抗饱和：给超大阶跃再撤销，积分不能卡死 */
    foc_pi_reset(&pi);
    for (int i = 0; i < 5000; i++) foc_pi_run(&pi, 32767, 0);
    q15_t out = 0;
    for (int i = 0; i < 200; i++) out = foc_pi_run(&pi, 0, 5000);
    CHECK(out < 0, "anti-windup: output should unwind, got %d", out);
}

int main(void)
{
    test_sincos();
    test_clarke_park_roundtrip();
    test_inv_park_roundtrip();
    test_svpwm();
    test_limit_vector();
    test_pi();
    if (fails) { printf("\n%d test(s) FAILED\n", fails); return 1; }
    printf("\nAll tests passed.\n");
    return 0;
}
