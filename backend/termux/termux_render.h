/*
 * Thin wrapper around libtermux-render (termux-render.so).
 * Uses setScreenConfig, connectToRender, lorieBuffer, serverState, stopEventLoop.
 */
#ifndef WLR_TERMUX_RENDER_H
#define WLR_TERMUX_RENDER_H

/**
 * Connect to termux render server. Must be called before push_frame.
 * width/height/refresh used for setScreenConfig; refresh can be 0 for default.
 * Returns 0 on success.
 */
int termux_render_connect(int width, int height, int refresh);

/**
 * Disconnect and release resources (calls stopEventLoop).
 */
void termux_render_disconnect(void);

/**
 * Copy pixel data to the shared buffer and signal the server to refresh.
 * data: RGBA or RGBX, stride_bytes: bytes per row (0 = width*4).
 * Returns 0 on success.
 */
int termux_render_push_frame(const void *data, size_t stride_bytes);

/**
 * Get current buffer dimensions (from LorieBuffer_description). Optional.
 */
void termux_render_get_size(int *width, int *height);

/** True after a successful connect. */
bool termux_render_connected(void);

/**
 * Fd to read input events (mouse, touch, etc.) from; -1 if not connected.
 * Only valid after termux_render_connect() and when the display client has
 * sent the event fd (see get_conn_fd in libtermux-render).
 */
int termux_render_get_conn_fd(void);

#endif
