#ifndef __HUDI_ADEC_INTER_H__
#define __HUDI_ADEC_INTER_H__

#include <hudi_snd.h>
#include <hudi_adec.h>

#define HUDI_ADEC_DEV       "/dev/auddec"
#define HUDI_SND_I2SO_DEV   "/dev/sndC0i2so"
#define HUDI_SND_I2SI_DEV   "/dev/sndC0i2si"
#define HUDI_SND_SPO_DEV    "/dev/sndC0spo"
#define HUDI_SND_KSHM       "/dev/kshmdev"

typedef struct
{
    hudi_adec_cb notifier;
    int inited;
    int fd;
} hudi_adec_instance_t;

typedef struct
{
    hudi_snd_cb notifier;
    int inited;
    int started;
    int fd_i2so;
    int fd_spo;
    int i2so_feeded;
    int fd_kshm;
    struct kshm_info aud_hdl;
} hudi_snd_instance_t;

typedef struct
{
    int inited;
    int started;
    int fd_i2si;
} hudi_snd_i2si_instance_t;

#endif
