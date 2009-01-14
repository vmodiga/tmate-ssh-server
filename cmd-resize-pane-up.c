/* $Id: cmd-resize-pane-up.c,v 1.2 2009-01-14 19:29:32 nicm Exp $ */

/*
 * Copyright (c) 2009 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <stdlib.h>

#include "tmux.h"

/*
 * Increase pane size.
 */

void	cmd_resize_pane_up_exec(struct cmd *, struct cmd_ctx *);

const struct cmd_entry cmd_resize_pane_up_entry = {
	"resize-pane-up", "resizep-up",
	CMD_PANE_WINDOW_USAGE " [adjustment]",
	CMD_ZEROONEARG,
	cmd_pane_init,
	cmd_pane_parse,
	cmd_resize_pane_up_exec,
       	cmd_pane_send,
	cmd_pane_recv,
	cmd_pane_free,
	cmd_pane_print
};

void
cmd_resize_pane_up_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct cmd_pane_data	*data = self->data;
	struct winlink		*wl;
	const char	       	*errstr;
	struct window_pane	*wp, *wq;
	u_int			 adjust;
	
	if ((wl = cmd_find_window(ctx, data->target, NULL)) == NULL)
		return;
	if (data->pane == -1)
		wp = wl->window->active;
	else {
		wp = window_pane_at_index(wl->window, data->pane);
		if (wp == NULL) {
			ctx->error(ctx, "no pane: %d", data->pane);
			return;
		}
	}

	if (data->arg == NULL)
		adjust = 1;
	else {
		adjust = strtonum(data->arg, 1, INT_MAX, &errstr);
		if (errstr != NULL) {
			ctx->error(ctx, "adjustment %s: %s", errstr, data->arg);
			return;
		}
	}

	/*
	 * If this is not the last window, keep trying to reduce size and add
	 * to the following window. If it is the last, do so on the previous
	 * window.
	 */
	wq = TAILQ_NEXT(wp, entry);
	if (wq == NULL) {
		if (wp == TAILQ_FIRST(&wl->window->panes)) {
			/* Only one pane. */
			goto out;
		}
		wq = wp;
		wp = TAILQ_PREV(wq, window_panes, entry);
	}
	while (adjust-- > 0) {
		if (wp->sy <= PANE_MINIMUM)
			break;
		window_pane_resize(wq, wq->sx, wq->sy + 1);
		window_pane_resize(wp, wp->sx, wp->sy - 1);
	}
	window_update_panes(wl->window);

	server_redraw_window(wl->window);

out:
	if (ctx->cmdclient != NULL)
		server_write_client(ctx->cmdclient, MSG_EXIT, NULL, 0);
}
