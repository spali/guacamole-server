/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include "client.h"
#include "rdp.h"
#include "rdp_disp.h"
#include "rdp_settings.h"

#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <guacamole/timestamp.h>

#ifdef HAVE_FREERDP_CLIENT_DISP_H
#include <freerdp/client/disp.h>
#endif

guac_rdp_disp* guac_rdp_disp_alloc() {

    guac_rdp_disp* disp = malloc(sizeof(guac_rdp_disp));

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    /* Not yet connected */
    disp->disp = NULL;
#endif

    /* No requests have been made */
    disp->last_request = guac_timestamp_current();
    disp->requested_width  = 0;
    disp->requested_height = 0;
    disp->reconnect_needed = 0;

    return disp;

}

void guac_rdp_disp_free(guac_rdp_disp* disp) {
    free(disp);
}

void guac_rdp_disp_load_plugin(rdpContext* context) {

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
#ifdef HAVE_RDPSETTINGS_SUPPORTDISPLAYCONTROL
    context->settings->SupportDisplayControl = TRUE;
#endif

    /* Add "disp" channel */
    ADDIN_ARGV* args = malloc(sizeof(ADDIN_ARGV));
    args->argc = 1;
    args->argv = malloc(sizeof(char**) * 1);
    args->argv[0] = strdup("disp");
    freerdp_dynamic_channel_collection_add(context->settings, args);
#endif

}

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
void guac_rdp_disp_connect(guac_rdp_disp* guac_disp, DispClientContext* disp) {
    guac_disp->disp = disp;
}
#endif

/**
 * Fits a given dimension within the allowed bounds for Display Update
 * messages, adjusting the other dimension such that aspect ratio is
 * maintained.
 *
 * @param a The dimension to fit within allowed bounds.
 *
 * @param b
 *     The other dimension to adjust if and only if necessary to preserve
 *     aspect ratio.
 */
static void guac_rdp_disp_fit(int* a, int* b) {

    int a_value = *a;
    int b_value = *b;

    /* Ensure first dimension is within allowed range */
    if (a_value < GUAC_RDP_DISP_MIN_SIZE) {

        /* Adjust other dimension to maintain aspect ratio */
        int adjusted_b = b_value * GUAC_RDP_DISP_MIN_SIZE / a_value;
        if (adjusted_b > GUAC_RDP_DISP_MAX_SIZE)
            adjusted_b = GUAC_RDP_DISP_MAX_SIZE;

        *a = GUAC_RDP_DISP_MIN_SIZE;
        *b = adjusted_b;

    }
    else if (a_value > GUAC_RDP_DISP_MAX_SIZE) {

        /* Adjust other dimension to maintain aspect ratio */
        int adjusted_b = b_value * GUAC_RDP_DISP_MAX_SIZE / a_value;
        if (adjusted_b < GUAC_RDP_DISP_MIN_SIZE)
            adjusted_b = GUAC_RDP_DISP_MIN_SIZE;

        *a = GUAC_RDP_DISP_MAX_SIZE;
        *b = adjusted_b;

    }

}

void guac_rdp_disp_set_size(guac_rdp_disp* disp, guac_rdp_settings* settings,
        freerdp* rdp_inst, int width, int height) {

    /* Fit width within bounds, adjusting height to maintain aspect ratio */
    guac_rdp_disp_fit(&width, &height);

    /* Fit height within bounds, adjusting width to maintain aspect ratio */
    guac_rdp_disp_fit(&height, &width);

    /* Width must be even */
    if (width % 2 == 1)
        width -= 1;

    /* Store deferred size */
    disp->requested_width = width;
    disp->requested_height = height;

    /* Send display update notification if possible */
    guac_rdp_disp_update_size(disp, settings, rdp_inst);

}

void guac_rdp_disp_update_size(guac_rdp_disp* disp,
        guac_rdp_settings* settings, freerdp* rdp_inst) {

    int width = disp->requested_width;
    int height = disp->requested_height;

    /* Do not update size if no requests have been received */
    if (width == 0 || height == 0)
        return;

    guac_timestamp now = guac_timestamp_current();

    /* Limit display update frequency */
    if (now - disp->last_request <= GUAC_RDP_DISP_UPDATE_INTERVAL)
        return;

    /* Do NOT send requests unless the size will change */
    if (rdp_inst != NULL
            && width == guac_rdp_get_width(rdp_inst)
            && height == guac_rdp_get_height(rdp_inst))
        return;

    disp->last_request = now;

    if (settings->resize_method == GUAC_RESIZE_RECONNECT) {

        /* Update settings with new dimensions */
        settings->width = width;
        settings->height = height;

        /* Signal reconnect */
        disp->reconnect_needed = 1;

    }

    else if (settings->resize_method == GUAC_RESIZE_DISPLAY_UPDATE) {
#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
        DISPLAY_CONTROL_MONITOR_LAYOUT monitors[1] = {{
            .Flags  = 0x1, /* DISPLAYCONTROL_MONITOR_PRIMARY */
            .Left = 0,
            .Top = 0,
            .Width  = width,
            .Height = height,
            .PhysicalWidth = 0,
            .PhysicalHeight = 0,
            .Orientation = 0,
            .DesktopScaleFactor = 0,
            .DeviceScaleFactor = 0
        }};

        /* Send display update notification if display channel is connected */
        if (disp->disp != NULL)
            disp->disp->SendMonitorLayout(disp->disp, 1, monitors);
#endif
    }

}

int guac_rdp_disp_reconnect_needed(guac_rdp_disp* disp) {
    return disp->reconnect_needed;
}

void guac_rdp_disp_reconnect_complete(guac_rdp_disp* disp) {
    disp->reconnect_needed = 0;
    disp->last_request = guac_timestamp_current();
}

