#ifndef BACKEND_TERMUX_H
#define BACKEND_TERMUX_H

#include <wlr/backend/termux.h>
#include <wlr/backend/interface.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_touch.h>

struct wlr_termux_backend {
	struct wlr_backend backend;
	struct wl_event_loop *event_loop;
	struct wl_list outputs;
	struct wl_listener event_loop_destroy;
	bool started;
	char *socket_path;

	struct wl_event_source *input_event;
	struct wlr_termux_pointer *pointer;
	struct wlr_termux_touch *touch;
};

struct wlr_termux_output {
	struct wlr_output wlr_output;
	struct wlr_termux_backend *backend;
	struct wl_list link;
};

struct wlr_termux_pointer {
	struct wlr_pointer wlr_pointer;
	struct wlr_termux_backend *backend;
};

struct wlr_termux_touch {
	struct wlr_touch wlr_touch;
	struct wlr_termux_backend *backend;
};

struct wlr_termux_backend *termux_backend_from_backend(struct wlr_backend *wlr_backend);

void termux_input_create_devices(struct wlr_termux_backend *backend);
void termux_input_destroy(struct wlr_termux_backend *backend);

#endif
