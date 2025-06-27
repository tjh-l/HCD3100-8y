#ifndef __LINIX_IO_H__
#define __LINIX_IO_H__

#ifdef __linux__

#include <stdint.h> //uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BIT31       0x80000000
#define BIT30       0x40000000
#define BIT29       0x20000000
#define BIT28       0x10000000
#define BIT27       0x08000000
#define BIT26       0x04000000
#define BIT25       0x02000000
#define BIT24       0x01000000
#define BIT23       0x00800000
#define BIT22       0x00400000
#define BIT21       0x00200000
#define BIT20       0x00100000
#define BIT19       0x00080000
#define BIT18       0x00040000
#define BIT17       0x00020000
#define BIT16       0x00010000
#define BIT15       0x00008000
#define BIT14       0x00004000
#define BIT13       0x00002000
#define BIT12       0x00001000
#define BIT11       0x00000800
#define BIT10       0x00000400
#define BIT9        0x00000200
#define BIT8        0x00000100
#define BIT7        0x00000080
#define BIT6        0x00000040
#define BIT5        0x00000020
#define BIT4        0x00000010
#define BIT3        0x00000008
#define BIT2        0x00000004
#define BIT1        0x00000002
#define BIT0        0x00000001

#define LINUX_ADDR(addr)    (addr& 0x1fffffff)

#define PAGE_SIZE 4096
#define PAGE_ADDR_ALIGN(addr)  (addr & ~(uint32_t)(PAGE_SIZE - 1))
#define PAGE_ADDR_OFFSET(addr)  (addr & (PAGE_SIZE - 1))


//#ifndef __ASSEMBLER__
#if 1
#define BIT(nr)				(1UL << (nr))
#else
#define BIT(nr)				(1 << (nr))
#endif

//#ifndef __ASSEMBLER__
#if 1
#define NBITS_V(n)			((1UL << (n)) - 1)
#else
#define NBITS_V(n)			((1 << (n)) - 1)
#endif

#define NBITS_M(s, n)			(NBITS_V(n) << s)


static inline uint32_t linux_REG_READ(uint32_t addr, int bits)
{
    uint32_t reg_val = 0;
    uint32_t addr_align = 0;
    uint32_t addr_offset = 0;
    uint32_t linux_addr = LINUX_ADDR(addr);
    void *map_addr = 0;
    void *virt_addr = 0;

    if (bits != 8 && bits != 16 && bits != 32)
        return 0;

    addr_align = PAGE_ADDR_ALIGN(linux_addr);
    addr_offset = PAGE_ADDR_OFFSET(linux_addr);

    int fd =  open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
        return 0;

    map_addr = (void*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr_align);
    if (NULL == map_addr || 0xFFFFFFFF == (uint32_t)map_addr)
        return 0;
    virt_addr = (char*)map_addr + addr_offset;

    switch (bits){
    case 8:
        reg_val = *(volatile uint8_t*)virt_addr;
        break;
    case 16:
        reg_val = *(volatile uint16_t*)virt_addr;
        break;
    case 32:
        reg_val = *(volatile uint32_t*)virt_addr;
        break;
    default:
        reg_val = *(volatile uint32_t*)virt_addr;
        break;
    }

    munmap(map_addr, PAGE_SIZE);
    close(fd);

    return reg_val; 
}

static inline void linux_REG_WRITE(uint32_t addr, int bits, const uint32_t value)
{
    uint32_t addr_align = 0;
    uint32_t addr_offset = 0;
    uint32_t linux_addr = LINUX_ADDR(addr);
    void *map_addr = 0;
    void *virt_addr = 0;

    if (bits != 8 && bits != 16 && bits != 32)
        return;

    addr_align = PAGE_ADDR_ALIGN(linux_addr);
    addr_offset = PAGE_ADDR_OFFSET(linux_addr);

    int fd =  open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
        return;

    map_addr = (void*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr_align);
    if (NULL == map_addr || 0xFFFFFFFF == (uint32_t)map_addr)
        return;
    virt_addr = (char*)map_addr + addr_offset;

    switch (bits){
    case 8:
        *(volatile uint8_t*)virt_addr = value;
        break;
    case 16:
        *(volatile uint16_t*)virt_addr = value;
        break;
    case 32:
        *(volatile uint32_t*)virt_addr = value;
        break;
    default:
        *(volatile uint32_t*)virt_addr = value;
        break;
    }

    munmap(map_addr, PAGE_SIZE);
    close(fd);

}


#define REG8_WRITE(_r, _v) ({                                                                                           \
            linux_REG_WRITE(_r, 8, (uint32_t)_v);                                                                       \
        })

#define REG8_READ(_r) ({                                                                                                \
            (uint8_t)linux_REG_READ(_r, 8);                                                                              \
        })

#define REG16_WRITE(_r, _v) ({                                                                                           \
            linux_REG_WRITE(_r, 16, (uint32_t)_v);                                                                       \
        })


#define REG16_READ(_r) ({                                                                                                \
            (uint16_t)linux_REG_READ(_r, 16);                                                                              \
        })


#define REG32_WRITE(_r, _v) ({                                                                                           \
            linux_REG_WRITE(_r, 32, (uint32_t)_v);                                                                       \
        })


#define REG32_READ(_r) ({                                                                                                \
            (uint32_t)linux_REG_READ(_r, 32);                                                                              \
        })


//get bit or get bits from register
#define REG32_GET_BIT(_r, _b)  ({                                                                                        \
            (REG32_READ(_r) & (_b));                                                                        \
        })

#define REG16_GET_BIT(_r, _b)  ({                                                                                        \
            (REG16_READ(_r) & (_b));                                                                        \
        })

#define REG8_GET_BIT(_r, _b)  ({                                                                                        \
            (REG8_READ(_r) & (_b))                                                                        \
        })

//set bit or set bits to register
#define REG32_SET_BIT(_r, _b)  ({                                                                                        \
            (REG32_WRITE(_r, REG32_READ(_r) | (_b)));                                                                       \
        })

#define REG16_SET_BIT(_r, _b)  ({                                                                                        \
            (REG16_WRITE(_r, REG16_READ(_r) | (_b)));                                                                       \
        })

#define REG8_SET_BIT(_r, _b)  ({                                                                                        \
            (REG8_WRITE(_r, REG8_READ(_r) | (_b)));                                                                       \
        })

//clear bit or clear bits of register
#define REG32_CLR_BIT(_r, _b)  ({                                                                                        \
            (REG32_WRITE(_r, REG32_READ(_r) & (~(_b))));                                                                       \
        })

#define REG16_CLR_BIT(_r, _b)  ({                                                                                        \
            (REG16_WRITE(_r, REG16_READ(_r) & (~(_b))));                                                                       \
        })

#define REG8_CLR_BIT(_r, _b)  ({                                                                                        \
            (REG8_WRITE(_r, REG8_READ(_r) & (~(_b))));                                                                       \
        })

//set bits of register controlled by mask
#define REG32_SET_BITS(_r, _b, _m) ({                                                                                    \
            (REG32_WRITE(_r, (REG32_READ(_r) & (~(_m))) | ((_b) & (_m))));                                                \
        })

#define REG16_SET_BITS(_r, _b, _m) ({                                                                                    \
            (REG16_WRITE(_r, (REG16_READ(_r) & (~(_m))) | ((_b) & (_m))));                                                \
        })

#define REG8_SET_BITS(_r, _b, _m) ({                                                                                    \
            (REG8_WRITE(_r, (REG8_READ(_r) & (~(_m))) | ((_b) & (_m))));                                                \
        })

//get field from register, uses field _S & _V to determine mask
#define REG32_GET_FIELD(_r, _f) ({                                                                                       \
            ((REG32_READ(_r) >> (_f##_S)) & (_f##_V));                                                                   \
        })

#define REG16_GET_FIELD(_r, _f) ({                                                                                       \
            ((REG16_READ(_r) >> (_f##_S)) & (_f##_V));                                                                   \
        })

#define REG8_GET_FIELD(_r, _f) ({                                                                                       \
            ((REG8_READ(_r) >> (_f##_S)) & (_f##_V));                                                                   \
        })

//set field of a register from variable, uses field _S & _V to determine mask
#define REG32_SET_FIELD(_r, _f, _v) ({                                                                                   \
            (REG32_WRITE((_r),((REG32_READ(_r) & ~(_f))|(((_v) & (_f##_V))<<(_f##_S)))));                \
        })

#define REG16_SET_FIELD(_r, _f, _v) ({                                                                                   \
            (REG16_WRITE((_r),((REG16_READ(_r) & ~(_f))|(((_v) & (_f##_V))<<(_f##_S)))));                \
        })

#define REG8_SET_FIELD(_r, _f, _v) ({                                                                                   \
            (REG8_WRITE((_r),((REG8_READ(_r) & ~(_f))|(((_v) & (_f##_V))<<(_f##_S)))));                \
        })


#define REG32_SET_FIELD2(_r, _shift, _nbits, _v) ({ \
        (REG32_WRITE((_r), ((REG32_READ(_r) & ~(NBITS_M((_shift), (_nbits)))) | (((_v) & (NBITS_V((_nbits))))<<(_shift))))); \
    })

#define REG16_SET_FIELD2(_r, _shift, _nbits, _v) ({ \
        (REG16_WRITE((_r), ((REG16_READ(_r) & ~(NBITS_M((_shift), (_nbits)))) | (((_v) & (NBITS_V((_nbits))))<<(_shift))))); \
    })

#define REG8_SET_FIELD2(_r, _shift, _nbits, _v) ({ \
        (REG8_WRITE((_r), ((REG8_READ(_r) & ~(NBITS_M((_shift), (_nbits)))) | (((_v) & (NBITS_V((_nbits))))<<(_shift))))); \
    })

#define REG32_GET_FIELD2(_r, _shift, _nbits) ({ \
        ((REG32_READ(_r) & (NBITS_M((_shift), (_nbits)))) >> (_shift)); \
    })

#define REG16_GET_FIELD2(_r, _shift, _nbits) ({ \
        ((REG16_READ(_r) & (NBITS_M((_shift), (_nbits)))) >> (_shift)); \
    })

#define REG8_GET_FIELD2(_r, _shift, _nbits) ({ \
        ((REG8_READ(_r) & (NBITS_M((_shift), (_nbits)))) >> (_shift)); \
    })



#endif //end of __linux__


#endif //end of __LINIX_IO_H__