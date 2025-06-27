#include <fcntl.h>
#include <unistd.h>
#include <kernel/vfs.h>
#include <stdio.h>
#include <kernel/io.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <kernel/lib/console.h>
#include <kernel/completion.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>
#include <hcuapi/pq.h>

enum {
    INV_GAMMA_LINEAR = 1,
    INV_GAMMA_LUT,
    INV_GAMMA_LUT_R,
    INV_GAMMA_LUT_G,
    INV_GAMMA_LUT_B,
    MATRIX,
    GAIN_OFFSET,
    GAIN_OFFSET_R,
    GAIN_OFFSET_G,
    GAIN_OFFSET_B,
    SOFT_LIMITER,
    GAMMA_LUT,
    GAMMA_LUT_R,
    GAMMA_LUT_G,
    GAMMA_LUT_B,
    UNKNOWN,
};

static const char *msg_from_id(int id) {
    switch (id) {
        case INV_GAMMA_LINEAR: return "inv_gamma_linear";
        case INV_GAMMA_LUT: return "inv_gamma_lut";
        case INV_GAMMA_LUT_R: return "inv_gamma_lut_r";
        case INV_GAMMA_LUT_G: return "inv_gamma_lut_g";
        case INV_GAMMA_LUT_B: return "inv_gamma_lut_b";
        case MATRIX: return "matrix";
        case GAIN_OFFSET: return "gain_offset";
        case GAIN_OFFSET_R: return "gain_offset_r";
        case GAIN_OFFSET_G: return "gain_offset_g";
        case GAIN_OFFSET_B: return "gain_offset_b";
        case SOFT_LIMITER: return "soft_limiter";
        case GAMMA_LUT: return "gamma_lut";
        case GAMMA_LUT_R: return "gamma_lut_r";
        case GAMMA_LUT_G: return "gamma_lut_g";
        case GAMMA_LUT_B: return "gamma_lut_b";
        case UNKNOWN: // fall through
        default: return "unknown";
    }
}

static int parse_int_seq(const char *s, int n, int *array) {
    const char *next = s;
    int *p = array;
    for (int i = 0; i < n && *next != '\0'; ++i) {
        char *stop;
        *p++ = strtol(next, &stop, 0);
        next = stop;
        while (*next != '\0' && *next == ',') { // skip seperator
            ++next;
        }
    }
    return p - array;
}

static void parse_gain_offset_pair(const char *s, int *gain, int *offset) {
    *gain = 128;
    *offset = 0;
    sscanf(s, "%d,%d", gain, offset);
}

static void parse_gain_and_offset(int opt_modifier, const char *arg, struct pq_settings *d) {
    if (opt_modifier == '\0') {
        parse_gain_offset_pair(arg, &d->gntf_gain_b, &d->gntf_offset_b);
        d->gntf_gain_r = d->gntf_gain_g = d->gntf_gain_b;
        d->gntf_offset_r = d->gntf_offset_g = d->gntf_offset_b;
    } else if (opt_modifier == 'r') {
        parse_gain_offset_pair(arg, &d->gntf_gain_r, &d->gntf_offset_r);
    } else if (opt_modifier == 'g') {
        parse_gain_offset_pair(arg, &d->gntf_gain_g, &d->gntf_offset_g);
    } else if (opt_modifier == 'b') {
        parse_gain_offset_pair(arg, &d->gntf_gain_b, &d->gntf_offset_b);
    }
    d->gntf_en = true;
}

static void parse_linear_region(const char *s, struct pq_settings *d) {
    sscanf(s, "%u,%u", &d->linear_region, &d->linear_slope);
    d->linear_region_enable = true;
    // printf("linear region: %d, %d, %d\n",
    //     d->linear_region_enable, d->linear_region, d->linear_slope);
}

static int parse_invgamma(int opt_modifier, const char *arg, struct pq_settings *d) {
    int n = sizeof(d->invgamma_lut_r) / sizeof(d->invgamma_lut_r[0]);
    int sz = sizeof(d->invgamma_lut_r);
    if (opt_modifier == '\0') {
        if (parse_int_seq(arg, n, d->invgamma_lut_r) != n)
            return -INV_GAMMA_LUT;
        memcpy(d->invgamma_lut_g, d->invgamma_lut_r, sz);
        memcpy(d->invgamma_lut_b, d->invgamma_lut_r, sz);
    } else if (opt_modifier == 'r') {
        if (parse_int_seq(arg, n, d->invgamma_lut_r) != n)
            return -INV_GAMMA_LUT_R;
    } else if (opt_modifier == 'g') {
        if (parse_int_seq(arg, n, d->invgamma_lut_g) != n)
            return -INV_GAMMA_LUT_G;
    } else if (opt_modifier == 'b') {
        if (parse_int_seq(arg, n, d->invgamma_lut_b) != n)
            return -INV_GAMMA_LUT_B;
    } else {
        return -UNKNOWN;
    }
    d->igam_en = true;
    return 0;
}

static void parse_soft_limiter(const char *s, struct pq_settings *d) {
    int neg_en = 0;
    int max_en = 0;
    sscanf(s, "%d,%d,%u,%u,%d", &neg_en, &max_en,
        &d->slmt_neg_oftgn, &d->slmt_slope, &d->slmt_delta);
    d->slmt_neg_en = neg_en;
    d->slmt_max_en = max_en;
    d->slmt_en = neg_en || max_en;
}

static int parse_gamma(int opt_modifier, const char *arg, struct pq_settings *d) {
    int n = sizeof(d->gamma_lut_r) / sizeof(d->gamma_lut_r[0]);
    int sz = sizeof(d->gamma_lut_r);
    if (opt_modifier == '\0') {
        if (parse_int_seq(arg, n, d->gamma_lut_r) != n)
            return -INV_GAMMA_LUT;
        memcpy(d->gamma_lut_g, d->gamma_lut_r, sz);
        memcpy(d->gamma_lut_b, d->gamma_lut_r, sz);
    } else if (opt_modifier == 'r') {
        if (parse_int_seq(arg, n, d->gamma_lut_r) != n)
            return -INV_GAMMA_LUT_R;
    } else if (opt_modifier == 'g') {
        if (parse_int_seq(arg, n, d->gamma_lut_g) != n)
            return -INV_GAMMA_LUT_G;
    } else if (opt_modifier == 'b') {
        if (parse_int_seq(arg, n, d->gamma_lut_b) != n)
            return -INV_GAMMA_LUT_B;
    } else {
        return -UNKNOWN;
    }
    d->gam_en = true;
    return 0;
}

/// @returns 1: arg is consumed, 0: arg is not consumed, < 0: error
static int parse_arg(int opt, int opt_modifier, const char *arg,
                     struct pq_settings *d) {
    int ret = 0;
    switch (opt) {
        case 'l': // inv-gamma linear region
            // region,slope
            if (arg == NULL) return -UNKNOWN;
            parse_linear_region(arg, d);
            return 1;
        case 'i': // inv-gamma
            if (arg == NULL) return -UNKNOWN;
            ret = parse_invgamma(opt_modifier, arg, d);
            if (ret < 0)
                return ret;
            return 1;
        case 'm': // matrix
            // c00,c01,c02,c10,c11,c12,c20,c21,c22
            if (arg == NULL) return -UNKNOWN;
            if (parse_int_seq(arg, 9, &d->mtx_c00) != 9) {
                return -MATRIX;
            }
            d->mtx_en = true;
            return 1;
        case 'g': // gain and offset
            if (arg == NULL) return -UNKNOWN;
            parse_gain_and_offset(opt_modifier, arg, d);
            return 1;
        case 's': // soft-limiter
            if (arg == NULL) return -UNKNOWN;
            parse_soft_limiter(arg, d);
            return 1;
        case 't': // gamma transfer func
            if (arg == NULL) return -UNKNOWN;
            ret = parse_gamma(opt_modifier, arg, d);
            if (ret < 0)
                return ret;
            return 1;
        default:
            return -UNKNOWN;
    }
}

struct op_t {
    bool reset;
    bool list;
    bool program_hw;
};

/// @returns 1: arg is consumed, 0: arg is not consumed, < 0: error
static int parse_long_arg(const char *opt, struct op_t *op) {
    if (strcmp(opt, "reset") == 0) {
        op->reset = 1;
        return 0;
    } else if (strcmp(opt, "list") == 0) {
        op->list = 1;
        return 0;
    } else if (strcmp(opt, "program-hw") == 0) {
        op->program_hw = 1;
        return 0;
    }
    return -UNKNOWN;
}

static int parse_args(int argc, char *argv[], struct pq_settings *d, struct op_t *op) {
    for (int i = 1; i < argc; ++i) {
        const char *opt = argv[i];
        const char *arg = i + 1 < argc ? argv[i + 1] : NULL;
        // printf("parsing option %s\n", opt);
        int ret = -UNKNOWN;
        if (opt[0] == '-' && opt[1] == '-') {
            ret = parse_long_arg(opt + 2, op);
        } else if (opt[0] == '-' && opt[1] != '-') {
            ret = parse_arg(opt[1], opt[2], arg, d);
        }
        if (ret < 0) {
            return ret;
        } else if (ret > 0) {
            ++i; // argument is consumed
        }
    }
    return 0;
}

static void print_int_array(const int *array, int n, int n_per_line) {
    for (int i = 0; i < n; ++i) {
        printf("%s%4d,", i % n_per_line == 0 ? "    " : " ", array[i]);
        if ((i + 1) % n_per_line == 0 || i + 1 == n)
            printf("\n");
    }
}

static void print_settings(const struct pq_settings *cfg) {
    printf("inv-gamma %s\n", cfg->igam_en ? "on" : "off");
    if (cfg->igam_en) {
        printf("    linear region: %d, %d, %d\n",
            cfg->linear_region_enable, cfg->linear_region, cfg->linear_slope);
        const int sz = sizeof(cfg->invgamma_lut_r);
        const int n = sz / sizeof(cfg->invgamma_lut_r[0]);
        int rg_match = memcmp(cfg->invgamma_lut_r, cfg->invgamma_lut_g, sz) == 0;
        int rb_match = memcmp(cfg->invgamma_lut_r, cfg->invgamma_lut_b, sz) == 0;
        if (rg_match && rb_match) {
            print_int_array(cfg->invgamma_lut_r, n, 16);
        } else {
            print_int_array(cfg->invgamma_lut_r, n, 16);
            print_int_array(cfg->invgamma_lut_g, n, 16);
            print_int_array(cfg->invgamma_lut_b, n, 16);
        }
    }
    printf("matrix %s\n", cfg->mtx_en ? "on" : "off");
    if (cfg->mtx_en) {
        printf("    %5d, %5d, %5d,\n", cfg->mtx_c00, cfg->mtx_c01, cfg->mtx_c02);
        printf("    %5d, %5d, %5d,\n", cfg->mtx_c10, cfg->mtx_c11, cfg->mtx_c12);
        printf("    %5d, %5d, %5d,\n", cfg->mtx_c20, cfg->mtx_c21, cfg->mtx_c22);
    }
    printf("gain-offset %s\n", cfg->gntf_en ? "on" : "off");
    if (cfg->gntf_en) {
        printf("    %4d, %4d\n", cfg->gntf_gain_r, cfg->gntf_offset_r);
        printf("    %4d, %4d\n", cfg->gntf_gain_g, cfg->gntf_offset_g);
        printf("    %4d, %4d\n", cfg->gntf_gain_b, cfg->gntf_offset_b);
    }
    printf("soft-limiter %s\n", cfg->slmt_en ? "on" : "off");
    if (cfg->slmt_en) {
        printf("    cfg %d %d %d %d %d\n",
            cfg->slmt_neg_en, cfg->slmt_max_en,
            cfg->slmt_neg_oftgn, cfg->slmt_slope, cfg->slmt_delta);
    }
    printf("gamma-tf %s\n", cfg->gam_en ? "on" : "off");
    if (cfg->gam_en) {
        const int sz = sizeof(cfg->gamma_lut_r);
        const int n = sz / sizeof(cfg->gamma_lut_r[0]);
        int rg_match = memcmp(cfg->gamma_lut_r, cfg->gamma_lut_g, sz) == 0;
        int rb_match = memcmp(cfg->gamma_lut_r, cfg->gamma_lut_b, sz) == 0;
        if (rg_match && rb_match) {
            print_int_array(cfg->gamma_lut_r, n, 16);
        } else {
            print_int_array(cfg->gamma_lut_r, n, 16);
            print_int_array(cfg->gamma_lut_g, n, 16);
            print_int_array(cfg->gamma_lut_b, n, 16);
        }
    }
}

static void print_usage(const char *prog_name) {
    printf("%s [-l linear_region,slope] [-i[r|g|b] inv-gamma-lut] [-m matrix] [-g[r|g|b] gain,offset] [-s neg_en,max_en,offset,slope,delta] [-t[r|g|b] gamma-lut]\n", prog_name);
    printf("%s [--reset] [--program-hw] [--list]", prog_name);
}

static int pq_cfg_main(int argc, char *argv[])
{
    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }

    const char *dev_path = "/dev/pq";
    int pq_fd = open(dev_path, O_WRONLY);
    if(pq_fd < 0) {
        printf("Failed to open device %s\n", dev_path);
        return -1;
    }
    static struct pq_settings cfg = {
        .name = "test",
    };
    struct op_t op = {};

    int ret = parse_args(argc, argv, &cfg, &op);
    if (ret < 0) {
        printf("Failed parse option %s\n", msg_from_id(-ret));
        return -1;
    }
    if (op.reset) {
        printf("reseting static PQ setting...\n");
        memset(&cfg, 0, sizeof(cfg));
    }
    cfg.pq_en = cfg.igam_en || cfg.mtx_en || cfg.gntf_en || cfg.slmt_en || cfg.gam_en;
    if (op.list) {
        print_settings(&cfg);
    }

    if (op.program_hw) {
        ioctl(pq_fd , PQ_SET_PARAM, &cfg);
        ioctl(pq_fd , PQ_START);
    }

    return 0;
}

CONSOLE_CMD(pq_cfg, NULL, pq_cfg_main, CONSOLE_CMD_MODE_SELF, "PQ hardware configuration tool")