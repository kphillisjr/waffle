// Copyright 2012 Intel Corporation
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// @file
/// @brief TODO

#define PROG_NAME "cross-api"

#define _POSIX_C_SOURCE 199309L // glibc feature macro for nanosleep.

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "waffle.h"

static const char *usage_message =
    "usage:\n"
    "    " PROG_NAME " <platform> <draw_api> <read_api>"
    "\n"
    "arguments:\n"
    "    platform: One of android, gbm, glx, wayland, x11_egl.\n"
    "    draw_api, read_api: One of gl, gles1, gles2, gles3.\n"
    "\n"
    "example:\n"
    "    " PROG_NAME " x11_egl gl gles1\n"
    "\n"
    "description:\n"
    "    Create a window and two contexts: a \"draw context\" that  uses\n"
    "    `draw_api` and a \"read context\" that uses `read_api`. With the\n"
    "    \"draw context\" current, fill the window with red. Then, with the \n"
    "    \"read context\" current, verify that the window's pixels are red.\n"
    "    Repeat with green and blue.\n"
    ;

static void
error_printf(const char *fmt, ...)
{
    va_list ap;

    fflush(stdout);

    va_start(ap, fmt);
    fprintf(stderr, "cross-api: error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(EXIT_FAILURE);
}
static void
error_usage(void)
{
    fprintf(stderr, "usage error\n\n");
    fprintf(stderr, "%s", usage_message);
    exit(EXIT_FAILURE);
}


static void
error_waffle(void)
{
    const struct waffle_error_info *info = waffle_error_get_info();
    const char *code = waffle_error_to_string(info->code);

    if (info->message_length > 0)
        error_printf("%s: %s", code, info->message);
    else
        error_printf("%s", code);
}

static void
error_get_gl_symbol(const char *name)
{
    error_printf("failed to get function pointer for %s", name);
}

typedef float GLclampf;
typedef unsigned int GLbitfield;
typedef unsigned int GLint;
typedef int GLsizei;
typedef unsigned int GLenum;
typedef void GLvoid;

enum {
    // Copied for <GL/gl.h>.
    GL_UNSIGNED_BYTE =    0x00001401,
    GL_RGBA =             0x00001908,
    GL_COLOR_BUFFER_BIT = 0x00004000,
};

#define WINDOW_WIDTH  320
#define WINDOW_HEIGHT 240

static void (*glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static void (*glClear)(GLbitfield mask);
static void (*glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, GLvoid* data);

struct options {
    /// @brief One of `WAFFLE_PLATFORM_*`.
    int platform;

    /// @brief One of `WAFFLE_CONTEXT_OPENGL_*`.
    int draw_api;

    /// @brief One of `WAFFLE_CONTEXT_OPENGL_*`.
    int read_api;

    /// @brief One of `WAFFLE_DL_*`.
    int draw_dl;

    /// @brief One of `WAFFLE_DL_*`.
    int read_dl;
};

struct enum_map {
    int i;
    const char *s;
};

static const struct enum_map platform_map[] = {
    {WAFFLE_PLATFORM_ANDROID,   "android"       },
    {WAFFLE_PLATFORM_GBM,       "gbm"           },
    {WAFFLE_PLATFORM_GLX,       "glx"           },
    {WAFFLE_PLATFORM_WAYLAND,   "wayland"       },
    {WAFFLE_PLATFORM_X11_EGL,   "x11_egl"       },
    {0,                         0               },
};

static const struct enum_map context_api_map[] = {
    {WAFFLE_CONTEXT_OPENGL,         "gl"        },
    {WAFFLE_CONTEXT_OPENGL_ES1,     "gles1"     },
    {WAFFLE_CONTEXT_OPENGL_ES2,     "gles2"     },
    {WAFFLE_CONTEXT_OPENGL_ES3,     "gles3"     },
    {0,                             0           },
};

/// @brief Translate string to `enum waffle_enum`.
///
/// @param self is a list of map items. The last item must be zero-filled.
/// @param result is altered only if @a s if found.
/// @return true if @a s was found in @a map.
static bool
enum_map_translate_str(
        const struct enum_map *self,
        const char *s,
        int *result)
{
    const struct enum_map *i;

    for (i = self; i->i != 0; ++i) {
        if (!strncmp(s, i->s, strlen(i->s) + 1)) {
            *result = i->i;
            return true;
        }
    }

    return false;
}

static void
parse_api_arg(const char *arg,
              int32_t *api,
              int32_t *dl)
{
    bool ok;

    ok = enum_map_translate_str(context_api_map, arg, api);
    if (!ok) {
        fprintf(stderr, "error: '%s' is not a valid API for a GL context\n", arg);
        error_usage();
    }

    switch (*api) {
        case WAFFLE_CONTEXT_OPENGL:
            *dl = WAFFLE_DL_OPENGL;
            break;
        case WAFFLE_CONTEXT_OPENGL_ES1:
            *dl = WAFFLE_DL_OPENGL_ES1;
            break;
        case WAFFLE_CONTEXT_OPENGL_ES2:
        case WAFFLE_CONTEXT_OPENGL_ES3:
            *dl = WAFFLE_DL_OPENGL_ES2;
            break;
        default:
            abort();
            break;
    }
}
/// @return true on success.
static bool
parse_args(int argc, char *argv[], struct options *opts)
{
    const char *arg;
    bool ok;

    if (argc != 4)
        error_usage();

    // Parse platform.
    arg = argv[1];
    ok = enum_map_translate_str(platform_map, arg, &opts->platform);
    if (!ok) {
        fprintf(stderr, "error: '%s' is not a valid platform\n", arg);
        error_usage();
    }

    parse_api_arg(argv[2], &opts->draw_api, &opts->draw_dl);
    parse_api_arg(argv[3], &opts->read_api, &opts->read_dl);

    return true;
}

static bool
draw_read_loop(struct waffle_display *dpy,
               struct waffle_window *window,
               struct waffle_context *draw_ctx,
               struct waffle_context *read_ctx)
{
    int i, j;
    bool ok;
    unsigned char *colors;

    static const struct timespec sleep_time = {
         // 0.5 sec
        .tv_sec = 0,
        .tv_nsec = 500000000,
    };

    for (i = 0; i < 3; ++i) {
        ok = waffle_make_current(dpy, window, draw_ctx);
        if (!ok)
            error_waffle();

        switch (i) {
            case 0: glClearColor(1, 0, 0, 1); break;
            case 1: glClearColor(0, 1, 0, 1); break;
            case 2: glClearColor(0, 0, 1, 1); break;
            case 3: abort(); break;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        ok = waffle_make_current(dpy, NULL, NULL);
        if (!ok)
            error_waffle();

        ok = waffle_make_current(dpy, window, read_ctx);
        if (!ok)
            error_waffle();

        colors = calloc(WINDOW_WIDTH * WINDOW_HEIGHT * 4, sizeof(*colors));
        glReadPixels(0, 0,
                     WINDOW_WIDTH, WINDOW_HEIGHT,
                     GL_RGBA, GL_UNSIGNED_BYTE,
                     colors);
        for (j = 0; j < WINDOW_WIDTH * WINDOW_HEIGHT * 4; j += 4) {
           if ((colors[j]   != (i == 0 ? 0xff : 0)) ||
               (colors[j+1] != (i == 1 ? 0xff : 0)) ||
               (colors[j+2] != (i == 2 ? 0xff : 0)) ||
               (colors[j+3] != 0xff)) {
              free(colors);
              error_printf("glReadPixels returned unexpected result");
           }
        }
        free(colors);

        ok = waffle_window_swap_buffers(window);
        if (!ok)
            return error_waffle;

        ok = waffle_make_current(dpy, NULL, NULL);
        if (!ok)
            error_waffle();

        nanosleep(&sleep_time, NULL);
    }

    return true;
}

static struct waffle_config*
make_config(struct waffle_display *dpy,
            int32_t context_api)
{
    struct waffle_config *config;
    int32_t attrib_list[11];
    int i;

    i = 0;
    attrib_list[i++] = WAFFLE_CONTEXT_API;
    attrib_list[i++] = context_api;
    attrib_list[i++] = WAFFLE_RED_SIZE;
    attrib_list[i++] = 8;
    attrib_list[i++] = WAFFLE_GREEN_SIZE;
    attrib_list[i++] = 8;
    attrib_list[i++] = WAFFLE_BLUE_SIZE;
    attrib_list[i++] = 8;
    attrib_list[i++] = WAFFLE_DOUBLE_BUFFERED;
    attrib_list[i++] = true;
    attrib_list[i++] = 0;

    config = waffle_config_choose(dpy, attrib_list);
    if (!config)
        error_waffle();

    return config;
}

int
main(int argc, char **argv)
{
    bool ok;
    int i;

    struct options opts;

    int32_t init_attrib_list[3];

    struct waffle_display *dpy;
    struct waffle_config *draw_config;
    struct waffle_config *read_config;
    struct waffle_context *draw_ctx;
    struct waffle_context *read_ctx;
    struct waffle_window *window;

    ok = parse_args(argc, argv, &opts);
    if (!ok)
        exit(EXIT_FAILURE);

    i = 0;
    init_attrib_list[i++] = WAFFLE_PLATFORM;
    init_attrib_list[i++] = opts.platform;
    init_attrib_list[i++] = WAFFLE_NONE;

    ok = waffle_init(init_attrib_list);
    if (!ok)
        error_waffle();

    dpy = waffle_display_connect(NULL);
    if (!dpy)
        error_waffle();

    if (!waffle_display_supports_context_api(dpy, opts.draw_api)) {
        error_printf("Display does not support %s",
                     waffle_enum_to_string(opts.draw_api));
    }

    if (!waffle_display_supports_context_api(dpy, opts.read_api)) {
        error_printf("Display does not support %s",
                     waffle_enum_to_string(opts.read_api));
    }

    glClear = waffle_dl_sym(opts.draw_dl, "glClear");
    if (!glClear)
        error_get_gl_symbol("glClear");

    glClearColor = waffle_dl_sym(opts.draw_dl, "glClearColor");
    if (!glClearColor)
        error_get_gl_symbol("glClearColor");

    glReadPixels = waffle_dl_sym(opts.read_dl, "glReadPixels");
    if (!glReadPixels)
        error_get_gl_symbol("glReadPixels");


    draw_config = make_config(dpy, opts.draw_api);
    read_config = make_config(dpy, opts.read_api);

    draw_ctx = waffle_context_create(draw_config, NULL);
    if (!draw_ctx)
        error_waffle();

    read_ctx = waffle_context_create(read_config, NULL);
    if (!read_ctx)
        error_waffle();

    window = waffle_window_create(draw_config, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!window)
        error_waffle();

    ok = waffle_window_show(window);
    if (!ok)
        error_waffle();

    ok = draw_read_loop(dpy, window, draw_ctx, read_ctx);
    if (!ok)
        error_waffle();

    ok = waffle_window_destroy(window);
    if (!ok)
        error_waffle();

    ok = waffle_context_destroy(draw_ctx);
    if (!ok)
        error_waffle();

    ok = waffle_context_destroy(read_ctx);
    if (!ok)
        error_waffle();

    ok = waffle_config_destroy(draw_config);
    if (!ok)
        error_waffle();

    ok = waffle_config_destroy(read_config);
    if (!ok)
        error_waffle();

    ok = waffle_display_disconnect(dpy);
    if (!ok)
        error_waffle();

    printf("gl_basic: run was successful\n");
    return EXIT_SUCCESS;
}
