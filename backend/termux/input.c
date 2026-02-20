/*
 * Termux input: read lorieEvent from conn_fd (libtermux-render get_conn_fd),
 * dispatch to wlr_pointer and wlr_touch. Event layout matches termux-display-client
 * include/render.h lorieEvent union (EVENT_MOUSE, EVENT_TOUCH).
 */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wlr/interfaces/wlr_pointer.h>
#include <wlr/interfaces/wlr_touch.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_touch.h>
#include <wlr/util/log.h>

#include "backend/termux.h"
#include "termux_render.h"
#include "util/time.h"

/* Match termux-display-client include/render.h eventType enum */
#define LORIE_EVENT_TOUCH  6
#define LORIE_EVENT_MOUSE  7

/* Layout matches lorieEvent.touch / .mouse in render.h (no Android deps here) */
typedef struct {
	uint8_t t;
	uint8_t _pad; /* alignment for uint16_t */
	uint16_t type; /* Android MotionEvent: 0=DOWN, 1=UP, 2=MOVE */
	uint16_t id;
	uint16_t x;
	uint16_t y;
} lorie_touch_ev;

typedef struct {
	uint8_t t;
	uint8_t _pad[3];
	float x;
	float y;
	uint8_t detail; /* which_button: 1=left, 2=right, 3=middle */
	uint8_t down;
	uint8_t relative;
} lorie_mouse_ev;

#define LORIE_EVENT_SIZE 32

static const struct wlr_pointer_impl termux_pointer_impl = {
	.name = "termux-pointer",
};

static const struct wlr_touch_impl termux_touch_impl = {
	.name = "termux-touch",
};

/* Wayland/Linux button codes */
static uint32_t lorie_button_to_linux(uint8_t detail) {
	switch (detail) {
	case 1: return 272; /* BTN_LEFT */
	case 2: return 273; /* BTN_RIGHT */
	case 3: return 274; /* BTN_MIDDLE */
	default: return (uint32_t)detail;
	}
}

static struct wlr_termux_output *termux_backend_first_output(struct wlr_termux_backend *backend) {
	struct wlr_termux_output *out;
	wl_list_for_each(out, &backend->outputs, link) {
		return out;
	}
	return NULL;
}

static void handle_lorie_mouse(struct wlr_termux_backend *backend,
		const lorie_mouse_ev *ev) {
	if (!backend->pointer) {
		return;
	}
	struct wlr_pointer *pointer = &backend->pointer->wlr_pointer;
	struct wlr_termux_output *out = termux_backend_first_output(backend);
	double nx = 0.5, ny = 0.5;
	if (out && out->wlr_output.width > 0 && out->wlr_output.height > 0) {
		nx = (double)ev->x / (double)out->wlr_output.width;
		ny = (double)ev->y / (double)out->wlr_output.height;
		if (nx < 0.0) nx = 0.0;
		if (nx > 1.0) nx = 1.0;
		if (ny < 0.0) ny = 0.0;
		if (ny > 1.0) ny = 1.0;
	}
	uint32_t time_msec = (uint32_t)get_current_time_msec();

	if (ev->relative) {
		struct wlr_pointer_motion_event motion = {
			.pointer = pointer,
			.time_msec = time_msec,
			.delta_x = (double)ev->x,
			.delta_y = (double)ev->y,
			.unaccel_dx = (double)ev->x,
			.unaccel_dy = (double)ev->y,
		};
		wl_signal_emit_mutable(&pointer->events.motion, &motion);
	} else {
		struct wlr_pointer_motion_absolute_event abs = {
			.pointer = pointer,
			.time_msec = time_msec,
			.x = nx,
			.y = ny,
		};
		wl_signal_emit_mutable(&pointer->events.motion_absolute, &abs);
	}

	if (ev->detail != 0) {
		uint32_t button = lorie_button_to_linux(ev->detail);
		enum wl_pointer_button_state state = ev->down ?
			WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED;
		struct wlr_pointer_button_event btn = {
			.pointer = pointer,
			.time_msec = time_msec,
			.button = button,
			.state = state,
		};
		wlr_pointer_notify_button(pointer, &btn);
	}

	wl_signal_emit_mutable(&pointer->events.frame, pointer);
}

static void handle_lorie_touch(struct wlr_termux_backend *backend,
		const lorie_touch_ev *ev, uint32_t time_msec) {
	if (!backend->touch) {
		return;
	}
	struct wlr_touch *touch = &backend->touch->wlr_touch;
	struct wlr_termux_output *out = termux_backend_first_output(backend);
	double nx = 0.0, ny = 0.0;
	if (out && out->wlr_output.width > 0 && out->wlr_output.height > 0) {
		nx = (double)ev->x / (double)out->wlr_output.width;
		ny = (double)ev->y / (double)out->wlr_output.height;
		if (nx < 0.0) nx = 0.0;
		if (nx > 1.0) nx = 1.0;
		if (ny < 0.0) ny = 0.0;
		if (ny > 1.0) ny = 1.0;
	}

	switch (ev->type) {
	case 0: { /* ACTION_DOWN */
		struct wlr_touch_down_event down = {
			.touch = touch,
			.time_msec = time_msec,
			.touch_id = (int32_t)ev->id,
			.x = nx,
			.y = ny,
		};
		wl_signal_emit_mutable(&touch->events.down, &down);
		break;
	}
	case 1: { /* ACTION_UP */
		struct wlr_touch_up_event up = {
			.touch = touch,
			.time_msec = time_msec,
			.touch_id = (int32_t)ev->id,
		};
		wl_signal_emit_mutable(&touch->events.up, &up);
		break;
	}
	case 2: { /* ACTION_MOVE */
		struct wlr_touch_motion_event motion = {
			.touch = touch,
			.time_msec = time_msec,
			.touch_id = (int32_t)ev->id,
			.x = nx,
			.y = ny,
		};
		wl_signal_emit_mutable(&touch->events.motion, &motion);
		break;
	}
	default:
		break;
	}
	wl_signal_emit_mutable(&touch->events.frame, NULL);
}

static int termux_input_readable(int fd, uint32_t mask, void *data) {
	struct wlr_termux_backend *backend = data;
	uint8_t buf[LORIE_EVENT_SIZE];
	ssize_t n = read(fd, buf, sizeof(buf));
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}
		wlr_log(WLR_ERROR, "termux: read conn_fd: %s", strerror(errno));
		return 0;
	}
	if (n < 1) {
		return 0;
	}
	uint8_t type = buf[0];
	uint32_t time_msec = (uint32_t)get_current_time_msec();

	if (type == LORIE_EVENT_MOUSE && n >= (ssize_t)sizeof(lorie_mouse_ev)) {
		handle_lorie_mouse(backend, (const lorie_mouse_ev *)buf);
	} else if (type == LORIE_EVENT_TOUCH && n >= (ssize_t)sizeof(lorie_touch_ev)) {
		handle_lorie_touch(backend, (const lorie_touch_ev *)buf, time_msec);
	}
	return 0;
}

void termux_input_create_devices(struct wlr_termux_backend *backend) {
	int conn_fd = termux_render_get_conn_fd();
	if (conn_fd < 0) {
		wlr_log(WLR_DEBUG, "termux: no conn_fd for input");
		return;
	}

	struct wlr_termux_output *out = termux_backend_first_output(backend);
	const char *output_name = out ? out->wlr_output.name : "TERMUX-1";

	backend->pointer = calloc(1, sizeof(*backend->pointer));
	if (!backend->pointer) {
		wlr_log(WLR_ERROR, "termux: failed to allocate pointer");
		return;
	}
	wlr_pointer_init(&backend->pointer->wlr_pointer, &termux_pointer_impl, "termux-pointer");
	backend->pointer->wlr_pointer.output_name = strdup(output_name);
	backend->pointer->backend = backend;
	wl_signal_emit_mutable(&backend->backend.events.new_input, &backend->pointer->wlr_pointer.base);

	backend->touch = calloc(1, sizeof(*backend->touch));
	if (!backend->touch) {
		wlr_log(WLR_ERROR, "termux: failed to allocate touch");
		wlr_pointer_finish(&backend->pointer->wlr_pointer);
		free(backend->pointer->wlr_pointer.output_name);
		free(backend->pointer);
		backend->pointer = NULL;
		return;
	}
	wlr_touch_init(&backend->touch->wlr_touch, &termux_touch_impl, "termux-touch");
	backend->touch->wlr_touch.output_name = strdup(output_name);
	backend->touch->backend = backend;
	wl_signal_emit_mutable(&backend->backend.events.new_input, &backend->touch->wlr_touch.base);

	backend->input_event = wl_event_loop_add_fd(backend->event_loop, conn_fd,
		WL_EVENT_READABLE, termux_input_readable, backend);
	if (!backend->input_event) {
		wlr_log(WLR_ERROR, "termux: failed to add conn_fd to event loop");
		wlr_touch_finish(&backend->touch->wlr_touch);
		free(backend->touch->wlr_touch.output_name);
		free(backend->touch);
		backend->touch = NULL;
		wlr_pointer_finish(&backend->pointer->wlr_pointer);
		free(backend->pointer->wlr_pointer.output_name);
		free(backend->pointer);
		backend->pointer = NULL;
		return;
	}
	wlr_log(WLR_INFO, "termux: input devices and conn_fd listener added");
}

void termux_input_destroy(struct wlr_termux_backend *backend) {
	if (backend->input_event) {
		wl_event_source_remove(backend->input_event);
		backend->input_event = NULL;
	}
	if (backend->pointer) {
		wlr_pointer_finish(&backend->pointer->wlr_pointer);
		free(backend->pointer->wlr_pointer.output_name);
		free(backend->pointer);
		backend->pointer = NULL;
	}
	if (backend->touch) {
		wlr_touch_finish(&backend->touch->wlr_touch);
		free(backend->touch->wlr_touch.output_name);
		free(backend->touch);
		backend->touch = NULL;
	}
}
