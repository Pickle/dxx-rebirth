/*
 *  messagebox.c
 *  d1x-rebirth
 *
 *  Display an error or warning messagebox using the OS's window server.
 *
 */

#include "window.h"
#include "event.h"
#include "messagebox.h"

static void display_linux_alert(const char *message, int error)
{
	d_event	event;
	window	*wind;

	// Handle Descent's windows properly
	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);

	// TODO: insert messagebox code...

	if ((wind = window_get_front()))
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
}

void msgbox_warning(const char *message)
{
	display_linux_alert(message, 0);
}

void msgbox_error(const char *message)
{
	display_linux_alert(message, 1);
}
