#ifndef VSX_SAMPLE_MODULE_H
#define VSX_SAMPLE_MODULE_H

#include "vsx_math_3d.h"
#include "vsx_param.h"
#include "vsx_module.h"
#include "vsx_float_array.h"

#include "rtaudio/rtaudio.h"
#include "rtaudio/rterror.h"
#include "fftreal/fftreal.h"

typedef struct
{
    float l_mul;
    vsx_float_array spectrum[2];
    vsx_float_array wave[2];    // 512 L 512 R BANZAI!
    float vu[2];
} vsx_paudio_struct;


void normalize_fft(float* fft, vsx_float_array& spectrum);

int record(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
               double streamTime, RtAudioStreamStatus status, void *ptr);
void worker(void *ptr);

class vsx_sound_input_rtaudio_module : public vsx_module 
{
    // in 
    vsx_module_param_int* quality;
    // out
    vsx_module_param_float* multiplier;
    float old_mult;
    vsx_paudio_struct pa_audio_data;
    float fft[512];

    vsx_module_param_float* vu_l_p;
    vsx_module_param_float* vu_r_p;

    vsx_float_array spectrum;
    vsx_module_param_float_array* spectrum_p;

    public:

    vsx_module_param_float_array* wave_p;
    vsx_float_array wave;

    virtual void module_info(vsx_module_info* info);
    virtual void declare_params(vsx_module_param_list& in_parameters, vsx_module_param_list& out_parameters);
    virtual void run();
    virtual bool init();
    virtual void on_delete();
};

#endif 

