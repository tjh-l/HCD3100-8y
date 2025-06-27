#include <stdio.h>
#include <stdlib.h>
// #include <time.h>
#include <unistd.h>
#include <fcntl.h>
// #include <termios.h>
// #include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
// #include <kernel/lib/fdt_api.h>
#include <sys/ioctl.h>
#include <hcuapi/dis.h>

#include <kernel/lib/console.h>
// #include <kernel/completion.h>
// #include <hcuapi/common.h>
// #include <poll.h>

// #include <errno.h>
// #include <nuttx/compiler.h>
// #include <nuttx/semaphore.h>

/*  syntax: dis -dev hd/sd/4k [log.counter.modulo=30]
 */
int dis_main(int argc, char **argv)
{
    const char *dis_path = "/dev/dis";
    int dis_fd = open(dis_path, O_RDWR);
    if (dis_fd == -1) {
        printf("failed to open %s\n", dis_path);
        return -1;
    }

    for (int i = 1; i < argc; ++i) {
        const char *msg = argv[i];
        write(dis_fd, msg, strlen(msg) + 1);
    }
    close(dis_fd);
    return 0;
}

static char *trim_space(char *s, int *n) {
    int len = strlen(s);
    if (len == 0) {
        *n = 0;
        return s;
    }
    char *p = s + len - 1;
    while (isspace(*p) && p > s) {
        --p;
    }
    p[1] = '\0';
    while (*s != '\0' && isspace(*s)) {
        ++s;
    }
    if (n)
        *n = p + 1 - s;
    return s;
}

static void read_user_scenes(FILE *fp, struct dis_dyn_enh_user_scenes *usc) {
    char buffer[120];
    // a line starts with '#' is comment
    // first data line: histogram bounds
    // then follows scenes, one scene each line
    //      v0 v1 v2 v3 v4  |  contrast pivot black_level  |  desc
    int data_line_idx = 0;
    usc->n_scenes = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        buffer[sizeof(buffer) - 1] = '\0'; // make sure properly terminated
        int len;
        const char *line = trim_space(buffer, &len);
        if (len == 0 || line[0] == '#') { // skip blank and comment lines
            continue;
        }
        if (data_line_idx == 0) {
            sscanf(buffer, "%hi %hi %hi %hi %hi",
                &usc->hist_bounds[0], &usc->hist_bounds[1], &usc->hist_bounds[2],
                &usc->hist_bounds[3], &usc->hist_bounds[4]);
            printf("parsed bounds: %d %d %d %d %d\n",
                usc->hist_bounds[0], usc->hist_bounds[1], usc->hist_bounds[2],
                usc->hist_bounds[3], usc->hist_bounds[4]);
        } else {
            struct dis_denh_user_scene *s = &usc->scenes[data_line_idx - 1];
            float vec_f[DIS_HIST_VEC_N];
            float cont_f = -1;
            int n;
            sscanf(line, "%f %f %f %f %f | %f %hd %hd | %n",
                &vec_f[0], &vec_f[1], &vec_f[2], &vec_f[3], &vec_f[4],
                &cont_f, &s->pivot, &s->blk_level, &n);
            printf("  %5.3f,  %5.3f,  %5.3f,  %5.3f,  %5.3f | %5.3f,  %3d,  %3d | %s\n",
                vec_f[0], vec_f[1], vec_f[2], vec_f[3], vec_f[4],
                cont_f, s->pivot, s->blk_level, line+n);
            for (int i = 0; i < DIS_HIST_VEC_N; ++i) {
                s->vec[i] = DIS_Q_FROM_FLOAT(vec_f[i], DIS_DENH_HIST_VEC_QP);
            }
            s->contrast = DIS_DENH_CONTRAST(cont_f);
            strncpy(s->desc, line + n, sizeof(s->desc));
            if (++usc->n_scenes >= DIS_DENH_MAX_N_SCENES) {
                // reach maximum capacity, read no more
                break;
            }
        }
        ++data_line_idx;
    }
}

void print_usage(void) {
    printf("usage: dyn_cont_scene [-s contrast scale] [scene define file]\n");
}

int dyn_cont_scene_main(int argc, char **argv)
{
    // parsing command-line
    if (argc == 1) {
        print_usage();
        return 0;
    }
    const char *scene_filepath = NULL;
    int16_t contrast_scale = DIS_DENH_CONT_SCALE(1.0);
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            const char opt_ch = argv[i][1];
            const char *opt_arg = i + 1 < argc ? argv[i + 1] : NULL;
            if (opt_ch == 's') {
                if (opt_arg == NULL) {
                    printf("-s requires an argument\n");
                    return 0;
                }
                contrast_scale = DIS_DENH_CONT_SCALE(atof(opt_arg));
                ++i; // consumed one argv
            } else {
                printf("unknown option -%c\n", opt_ch);
                return 0;
            }
        } else {
            if (scene_filepath) {
                printf("load_dcs accepts only one scene file.");
                return 0;
            }
            scene_filepath = argv[i];
        }
    }

    // load user scenes
    struct dis_dyn_enh_user_scenes scenes;
    scenes.distype = DIS_TYPE_HD;
    scenes.contrast_scale = contrast_scale;
    if (scene_filepath) {
        FILE *fp = fopen(scene_filepath, "r");
        if (fp == NULL) {
            printf("cannot open file %s\n", scene_filepath);
            return 0;
        }
        read_user_scenes(fp, &scenes);
        fclose(fp);
    } else {
        printf("Loading default scenes\n");
        // to use default scenes
        memset(scenes.hist_bounds, 0, sizeof(scenes.hist_bounds));
        scenes.n_scenes = 0;
    }

    // program thru ioctl
    const char *dis_path = "/dev/dis";
    int dis_fd = open(dis_path, O_RDWR);
    if (dis_fd == -1) {
        printf("Failed to open %s!\n", dis_path);
        return 0;
    }
    int ret = ioctl(dis_fd, DIS_DENH_SET_USER_SCENES, &scenes);
    if (scene_filepath) {
        printf("Programming %d user scenes %s.\n",
            scenes.n_scenes, ret == 0 ? "succeed" : "failed");
    } else {
        printf("Programming default scenes %s.\n",
            ret == 0 ? "succeed" : "failed");
    }
    close(dis_fd);
    return 0;
}

CONSOLE_CMD(dis, NULL, dis_main, CONSOLE_CMD_MODE_SELF, "Display debugging tool")
// temporary dcs command to program dyn-contrast scene
CONSOLE_CMD(load_dcs, NULL, dyn_cont_scene_main, CONSOLE_CMD_MODE_SELF, "Dynamic contrast scene loading tool")
