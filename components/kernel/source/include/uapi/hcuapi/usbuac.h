#ifndef __HCUAPI_USBUAC_H__
#define __HCUAPI_USBUAC_H__

#include <hcuapi/iocbase.h>
#include <hcuapi/common.h>

struct usbuac_config {
    uint8_t channels;
    uint32_t sample_rate;
    uint32_t bits_per_coded_sample;
};

#define USBUAC_MIC_START		_IO  (USBUAC_IOCBASE, 0)
#define USBUAC_MIC_STOP			_IO  (USBUAC_IOCBASE, 1)
#define USBUAC_MIC_SET_CONFIG		_IOW (USBUAC_IOCBASE, 2, struct usbuac_config)
#define USBUAC_MIC_GET_CONFIG		_IOR (USBUAC_IOCBASE, 3, struct usbuac_config)

#define USBUAC_SPEAKER_START		_IO  (USBUAC_IOCBASE, 4)
#define USBUAC_SPEAKER_STOP		_IO  (USBUAC_IOCBASE, 5)
#define USBUAC_SPEAKER_SET_CONFIG	_IOW (USBUAC_IOCBASE, 6, struct usbuac_config)
#define USBUAC_SPEAKER_GET_CONFIG	_IOR (USBUAC_IOCBASE, 7, struct usbuac_config)

#endif /* __HCUAPI_LIBUAC_H__ */
