/*
 * Input and HID logic
 * Lubomir Rintel <lkundrak@v3.sk>
 * License: GPL
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <linux/input.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hidp.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "btkbdd.h"
#include "hid.h"
#include "linux2hid.h"

/* A packet we're sending to host after a keypress */
struct key_report {
	uint8_t type;
	uint8_t report;
	uint8_t mods;
	uint8_t reserved;
	uint8_t key[6];
} __attribute__((packed));

/* Keyboard status (keys pressed and LEDs lit) */
struct status {
	uint8_t leds;
	struct key_report report;
};

/* Update LEDs.
 * TODO: Only ones that changed -- we already keep global track. */
static int
set_leds (input, leds)
	int input;
	uint8_t leds;
{
	struct input_event event;
	event.type = EV_LED;

	event.code = LED_NUML;
	event.value = !!(leds & HIDP_NUML);
	if (write (input, &event, sizeof(event)) != sizeof(event))
		return -1;

	event.code = LED_CAPSL;
	event.value = !!(leds & HIDP_CAPSL);
	if (write (input, &event, sizeof(event)) != sizeof(event))
		return -1;

	event.code = LED_SCROLLL;
	event.value = !!(leds & HIDP_SCROLLL);
	if (write (input, &event, sizeof(event)) != sizeof(event))
		return -1;

	event.code = LED_COMPOSE;
	event.value = !!(leds & HIDP_COMPOSE);
	if (write (input, &event, sizeof(event)) != sizeof(event))
		return -1;

	event.code = LED_KANA;
	event.value = !!(leds & HIDP_KANA);
	if (write (input, &event, sizeof(event)) != sizeof(event))
		return -1;

	return 0;
}

/* Read and process a command from given descriptor */
static int
btooth_command (status, fd, input)
	struct status *status;
	int fd;
	int input;
{
	uint8_t buf[HIDP_DEFAULT_MTU];
	int size;
	uint8_t handshake = HIDP_TRANS_HANDSHAKE;

	size = read (fd, &buf, sizeof(buf));
	switch (size) {
	case -1:
		perror ("Failure reading from bluetooth socket");
		return -1;
	case 0:
		DBG("Short read. Remote disconnected?\n");
		return -1;
	}

	switch (buf[0] & HIDP_HEADER_TRANS_MASK) {
	case HIDP_TRANS_SET_PROTOCOL:
		/* Acknowledge anything -- both protocols have
		 * the same descriptors for us */
		handshake |= HIDP_HSHK_SUCCESSFUL;
		if (write (fd, &handshake, 1) != 1) {
			perror ("Could not reply with handshake.");
			return -1;
		}
		break;
	case HIDP_TRANS_DATA:
		/* Apple (iPad) seemingly randomly sends either
		 * "a2 01 xx" or "a2 xx" when setting LEDs... */
		if (size == 3 || size == 2) {
			set_leds (input, buf[size - 1]);
			break;
		}
	default:
		handshake |= HIDP_HSHK_ERR_UNKNOWN;
		if (write (fd, &handshake, 1) != 1) {
			perror ("Could not reply with handshake.");
			return -1;
		}

#ifdef DEBUG
		int i;
		fprintf (stderr, "Not understood: ");
		for (i = 0; i < size; i++)
			fprintf (stderr, "%02x ",buf[i]);
		fprintf (stderr, "\n");
#endif
	}

	return 0;
}

/* Process an evdev event */
static int
input_event (status, input, ctrl, intr)
	struct status *status;
	int input;
	int ctrl;
	int intr;
{
	struct input_event event;
	int mod = 0;

	switch (read (input, &event, sizeof(event))) {
	case -1:
		perror ("Error reading from event device");
		return -1;
	case sizeof(event):
		break;
	default:
		fprintf (stderr, "Badly sized read from event device.\n");
		return -1;
	}

	if (event.type != EV_KEY)
		return 0;

	/* We're just a poor 101-key keyboard. */
	if (event.code >= 256) {
		DBG("Ignored code 0x%x > 0xff.\n", event.code);
		return 0;
	}

	/* Apply modifiers. "Left/RightGUI is Windows / Command / Meta" */
	switch (event.code) {
	case KEY_LEFTCTRL: mod = HIDP_LEFTCTRL; break;
	case KEY_LEFTSHIFT: mod = HIDP_LEFTSHIFT; break;
	case KEY_LEFTALT: mod = HIDP_LEFTALT; break;
	case KEY_LEFTMETA: mod = HIDP_LEFTGUI; break;
	case KEY_RIGHTCTRL: mod = HIDP_RIGHTCTRL; break;
	case KEY_RIGHTSHIFT: mod = HIDP_RIGHTSHIFT; break;
	case KEY_RIGHTALT: mod = HIDP_RIGHTALT; break;
	case KEY_RIGHTMETA: mod = HIDP_RIGHTGUI; break;
	}

	if (mod) {
		/* If a modifier was (de)pressed, update the track... */
		if (event.value) {
			status->report.mods |= mod;
		} else {
			status->report.mods &= ~mod;
		}
	} else {
		/* ...otherwise update the array of keys pressed. */
		int i;
		int code = linux2hid[event.code];

		DBG("code %d value %d hid %d mods 0x%x\n",
			event.code, event.value, linux2hid[event.code], status->report.mods);

		for (i = 0; i < 6; i++) {
			/* Remove key if already enabled */
			if (status->report.key[i] == code)
				status->report.key[i] = 0;

			/* Add key if it was pushed */
			if (event.value && status->report.key[i] == 0) {
				status->report.key[i] = code;
				break;
			}

			/* Shift the rest. Probably not needed, but keyboards do that */
			if (i < 5 && status->report.key[i] == 0) {
				status->report.key[i] = status->report.key[i+1];
				status->report.key[i+1] = 0;
			}
		}
	}

#ifdef DEBUG
	int i;
	uint8_t *buf = (uint8_t *)&status->report;
	for (i = 0; i < sizeof(status->report); i++)
		fprintf (stderr, "%02x ", buf[i]);
	fprintf (stderr, "\n");
#endif

	return 1;
}

/* Initialize an evdev device. */
static int
input_open (dev)
	char *dev;
{
	int version;
	int features;
	int input;
	int norepeat[2] = { 0, 0 };

	input = open (dev, O_RDWR);
	if (input == -1) {
		perror (dev);
		return -1;
	}

	/* Check if we're running against the same API that we're compiled with */
	if (ioctl (input, EVIOCGVERSION, &version) == -1) {
		perror ("Could not read input protocol version");
		goto fail;
	}
	if (version >> 16 != EV_VERSION >> 16) {
		fprintf (stderr, "Bad input subsystem version");
		goto fail;
	}

	/* Ensure we're talking to a keyboard. TODO: Check for LED support. */
	if (ioctl (input, EVIOCGBIT(0, EV_MAX), &features) == -1) {
		perror ("Could query device for supported features");
		goto fail;
	}
	if (!(features & EV_KEY)) {
		/* Not a keyboard? */
		fprintf (stderr, "Device not capable of producing key press event.");
		goto fail;
	}

	/* Grab the keyboard, so that the events are not seen by X11 */
	if (ioctl (input, EVIOCGRAB, 1) == -1) {
		perror ("Could not grab keyboard for exclusive use");
		goto fail;
	}

	/* Host takes care of autorepeat itself */
	if (ioctl (input, EVIOCSREP, norepeat) == -1) {
		perror ("Could not disable autorepeat");
		goto fail;
	}

	return input;
fail:
	close (input);
	return -1;
}

/* Handshake with Apple crap */
static int
hello (control)
	int control;
{
	/* Apple disconnects immediately,
	 * if we don't send this within a second. */
	if (write (control, "\xa1\x13\x03", 3) != 3) {
		perror ("Could not send a handshake.");
		return -1;
	}
	if (write (control, "\xa1\x13\x02", 3) != 3) {
		perror ("Could not send a handshake.");
		return -1;
	}
	/* Apple is known to require a small delay,
	 * otherwise it eats the first character. */
	sleep (1);

	return 0;
}

/* Dispatch the work */
static int
session (src, tgt, input, sintr, scontrol)
	bdaddr_t src;
	bdaddr_t *tgt;
	int input, sintr, scontrol;
{
	int control = -1, intr = -1;	/* host sockets */
	struct status status;		/* keyboard state */
	struct pollfd pf[5];

	/* Initialize the keyboard state */
	status.report.type = HIDP_TRANS_DATA | HIDP_DATA_RTYPE_INPUT;
	status.report.report = 0x01;
	status.report.mods = 0;
	status.report.reserved = 0;
	status.report.key[0] = status.report.key[1] = status.report.key[2]
		= status.report.key[3] = status.report.key[4]
		= status.report.key[5] = 0;
	status.leds = 0;
	set_leds (input, status.leds);

	/* Watch out */
	pf[0].fd = input;
	pf[1].fd = control;
	pf[2].fd = intr;
	pf[3].fd = scontrol;
	pf[4].fd = sintr;
	pf[0].events = pf[1].events = pf[2].events = pf[3].events
		= pf[4].events = POLLIN | POLLERR | POLLHUP;

	while (poll (pf, 5, -1) > 0) {
		DBG("Entered main loop.\n");

		if (pf[0].revents) {
			int ret;

			/* An input event */
			pf[0].revents = 0;

			/* Read the keyboard event and update status */
			ret = input_event (&status, input, control, intr);
			if (ret == -1) {
				if (control != -1)
					close (control);
				if (intr != -1)
					close (intr);
				return 0;
			}
			if (ret == 0)
				continue;
			DBG("Input event.\n");

			/* Noone managed to connect to us so far.
			 * Try to reach out for a host ourselves. */
			if (control == -1) {
				/* Noone to talk to? */
				if (!bacmp (tgt, BDADDR_ANY))
					break;

				pf[1].fd = control = l2cap_connect (&src, tgt, L2CAP_PSM_HIDP_CTRL);
				if (control == -1)
					break;
				pf[2].fd = intr = l2cap_connect (&src, tgt, L2CAP_PSM_HIDP_INTR);
				if (intr == -1) {
					close (control);
					control = -1;
					break;
				}
				if (hello (control) == -1)
					break;
			}

			/* Send the packet to the host. */
			if (write (intr, &status.report, sizeof(status.report)) <= 0) {
				perror ("Could not send a packet to the host");
				break;
			}

		}
		if (pf[1].revents) {
			/* Control connection command */
			pf[1].revents = 0;
			DBG("Control command.\n");

			if (btooth_command (&status, control, input))
				break;
		}
		if (pf[2].revents) {
			/* Interrupt */
			pf[2].revents = 0;
			DBG("Interrupt.\n");

			if (btooth_command (&status, intr, input))
				break;
		}
		if (pf[3].revents) {
			/* A host is likely attempting to connect. */
			pf[3].revents = 0;
			DBG("Control server activity.\n");

			if (control != -1)
				close (control);
			pf[1].fd = control = l2cap_accept (scontrol, tgt);
			if (control == -1)
				break;
			pf[3].fd = scontrol = -1;
		}
		if (pf[4].revents) {
			pf[4].revents = 0;
			DBG("Interrupt server activity.\n");

			/* Control connection needs to be connected first */
			if (control == -1)
				break;

			if (intr != -1)
				close (intr);
			pf[2].fd = intr = l2cap_accept (sintr, NULL);
			if (intr == -1)
				break;
			hello (control);
			pf[4].fd = sintr = -1;
		}
	}

	if (control != -1)
		close (control);
	if (intr != -1)
		close (intr);

	return 1;
}

int
loop (device, src, tgt)
	char *device;
	bdaddr_t src;
	bdaddr_t *tgt;
{
	int sintr, scontrol;	/* server sockets */
	int input;		/* event device */
	int hci = -1;
	uint32_t save_class = 0;

	/* Open the input event device */
	input = input_open (device);
	if (input == -1)
		return 0;

	/* Prepare the server sockets, in case a client will connect. */
	sintr = l2cap_listen (&src, L2CAP_PSM_HIDP_INTR, 0, 1);
	if (sintr == -1) {
		close (input);
		return 0;
	}
	scontrol = l2cap_listen (&src, L2CAP_PSM_HIDP_CTRL, 0, 1);
	if (scontrol == -1) {
		close (input);
		close (sintr);
		return 0;
	}

	do {
		if (hci == -1) {
			if (bacmp (&src, BDADDR_ANY)) {
				char addr[18];

				ba2str (&src, addr);
				hci = hci_devid (addr);
				/* Not yet plugged in or visited by udev? */
				if (hci == -1)
					perror ("Can not initialize HCI device");
			} else {
				/* No source device specified. Assume first. */
				hci = 0;
			}
			if (hci >= 0) {
				if (!save_class)
					save_class = set_class (hci, 0x002540UL);
				/* Retry opening HCI on next ocassion */
				if (!save_class)
					hci = -1;
			}
			if (hci >= 0) {
				if (sdp_open () == 1)
					sdp_add_keyboard ();
			}
		}
	} while (session (src, tgt, input, sintr, scontrol));
	sdp_remove ();
	if (save_class)
		set_class (hci, save_class);

	close (input);
	close (sintr);
	close (scontrol);

	return 1;
}
