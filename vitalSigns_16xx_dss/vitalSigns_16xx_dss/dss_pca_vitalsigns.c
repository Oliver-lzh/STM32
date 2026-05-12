/*
 * dss_pca_vitalsigns.c
 *
 * PCA is used only to rotate/correct IQ trajectory.
 * Do not use PCA projection directly as phase.
 */

#include "dss_pca_vitalsigns.h"
#include <math.h>
#include <string.h>

#define PCA_BUFFER_SIZE        128
#define PCA_EPS                1.0e-6f
#define PCA_VAR_THRESHOLD      1.0e-3f

static float I_buffer[PCA_BUFFER_SIZE];
static float Q_buffer[PCA_BUFFER_SIZE];
static int   buffer_index = 0;
static int   buffer_full = 0;

/* Principal axis */
static float pca_v1 = 1.0f;
static float pca_v2 = 0.0f;
static int   pca_ready = 0;

void PCA_VitalSigns_reset(void)
{
    memset(I_buffer, 0, sizeof(I_buffer));
    memset(Q_buffer, 0, sizeof(Q_buffer));
    buffer_index = 0;
    buffer_full  = 0;
    pca_v1 = 1.0f;
    pca_v2 = 0.0f;
    pca_ready = 0;
}

static void PCA_update_axis(void)
{
    int i;
    float meanI = 0.0f, meanQ = 0.0f;
    float C11 = 0.0f, C12 = 0.0f, C22 = 0.0f;
    float trace, det, disc;
    float lambda1;
    float v1, v2, norm;

    for (i = 0; i < PCA_BUFFER_SIZE; i++)
    {
        meanI += I_buffer[i];
        meanQ += Q_buffer[i];
    }
    meanI /= (float)PCA_BUFFER_SIZE;
    meanQ /= (float)PCA_BUFFER_SIZE;

    for (i = 0; i < PCA_BUFFER_SIZE; i++)
    {
        float Ic = I_buffer[i] - meanI;
        float Qc = Q_buffer[i] - meanQ;
        C11 += Ic * Ic;
        C12 += Ic * Qc;
        C22 += Qc * Qc;
    }

    C11 /= (float)PCA_BUFFER_SIZE;
    C12 /= (float)PCA_BUFFER_SIZE;
    C22 /= (float)PCA_BUFFER_SIZE;

    if ((C11 + C22) < PCA_VAR_THRESHOLD)
    {
        return;
    }

    trace = C11 + C22;
    det   = C11 * C22 - C12 * C12;
    disc  = trace * trace - 4.0f * det;
    if (disc < 0.0f)
    {
        disc = 0.0f;
    }

    lambda1 = 0.5f * (trace + sqrtf(disc));

    if (fabsf(C12) > PCA_EPS)
    {
        v1 = C12;
        v2 = lambda1 - C11;
    }
    else if (C11 >= C22)
    {
        v1 = 1.0f;
        v2 = 0.0f;
    }
    else
    {
        v1 = 0.0f;
        v2 = 1.0f;
    }

    norm = sqrtf(v1 * v1 + v2 * v2);
    if (norm < PCA_EPS)
    {
        return;
    }

    v1 /= norm;
    v2 /= norm;

    /* Keep axis direction continuous to avoid phase flips */
    if ((v1 * pca_v1 + v2 * pca_v2) < 0.0f)
    {
        v1 = -v1;
        v2 = -v2;
    }

    pca_v1 = v1;
    pca_v2 = v2;
    pca_ready = 1;
}

int PCA_VitalSigns_process(float I_in, float Q_in, float *I_out, float *Q_out)
{
    I_buffer[buffer_index] = I_in;
    Q_buffer[buffer_index] = Q_in;

    buffer_index++;
    if (buffer_index >= PCA_BUFFER_SIZE)
    {
        buffer_index = 0;
        buffer_full = 1;
    }

    if (buffer_full)
    {
        PCA_update_axis();
    }

    if (!pca_ready)
    {
        *I_out = I_in;
        *Q_out = Q_in;
        return 0;
    }

    *I_out =  I_in * pca_v1 + Q_in * pca_v2;
    *Q_out = -I_in * pca_v2 + Q_in * pca_v1;
    return 1;
}

/* -------------------- Optional heart/breath helper filters -------------------- */

static float hf_trend = 0.0f;
static float hf_prev_detrend = 0.0f;
static float hf_hp_prev_in = 0.0f;
static float hf_hp_prev_out = 0.0f;
static float hf_lp_out = 0.0f;
static float hf_out_smooth = 0.0f;
static float hf_breath_remove = 0.0f;

static float bf_lp_out = 0.0f;
static float bf_prev_in = 0.0f;
static float bf_hp_prev_out = 0.0f;

#define HEART_TREND_ALPHA        0.01f
#define HEART_DIFF_LIMIT         0.05f
#define HEART_HP_ALPHA           0.92f
#define HEART_LP_ALPHA           0.22f
#define HEART_SMOOTH_ALPHA       0.25f
#define BREATH_LP_ALPHA          0.08f
#define BREATH_HP_ALPHA          0.995f

static float clipf_local(float x, float lim)
{
    if (x > lim)  return lim;
    if (x < -lim) return -lim;
    return x;
}

void HeartSignal_reset(void)
{
    hf_trend = 0.0f;
    hf_prev_detrend = 0.0f;
    hf_hp_prev_in = 0.0f;
    hf_hp_prev_out = 0.0f;
    hf_lp_out = 0.0f;
    hf_out_smooth = 0.0f;
    hf_breath_remove = 0.0f;
}

float HeartSignal_process(float unwrapped_phase)
{
    float detrend;
    float diff_sig;
    float hp_out;
    float lp_out;

    hf_trend = (1.0f - HEART_TREND_ALPHA) * hf_trend + HEART_TREND_ALPHA * unwrapped_phase;
    detrend = unwrapped_phase - hf_trend;

    hf_breath_remove = 0.98f * hf_breath_remove + 0.02f * detrend;
    detrend = detrend - hf_breath_remove;

    diff_sig = detrend - hf_prev_detrend;
    hf_prev_detrend = detrend;
    diff_sig = clipf_local(diff_sig, HEART_DIFF_LIMIT);

    hp_out = HEART_HP_ALPHA * (hf_hp_prev_out + diff_sig - hf_hp_prev_in);
    hf_hp_prev_in = diff_sig;
    hf_hp_prev_out = hp_out;

    lp_out = HEART_LP_ALPHA * hp_out + (1.0f - HEART_LP_ALPHA) * hf_lp_out;
    hf_lp_out = lp_out;

    hf_out_smooth = HEART_SMOOTH_ALPHA * lp_out + (1.0f - HEART_SMOOTH_ALPHA) * hf_out_smooth;
    return 3.0f * hf_out_smooth;
}

void BreathSignal_reset(void)
{
    bf_lp_out = 0.0f;
    bf_prev_in = 0.0f;
    bf_hp_prev_out = 0.0f;
}

float BreathSignal_process(float unwrapped_phase)
{
    float hp_out;
    float lp_out;

    hp_out = BREATH_HP_ALPHA * (bf_hp_prev_out + unwrapped_phase - bf_prev_in);
    bf_prev_in = unwrapped_phase;
    bf_hp_prev_out = hp_out;

    lp_out = BREATH_LP_ALPHA * hp_out + (1.0f - BREATH_LP_ALPHA) * bf_lp_out;
    bf_lp_out = lp_out;

    return bf_lp_out;
}
