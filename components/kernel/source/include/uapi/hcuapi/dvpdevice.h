#ifndef __HCUAPI_DVPDEVICE_H_
#define __HCUAPI_DVPDEVICE_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/tvtype.h>

#ifndef __KERNEL__
#include <stdbool.h>
#endif

typedef struct dvp_device_tvtype {
	tvtype_e tv_sys;
	bool b_progressive;
} dvp_device_tvtype_t;

typedef enum DVP_ENHANCE_TYPE {
	DVP_ENHANCE_BRIGHTNESS,
	DVP_ENHANCE_CONTRAST,
	DVP_ENHANCE_SATURATION,
	DVP_ENHANCE_HUE,
} dvp_enhance_type_e;

typedef struct dvp_enhance {
	dvp_enhance_type_e type;
	int value;
} dvp_enhance_t;

#define DVPDEVICE_SET_INPUT_TVTYPE		_IOW (DVPDEVIDE_IOCBASE, 0, struct dvp_device_tvtype)
#define DVPDEVICE_GET_INPUT_DETECT_SUPPORT	_IOR (DVPDEVIDE_IOCBASE, 1, uint32_t)
#define DVPDEVICE_GET_INPUT_TVTYPE		_IOR (DVPDEVIDE_IOCBASE, 2, struct dvp_device_tvtype)  
#define DVPDEVICE_SET_INPUT_PORT		_IO  (DVPDEVIDE_IOCBASE, 3)	//!< param: port number
#define DVPDEVICE_SET_ENHANCE			_IOW (DVPDEVIDE_IOCBASE, 4, struct dvp_enhance)

#endif
