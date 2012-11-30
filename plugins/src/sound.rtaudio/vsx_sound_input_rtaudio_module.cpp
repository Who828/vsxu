#include "vsx_sound_input_rtaudio_module.h"

void normalize_fft(float* fft, vsx_float_array& spectrum){
    float B1 = pow(8.0,1.0/512.0); //1.004                                                                                                                                             
    float dd = 1*(512.0/8.0);
    float a = 0;
    float b;
    float diff = 0;
    int aa;
    for (int i = 1; i < 512; ++i) {
        (*(spectrum.data))[i] = 0;
        b = (float)((pow(B1,(float)(i))-1.0)*dd);
        diff = b-a;
        aa = (int)floor(a);

        if((int)b == (int)a) {
            (*(spectrum.data))[i] = fft[aa]*3 * diff;
        }
        else
        {
            ++aa;
            (*(spectrum.data))[i] += fft[aa]*3 * (ceil(a) - a);
            while (aa != (int)b) {
                (*(spectrum.data))[i] += fft[aa]*3;
                ++aa;
            }
            (*(spectrum.data))[i] += fft[aa+1]*3 * (b - floor(b));
        }
        a = b;
    }
}

void vsx_sound_input_rtaudio_module::module_info(vsx_module_info* info)
{
    info->output = 1;
    info->identifier = "sound;input_visualization_listener||system;sound;vsx_listener";
#ifndef VSX_NO_CLIENT
    info->description = "Simple fft runs at 86.13 fps\n\
                         HQ fft runs at 43.07 fps\n\
                         The octaves are 0 = bass, 7 = treble";
    info->in_param_spec = "\
                           quality:enum?\
                           normal_only\
                           `\
                           ,multiplier:float\
                           ";
    info->out_param_spec = "\
                            vu:complex{\
                                vu_l:float,\
                                    vu_r:float\
                            },\
wave:float_array,\                                                                                                                                                                   normal:complex{spectrum:float_array}";
     info->component_class = "output";
#endif
}

void vsx_sound_input_rtaudio_module::declare_params(vsx_module_param_list& in_parameters, vsx_module_param_list& out_parameters)
{

    quality = (vsx_module_param_int*)in_parameters.create(VSX_MODULE_PARAM_ID_INT,"quality");
    quality->set(0);

    multiplier = (vsx_module_param_float*)in_parameters.create(VSX_MODULE_PARAM_ID_FLOAT,"multiplier");
    multiplier->set(1);

    vu_l_p = (vsx_module_param_float*)out_parameters.create(VSX_MODULE_PARAM_ID_FLOAT,"vu_l");
    vu_r_p = (vsx_module_param_float*)out_parameters.create(VSX_MODULE_PARAM_ID_FLOAT,"vu_r");
    vu_l_p->set(0);
    vu_r_p->set(0);

    wave_p = (vsx_module_param_float_array*)out_parameters.create(VSX_MODULE_PARAM_ID_FLOAT_ARRAY,"wave");
    wave.data = new vsx_array<float>;
    for (int i = 0; i < 512; ++i) wave.data->push_back(0);
    wave_p->set_p(wave);

    spectrum_p = (vsx_module_param_float_array*)out_parameters.create(VSX_MODULE_PARAM_ID_FLOAT_ARRAY,"spectrum");
    spectrum.data = new vsx_array<float>;
    for (int i = 0; i < 512; ++i) spectrum.data->push_back(0);
    spectrum_p->set_p(spectrum);

    loading_done = true;
}

bool vsx_sound_input_rtaudio_module::init()
{
    return true;
}

void vsx_sound_input_rtaudio_module::on_delete()
{
    delete spectrum.data;
}

void vsx_sound_input_rtaudio_module::run()
{
    worker((void*) &pa_audio_data);
    pa_audio_data.l_mul = multiplier->get()*engine->amp;
    // set wave
    if (0 == engine->param_float_arrays.size())
    {
        wave_p->set_p(pa_audio_data.wave[0]);
    }

    spectrum_p->set_p(pa_audio_data.spectrum[0]);
    vu_l_p->set(pa_audio_data.vu[0]);
    vu_r_p->set(pa_audio_data.vu[1]);
}

int record(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
        double streamTime, RtAudioStreamStatus status, void *ptr)
{
    int16_t buf[1024];
    float fftbuf[1024];
    size_t fftbuf_it = 0;

    FFTReal* fft = new FFTReal(512);
    vsx_paudio_struct* pa_d = (vsx_paudio_struct*)ptr;

    pa_d->wave[0].data = new vsx_array<float>;
    pa_d->wave[1].data = new vsx_array<float>;
    for (int i = 0; i < 512; ++i) pa_d->wave[0].data->push_back(0);
    for (int i = 0; i < 512; ++i) pa_d->wave[1].data->push_back(0);

    pa_d->spectrum[0].data = new vsx_array<float>;
    for (int i = 0; i < 512; ++i) pa_d->spectrum[0].data->push_back(0);

    for (;;)
    {
        int j = 0;
        // nab left channel for spectrum data
        for (size_t i = 0; i < 512; i++)
        {
            const float &f = (float)buf[j] / 16384.0f;
            (*(pa_d->wave[0].data))[i] = f * pa_d->l_mul;
            fftbuf[fftbuf_it++] = f;
            j++;
            j++;
        }
        fftbuf_it = fftbuf_it % 1024;
        j = 1;
        for (size_t i = 0; i < 512; i++)
        {
            (*(pa_d->wave[1].data))[i] = (float)buf[j] / 16384.0f * pa_d->l_mul;
            j++;
            j++;
        }
        // do some FFT's
        float spectrum[1024];
        float spectrum_dest[512];
        fft->do_fft( (float*)&spectrum, (float*) &fftbuf[0]);
        float re, im;

        for(int ii = 0; ii < 256; ii++)
        {
            re = spectrum[ii];
            im = spectrum[ii + 256];
            spectrum_dest[ii] = (float)sqrt(re * re + im * im) / 256.0f * pa_d->l_mul;
        }

        // calc vu
        float vu = 0.0f;
        for (int ii = 0; ii < 256; ii++)
        {
            vu += spectrum_dest[ii];
        }
        pa_d->vu[0] = vu;
        pa_d->vu[1] = vu;

        for (size_t ii = 0; ii < 512; ii++)
        {
            (*(pa_d->spectrum[0].data))[ii] = spectrum_dest[ii >> 1] * 3.0f * pow(log( 10.0f + 44100.0f * (ii / 512.0f)) ,1.0f);
        }

    }

    return 0;
}

void worker(void *ptr)
{
    RtAudio audio;

    if (audio.getDeviceCount() < 1) {
        exit(0);
    }

    RtAudio::StreamParameters paramters;
    paramters.deviceId = audio.getDefaultInputDevice();
    paramters.nChannels = 2;
    paramters.firstChannel = 0;
    unsigned int sampleRate = 44100;
    unsigned int bufferSize = 256;

    try {
        audio.openStream(NULL, &paramters, RTAUDIO_SINT16, sampleRate, &bufferSize, &record, ptr);
        audio.startStream();
    }

    catch (RtError &error) {
        error.printMessage();
        exit(0);
    }

    try {
        audio.stopStream();
    }
    catch (RtError &error) {
        error.printMessage();
    }

    if (audio.isStreamOpen())
        audio.closeStream();

}
