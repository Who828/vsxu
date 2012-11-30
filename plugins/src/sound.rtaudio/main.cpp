#include "vsx_sound_input_rtaudio_module.h"

#ifndef _WIN32
#define __declspec(a)
#endif

extern "C" {
    __declspec(dllexport) vsx_module* create_new_module(unsigned long module);
    __declspec(dllexport) void destroy_module(vsx_module* m,unsigned long module);
    __declspec(dllexport) unsigned long get_num_modules();
}


vsx_module* create_new_module(unsigned long module) {
    switch (module) {
        case 0: return (vsx_module*)(new vsx_sound_input_rtaudio_module);
    }
    return 0;
}

void destroy_module(vsx_module* m,unsigned long module) {
    switch(module) {
        case 0: 
            return delete (vsx_sound_input_rtaudio_module*)m;
    }
}

unsigned long get_num_modules() {
    return 1;
}


