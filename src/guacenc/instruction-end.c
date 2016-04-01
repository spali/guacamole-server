/*
 * Copyright (C) 2016 Glyptodon, Inc.
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
#include "display.h"
#include "image-stream.h"
#include "log.h"

#include <guacamole/client.h>

#include <stdlib.h>

int guacenc_handle_end(guacenc_display* display, int argc, char** argv) {

    /* Verify argument count */
    if (argc < 1) {
        guacenc_log(GUAC_LOG_WARNING, "\"end\" instruction incomplete");
        return 1;
    }

    /* Parse arguments */
    int index = atoi(argv[0]);

    /* Retrieve image stream */
    guacenc_image_stream* stream =
        guacenc_display_get_image_stream(display, index);
    if (stream == NULL)
        return 1;

    /* Retrieve destination buffer */
    guacenc_buffer* buffer =
        guacenc_display_get_related_buffer(display, stream->index);
    if (buffer == NULL)
        return 1;

    /* End image stream, drawing final image to the buffer */
    return guacenc_image_stream_end(stream, buffer);

}

