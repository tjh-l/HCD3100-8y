#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <hcuapi/sci.h>
#include <sys/stat.h>
#include <limits.h>
#include <stddef.h>
#include <sys/mman.h>
#include <pthread.h>
#ifdef __HCRTOS__
#include <kernel/delay.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <nuttx/wqueue.h>
#include <kernel/types.h>
#include <kernel/module.h>
#endif
#include "hc_test_ddr_pressure.h"
#include "boardtest_module.h"
#define __version__ "4.5.0"
#define EXIT_FAIL_NONSTARTER    0x01            //表示测试程序未能启动。
#define EXIT_FAIL_ADDRESSLINES  0x02               //表示地址线测试失败。
#define EXIT_FAIL_OTHERTEST     0x04               //表示其他测试项目失败。
#define rand32() ((unsigned int) rand() | ( (unsigned int) rand() << 16))
#define PROGRESSLEN 4
#define PROGRESSOFTEN 2500
#define ONE 0x00000001L
#define exit(...) do {} while (0)
#define mlock(...) (0)
#define munlock(...) (0)
#undef _SC_VERSION
#undef _SC_PAGE_SIZE
 
typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

static int use_phys;
static off_t physaddrbase;
static char progress[] = "-\\|/";
static int suspend;
static char error_message[128] ={0};  //出错信息
static int loop_num;
#if (ULONG_MAX == 4294967295UL)
    #define rand_ul() rand32()
    #define UL_ONEBITS 0xffffffff
    #define UL_LEN 32
    #define CHECKERBOARD1 0x55555555
    #define CHECKERBOARD2 0xaaaaaaaa
    #define UL_BYTE(x) ((x | x << 8 | x << 16 | x << 24))
#elif (ULONG_MAX == 18446744073709551615ULL)
    #define rand64() (((ul) rand32()) << 32 | ((ul) rand32()))
    #define rand_ul() rand64()
    #define UL_ONEBITS 0xffffffffffffffffUL
    #define UL_LEN 64
    #define CHECKERBOARD1 0x5555555555555555
    #define CHECKERBOARD2 0xaaaaaaaaaaaaaaaa
    #define UL_BYTE(x) (((ul)x | (ul)x<<8 | (ul)x<<16 | (ul)x<<24 | (ul)x<<32 | (ul)x<<40 | (ul)x<<48 | (ul)x<<56))
#else
    #error long on this platform is not 32 or 64 bits
#endif

struct test {
    char *name;
    int (*fp)(ulv*, ulv*, size_t);
};

union {
    unsigned char bytes[UL_LEN/8];
    ul val;
} mword8;

union {
    unsigned short u16s[UL_LEN/16];
    ul val;
} mword16;

/* Function definitions. */

static int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
    int r = 0;
    size_t i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned long physaddr;

    for (i = 0; i < count; i++, p1++, p2++) {
        if (*p1 != *p2) {
            if (use_phys) {
                physaddr = physaddrbase + (i * sizeof(ul));
                fprintf(stderr, 
                        "FAILURE: 0x%08lx != 0x%08lx at physical address "
                        "0x%08lx.\n", 
                        (ul) *p1, (ul) *p2, physaddr);
            } else {
                fprintf(stderr, 
                        "FAILURE: 0x%08lx != 0x%08lx at offset 0x%08lx.\n", 
                        (ul) *p1, (ul) *p2, (ul) (i * sizeof(ul)));
            }
            /* printf("Skipping to next test..."); */
            r = -1;
        }
    }
    return r;
}

static int test_stuck_address(ulv *bufa, size_t count) {
    ulv *p1 = bufa;
    unsigned int j;
    size_t i;
    unsigned long physaddr;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 16; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            *p1 = ((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1);
            *p1++;
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        p1 = (ulv *) bufa;
        for (i = 0; i < count; i++, p1++) {
            if (*p1 != (((j + i) % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
                if (use_phys) {
                    physaddr = physaddrbase + (i * sizeof(ul));
                    fprintf(stderr, 
                            "FAILURE: possible bad address line at physical "
                            "address 0x%08lx.\n", 
                            physaddr);
                } else {
                    fprintf(stderr, 
                            "FAILURE: possible bad address line at offset "
                            "0x%08lx.\n", 
                            (ul) (i * sizeof(ul)));
                }
                printf("Skipping to next test...\n");
                fflush(stdout);
                return -1;
            }
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

static int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    ul j = 0;
    size_t i;

    // putchar(' ');
    printf(" ");
    fflush(stdout);
    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = rand_ul();
        if (!(i % PROGRESSOFTEN)) {
            printf("\b");
            printf("%c", progress[++j % PROGRESSLEN]);
            fflush(stdout);
        }
    }
    printf("\b \b");
    fflush(stdout);
    return compare_regions(bufa, bufb, count);
}

static int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
    }
    return compare_regions(bufa, bufb, count);
}

static int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
    }
    return compare_regions(bufa, bufb, count);
}

static int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
    }
    return compare_regions(bufa, bufb, count);
}

static int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        if (!q) {
            q++;
        }
        *p1++ /= q;
        *p2++ /= q;
    }
    return compare_regions(bufa, bufb, count);
}

static int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
    }
    return compare_regions(bufa, bufb, count);
}

static int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
    }
    return compare_regions(bufa, bufb, count);
}

static int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    size_t i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
    }
    return compare_regions(bufa, bufb, count);
}

static int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 64; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? UL_ONEBITS : 0;
        printf("setting %3u", j);
        fflush(stdout);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

static int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 64; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
        printf("setting %3u", j);
        fflush(stdout);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

static int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 256; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul) UL_BYTE(j);
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

static int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = ONE << j;
            } else { /* Walk it back down. */
                *p1++ = *p2++ = ONE << (UL_LEN * 2 - j - 1);
            }
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

static int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << j);
            } else { /* Walk it back down. */
                *p1++ = *p2++ = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
            }
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

static int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;
        printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            if (j < UL_LEN) { /* Walk it up. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? ((unsigned int)ONE << j) | ((unsigned int)ONE << (j + 2))
                    : UL_ONEBITS ^ (((unsigned int)ONE << j)
                                    | ((unsigned int)ONE << (j + 2)));
            } else { /* Walk it back down. */
                *p1++ = *p2++ = (i % 2 == 0)
                    ? ((unsigned int)ONE << (UL_LEN * 2 - 1 - j)) | ((unsigned int)ONE << (UL_LEN * 2 + 1 - j))
                    : UL_ONEBITS ^ ((unsigned int)ONE << (UL_LEN * 2 - 1 - j)
                                    | ((unsigned int)ONE << (UL_LEN * 2 + 1 - j)));
            }
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

static int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j, k;
    ul q;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (k = 0; k < UL_LEN; k++) {
        q = ONE << k;
        for (j = 0; j < 8; j++) {
            printf("\b\b\b\b\b\b\b\b\b\b\b");
            q = ~q;
            printf("setting %3u", k * 8 + j);
            fflush(stdout);
            p1 = (ulv *) bufa;
            p2 = (ulv *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
            }
            printf("\b\b\b\b\b\b\b\b\b\b\b");
            printf("testing %3u", k * 8 + j);
            fflush(stdout);
            if (compare_regions(bufa, bufb, count)) {
                return -1;
            }
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

#ifdef TEST_NARROW_WRITES    
int test_8bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u8v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    printf(" ");
    fflush(stdout);
    for (attempt = 0; attempt < 2;  attempt++) {
        if (attempt & 1) {
            p1 = (u8v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u8v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword8.bytes;
            *p2++ = mword8.val = rand_ul();
            for (b=0; b < UL_LEN/8; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                //  putchar('\b');
                printf("\b");
                //  putchar(progress[++j % PROGRESSLEN]);
                printf("%c", progress[++j % PROGRESSLEN]);
                fflush(stdout);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b \b");
    fflush(stdout);
    return 0;
}

int test_16bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
    u16v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    printf(" ");
    fflush( stdout );
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u16v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u16v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword16.u16s;
            *p2++ = mword16.val = rand_ul();
            for (b = 0; b < UL_LEN/16; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                // putchar('\b');
                    printf("\b");
                //putchar(progress[++j % PROGRESSLEN]);             
                printf("%c", progress[++j % PROGRESSLEN]);

                fflush(stdout);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b \b");
    fflush(stdout);
    return 0;
}
#endif

static struct test tests[] = {
    { "Random Value", test_random_value },
    { "Compare XOR", test_xor_comparison },
    { "Compare SUB", test_sub_comparison },
    { "Compare MUL", test_mul_comparison },
    { "Compare DIV",test_div_comparison },
    { "Compare OR", test_or_comparison },
    { "Compare AND", test_and_comparison },
    { "Sequential Increment", test_seqinc_comparison },
    { "Solid Bits", test_solidbits_comparison },
    { "Block Sequential", test_blockseq_comparison },
    { "Checkerboard", test_checkerboard_comparison },
    { "Bit Spread", test_bitspread_comparison },
    { "Bit Flip", test_bitflip_comparison },
    { "Walking Ones", test_walkbits1_comparison },
    { "Walking Zeroes", test_walkbits0_comparison },
#ifdef TEST_NARROW_WRITES    
    { "8-bit Writes", test_8bit_wide_random },
    { "16-bit Writes", test_16bit_wide_random },
#endif
    { NULL, NULL }
};



/* Sanity checks and portability helper macros. */
#ifdef _SC_VERSION
static void check_posix_system(void) {
    if (sysconf(_SC_VERSION) < 198808L) {
        fprintf(stderr, "A POSIX system is required.  Don't be surprised if "
            "this craps out.\n");
        fprintf(stderr, "_SC_VERSION is %lu\n", sysconf(_SC_VERSION));
    }
}
#else
#define check_posix_system()
#endif

#ifdef _SC_PAGE_SIZE
static int memtester_pagesize(void) {
    int pagesize = sysconf(_SC_PAGE_SIZE);
    if (pagesize == -1) {
        perror("get page size failed");
        exit(EXIT_FAIL_NONSTARTER);
    }
    printf("pagesize is %ld\n", (long) pagesize);
    return pagesize;
}
#else
static int memtester_pagesize(void) {
    printf("sysconf(_SC_PAGE_SIZE) not supported; using pagesize of 8192\n");  //sysconf(_SC_PAGE_SIZE)不受支持，因此使用了一个页面大小为8192字节的默认值。
    return 8192;
}
#endif

/* Some systems don't define MAP_LOCKED.  Define it to 0 here
    so it's just a no-op when ORed with other constants. */
#ifndef MAP_LOCKED
    #define MAP_LOCKED 0
#endif

/* Function declarations */
static void usage(char *me);

/* Global vars - so tests have access to this information */
//int use_phys = 0;
//off_t physaddrbase = 0;

/* Function definitions */
static void usage(char *me) {
    fprintf(stderr, "\n""Usage: %s [-p physaddrbase [-d device]] <mem>[B|K|M|G] [loops]\n",me);
    exit(EXIT_FAIL_NONSTARTER);//表示测试程序未能启动。
}

static int memtester_main(int argc, char **argv) {
    
    ul loops, loop, i;
    size_t  wantraw, wantmb,  wantbytes_orig, bufsize,halflen, count;
    long  wantbytes,pagesize;
    char *memsuffix, *addrsuffix, *loopsuffix;
    ptrdiff_t pagesizemask;
    void volatile *buf, *aligned;
    ulv *bufa, *bufb;
    int do_mlock = 1, done_mem = 0;
    int exit_code = 0;
    optind=1;
    int memfd, opt, memshift=0;
    size_t maxbytes = -1; /* addressable memory, in bytes */
    size_t maxmb = (maxbytes >> 20) + 1; /* addressable memory, in MB */
    /* Device to mmap memory from with -p, default is normal core */
    char *device_name = "/dev/mem";
    struct stat statbuf;
    int device_specified = 0;
    char *env_testmask = 0;
    ul testmask = 0;

    printf("memtester version " __version__ " (%d-bit)\n", UL_LEN);     //使用的是memtester版本4.5.0，
    printf("Copyright (C) 2001-2020 Charles Cazabon.\n");               //由Charles Cazabon开发
    printf("Licensed under the GNU General Public License version 2 (only).\n");    //遵循GNU通用公共许可证第2版（仅此版本）。
    printf("\n");
    check_posix_system();
    pagesize = memtester_pagesize();
    pagesizemask = (ptrdiff_t) ~(pagesize - 1);
    printf("pagesizemask is 0x%tx\n", pagesizemask);    //pagesizemask is 0xtx
    
    /* If MEMTESTER_TEST_MASK is set, we use its value as a mask of which
        tests we run.
        */
    if (env_testmask = getenv("MEMTESTER_TEST_MASK")) {
        errno = 0;
        testmask = strtoul(env_testmask, 0, 0);
        if (errno) {
            fprintf(stderr, "error parsing MEMTESTER_TEST_MASK %s: %s\n",env_testmask, strerror(errno));
            usage(argv[0]); /* doesn't return */
        }
        printf("using testmask 0x%lx\n", testmask);
    }

    while ((opt = getopt(argc, argv, "p:d:")) != -1) {
        switch (opt) {
            case 'p':
                errno = 0;
                physaddrbase = (off_t) strtoull(optarg, &addrsuffix, 16);
                if (errno != 0) {
                    fprintf(stderr,
                            "failed to parse physaddrbase arg; should be hex "
                            "address (0x123...)\n");
                    usage(argv[0]); /* doesn't return */
                }
                if (*addrsuffix != '\0') {
                    /* got an invalid character in the address */
                    fprintf(stderr,
                            "failed to parse physaddrbase arg; should be hex "
                            "address (0x123...)\n");
                    usage(argv[0]); /* doesn't return */
                }
                if (physaddrbase & (pagesize - 1)) {
                    fprintf(stderr,
                            "bad physaddrbase arg; does not start on page "
                            "boundary\n");
                    usage(argv[0]); /* doesn't return */
                }
                /* okay, got address */
                use_phys = 1;
                break;
            case 'd':
                if (stat(optarg,&statbuf)) {
                    fprintf(stderr, "can not use %s as device: %s\n", optarg, strerror(errno));
                    usage(argv[0]); /* doesn't return */
                } else {
                    if (!S_ISCHR(statbuf.st_mode)) {
                        fprintf(stderr, "can not mmap non-char device %s\n", optarg);
                        usage(argv[0]); /* doesn't return */
                    } else {
                        device_name = optarg;
                        device_specified = 1;
                    }
                }
                break;              
            default: /* '?' */
                usage(argv[0]); /* doesn't return */
        }
    }

    if (device_specified && !use_phys) {
        fprintf(stderr, "for mem device, physaddrbase (-p) must be specified\n");
        usage(argv[0]); /* doesn't return */
    }
    
    if (optind >= argc) {
        fprintf(stderr, "need memory argument, in MB\n");
        usage(argv[0]); /* doesn't return */
    }

    errno = 0;
    wantraw = (size_t) strtoul(argv[optind], &memsuffix, 0);
    if (errno != 0) {
        fprintf(stderr, "failed to parse memory argument");
        usage(argv[0]); /* doesn't return */
    }
    switch (*memsuffix) {
        case 'G':
        case 'g':
            memshift = 30; /* gigabytes */
            break;
        case 'M':
        case 'm':
            memshift = 20; /* megabytes */
            break;
        case 'K':
        case 'k':
            memshift = 10; /* kilobytes */
            break;
        case 'B':
        case 'b':
            memshift = 0; /* bytes*/
            break;
        case '\0':  /* no suffix */
            memshift = 20; /* megabytes */
            break;
        default:
            /* bad suffix */
            usage(argv[0]); /* doesn't return */
    }
    wantbytes_orig = wantbytes = ((size_t) wantraw << memshift);
    wantmb = (wantbytes_orig >> 20);
    optind++;
    if (wantmb > maxmb) {
        fprintf(stderr, "This system can only address %llu MB.\n", (ull) maxmb);
        exit(EXIT_FAIL_NONSTARTER);  //表示测试程序未能启动。
    }
    if (wantbytes < pagesize) {
        fprintf(stderr, "bytes %ld < pagesize %ld -- memory argument too large?\n",wantbytes, pagesize);
        exit(EXIT_FAIL_NONSTARTER);  //表示测试程序未能启动。
    }

    if (optind >= argc) {
        loops = 0;
    } else {
        errno = 0;
        loops = strtoul(argv[optind], &loopsuffix, 0);
        if (errno != 0) {
            fprintf(stderr, "failed to parse number of loops");
            usage(argv[0]); /* doesn't return */
        }
        if (*loopsuffix != '\0') {
            fprintf(stderr, "loop suffix %c\n", *loopsuffix);
            usage(argv[0]); /* doesn't return */
        }
    }

    printf("want %lluMB (%llu bytes)\n", (ull) wantmb, (ull) wantbytes);    //want 256MB (268435456 bytes)
    buf = NULL;

    if (use_phys) {
        memfd = open(device_name, O_RDWR | O_SYNC);
        if (memfd == -1) {
            fprintf(stderr, "failed to open %s for physical memory: %s\n",device_name, strerror(errno));
            exit(EXIT_FAIL_NONSTARTER);  //表示测试程序未能启动。
        }
        buf = (void volatile *) mmap(0, wantbytes, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_LOCKED, memfd,physaddrbase);
        if (buf == MAP_FAILED) {
            fprintf(stderr, "failed to mmap %s for physical memory: %s\n",device_name, strerror(errno));
            exit(EXIT_FAIL_NONSTARTER);  //表示测试程序未能启动。
        }

        if (mlock((void *) buf, wantbytes) < 0) {
            fprintf(stderr, "failed to mlock mmap'ed space\n");
            do_mlock = 0;
        }

        bufsize = wantbytes; /* accept no less */
        aligned = buf;
        done_mem = 1;
    }

    while (!done_mem) {
        while (!buf && wantbytes) {
            buf = (void volatile *) malloc(wantbytes);
            if (!buf) wantbytes -= pagesize;
        }
        bufsize = wantbytes;
        printf("got  %lluMB (%llu bytes)", (ull) wantbytes >> 20,   (ull) wantbytes);                                       //got  46MB (49225728 bytes)
        fflush(stdout);
        if (do_mlock) {
            printf(", trying mlock ...");       //trying mlock ...locked.
            fflush(stdout);
            if ((size_t) buf % pagesize) {
                /* printf("aligning to page -- was 0x%tx\n", buf); */
                aligned = (void volatile *) ((size_t) buf & pagesizemask) + pagesize;
                /* printf("  now 0x%tx -- lost %d bytes\n", aligned,
                    *      (size_t) aligned - (size_t) buf);
                    */
                bufsize -= ((size_t) aligned - (size_t) buf);
            } else {
                aligned = buf;
            }
            /* Try mlock */
            if (mlock((void *) aligned, bufsize) < 0) {
                switch(errno) {
                    case EAGAIN: /* BSDs */
                        printf("over system/pre-process limit, reducing...\n");
                        free((void *) buf);
                        buf = NULL;
                        wantbytes -= pagesize;
                        break;
                    case ENOMEM:
                        printf("too many pages, reducing...\n");
                        free((void *) buf);
                        buf = NULL;
                        wantbytes -= pagesize;
                        break;
                    case EPERM:
                        printf("insufficient permission.\n");
                        printf("Trying again, unlocked:\n");
                        do_mlock = 0;
                        free((void *) buf);
                        buf = NULL;
                        wantbytes = wantbytes_orig;
                        break;
                    default:
                        printf("failed for unknown reason.\n");
                        do_mlock = 0;
                        done_mem = 1;
                }
            } else {
                printf("locked.\n");
                done_mem = 1;
            }
        } else {
            done_mem = 1;
            printf("\n");
        }
    }

    if (!do_mlock) fprintf(stderr, "Continuing with unlocked memory; testing ""will be slower and less reliable.\n");

    /* Do alighnment here as well, as some cases won't trigger above if you
        define out the use of mlock() (cough HP/UX 10 cough). */
    if ((size_t) buf % pagesize) {
        /* printf("aligning to page -- was 0x%tx\n", buf); */
        aligned = (void volatile *) ((size_t) buf & pagesizemask) + pagesize;
        /* printf("  now 0x%tx -- lost %d bytes\n", aligned,
            *      (size_t) aligned - (size_t) buf);
            */
        bufsize -= ((size_t) aligned - (size_t) buf);
    } 
    else {
        aligned = buf;
    }

    halflen = bufsize / 2;
    count = halflen / sizeof(ul);
    bufa = (ulv *) aligned;
    bufb = (ulv *) ((size_t) aligned + halflen);

    for(loop=1; ((!loops) || loop <= loops); loop++) {
        printf("Loop %lu", loop);
        if (loops) {
            printf("/%lu", loops);
        }
        printf(":\n");
        printf("  %-20s: ", "Stuck Address");   //Stuck Address       : ok   
        fflush(stdout);
        if (!test_stuck_address(aligned, bufsize / sizeof(ul))) {
                printf("ok\n");
        } 
        else {
            exit_code |= EXIT_FAIL_ADDRESSLINES;    //表示地址线测试失败。
        }
        for (i=0;;i++) {
            usleep(100000);
            if(suspend == 3){
                printf("suspend \n");
                if(buf)
                {
                    free((void *) buf);
                    buf = NULL;
                }
                return suspend;
            }
            if (!tests[i].name) break;
            /* If using a custom testmask, only run this test if the
                bit corresponding to this test was set by the user.
                */
            if (testmask && (!((1 << i) & testmask))) {
                continue;
            }
            printf("  %-20s: ", tests[i].name);
            if (!tests[i].fp(bufa, bufb, count)) {
                printf("ok\n");
            } 
            else {                                    //出错，在这里！！！
                exit_code |= EXIT_FAIL_OTHERTEST;       //表示其他测试项目失败。
            }
            fflush(stdout);
            /* clear buffer */
            memset((void *) buf, 255, wantbytes);
        }
        loop_num=loop;
        
        printf("\n");
        fflush(stdout);
    }
    if (do_mlock) munlock((void *) aligned, bufsize);
    printf("Done.\n");
    fflush(stdout);
    exit(exit_code);
    return 0;
}
    

/**
 * @brief ddr压力测试
 *
 * @param	
 			无参（默认ddr大小是free的70%，测试时间是12h）
 * @return	
            BOARDTEST_FAIL:ddr压力测试失败
            BOARDTEST_CALL_PASS:ddr压力测试成功
 */
int hc_test_ddr_pressure_start(void)
{
    int memorySize=0;  //用于测试的ddr大小
    int test_count=144;   //测试的次数 测试一次约为5分钟，144次约为12h
    int result=0;
    unsigned int len=0;
    char cmd[4][128]={0};
    void *ptr;
    size_t size_sum=0;
    size_t size = 1024*1024; // 初始分配1M内存
    loop_num=0;

    suspend=0;  //标志位初始化为0

	hc_test_fixup_ddr_setting(); /* adaptive ddr setting fixup, maybe reduce the ddr running clock */
    while((ptr = malloc(size_sum + size)) != NULL)  // 1M/2M/4M/8M/16M/32M ....
    {
        printf("Successfully allocated %lu M\n", size/(1024*1024));
        free(ptr);// 释放分配的内存以防止内存泄漏
        size *= 2; // 每次分配的内存大小*2
    }
    size/=2;
    size_sum=size;

    while(1)
    {
        size/=2;
        if(size/(1024*1024) ==0)
        {
            break;
        }
        while((ptr = malloc(size_sum + size)) != NULL)      //32/32+16/32+16+8 ...
        {
            printf("Successfully allocated %lu M\n", size/(1024*1024));
            free(ptr);// 释放分配的内存以防止内存泄漏
            size_sum+=size;
            size/= 2; 
        }
    }

    memorySize=(size_sum/(1024*1024))*7/10;  //用于测试的ddr大小是free大小的70%
    if( test_count ==0 || memorySize < 20){
        char err[128]={0};
        sprintf(err,"memorySize =%d , <20",memorySize);
        write_boardtest_detail(BOARDTEST_DDR_STRESS_TEST, err); //用于测试的内存小于20M
        printf("memorySize = %d\n",memorySize);
        return BOARDTEST_FAIL; //输入的参数不符合常理
    }

    len=snprintf(cmd[0],sizeof(cmd[0]),"%s","memtester");
    len=snprintf(cmd[1],sizeof(cmd[0]),"%d",memorySize);
    len=snprintf(cmd[2],sizeof(cmd[0]),"%d",test_count);

    char* buffer[4]={0};
    for (int i = 0; i < 3; i++) {
        buffer[i] = cmd[i];
    }
    buffer[3]=NULL;
    printf("%s,%s,%s\n",buffer[0],buffer[1],buffer[2]);
    printf("Running DDR pressure test...\n");

    result =memtester_main(3,buffer);
    if (result == 0) //测试成功
    {
        printf("DDR pressure test passed.\n");
        return BOARDTEST_PASS;
    }
    else //测试失败
    {
        if(result == 0x01){
            sprintf(error_message,"EXIT_FAIL_NONSTARTER");
            write_boardtest_detail(BOARDTEST_DDR_STRESS_TEST, error_message); //测试程序未能启动。
        }
        else if(result == 0x02){
            sprintf(error_message,"EXIT_FAIL_ADDRESSLINES");
            write_boardtest_detail(BOARDTEST_DDR_STRESS_TEST, error_message); //地址线测试失败。
        }
        else if(result == 0x04){
            sprintf(error_message,"EXIT_FAIL_OTHERTEST");
            write_boardtest_detail(BOARDTEST_DDR_STRESS_TEST, error_message); //其他测试项目失败。
        }
        else if(result == 0x03){
            sprintf(error_message,"hc_test_ddr_pressure_stop %d/%d",loop_num,test_count);
            write_boardtest_detail(BOARDTEST_DDR_STRESS_TEST, error_message); //中断退出
            if(loop_num == 0){
                return BOARDTEST_FAIL;
            }
            else{
                return BOARDTEST_RESULT_PASS;
            }
        }
        return BOARDTEST_FAIL;        
    }
}

/**
 * @brief 退出ddr压力测试
 *
 * @param	
 			无参
 * @return	
            BOARDTEST_FAIL:退出ddr压力测试函数失败
            BOARDTEST_PASS:退出ddr压力测试函数成功
 */
int hc_test_ddr_pressure_stop(void) 
{
    suspend=3;  //标志位
    return BOARDTEST_PASS;
}

/**
 * @brief ddr test stress register func
 *
 * @return int
 */
static int hc_boardtest_ddr_stress_test(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "DDR_STRESS_TEST"; 
    test->sort_name = BOARDTEST_DDR_STRESS_TEST;      
    test->init = NULL;
    test->run = hc_test_ddr_pressure_start;
    test->exit =hc_test_ddr_pressure_stop;
    test->tips = NULL; 

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief register
 *
 */
__initcall(hc_boardtest_ddr_stress_test);
