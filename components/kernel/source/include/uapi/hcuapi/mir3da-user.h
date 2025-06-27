#ifndef __HCUAPI_MIR3DA_H__
#define __HCUAPI_MIR3DA_H__
 
#include <hcuapi/iocbase.h>

struct mir3da_pos {
    short x;
    short y;
    short z;
};

#define MIR3DA_ACC_IOCTL_SET_DELAY      _IOW(MIR3DA_ACC_IOCBASE, 0, int)
#define MIR3DA_ACC_IOCTL_GET_DELAY      _IOR(MIR3DA_ACC_IOCBASE, 1, int)
#define MIR3DA_ACC_IOCTL_SET_ENABLE     _IOW(MIR3DA_ACC_IOCBASE, 2, char)
#define MIR3DA_ACC_IOCTL_GET_ENABLE     _IOR(MIR3DA_ACC_IOCBASE, 3, char)
#define MIR3DA_ACC_IOCTL_GET_COOR_XYZ   _IOW(MIR3DA_ACC_IOCBASE, 4, struct mir3da_pos)

#endif /* !__HCUAPI_MIR3DA_H__ */
