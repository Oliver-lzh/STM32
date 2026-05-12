/*
 * dss_pca_vitalsigns.h
 */

#ifndef DSS_PCA_VITALSIGNS_H_
#define DSS_PCA_VITALSIGNS_H_

#ifdef __cplusplus
extern "C" {
#endif

void PCA_VitalSigns_reset(void);
int  PCA_VitalSigns_process(float I_in, float Q_in, float *I_out, float *Q_out);

/* Optional helper filters for GUI/debug use */
void HeartSignal_reset(void);
float HeartSignal_process(float unwrapped_phase);
void BreathSignal_reset(void);
float BreathSignal_process(float unwrapped_phase);

#ifdef __cplusplus
}
#endif

#endif /* DSS_PCA_VITALSIGNS_H_ */
