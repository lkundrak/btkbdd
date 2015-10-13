#include <linux/input.h>
#include <linux/uinput.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define UINPUT "/dev/uinput"

static int
open_uinput (const char *name)
{
	int fd;
	struct uinput_user_dev dev = { 0, };
	int i;

	fd = open (UINPUT, O_WRONLY | O_NDELAY);
	if (fd == -1) {
		perror (UINPUT);
		return -1;
	}

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) == -1) {
		perror ("Could not enable key events");
		goto fail;
	}

	for (i = 0; i < KEY_CNT; i++) {
		if (ioctl(fd, UI_SET_KEYBIT, i) == -1) {
			perror ("Could not enable a key event");
			goto fail;
		}
	}

	if (ioctl(fd, UI_SET_EVBIT, EV_REP) == -1) {
		perror ("Could not enable autorepeat events");
		goto fail;
	}

	strncpy (dev.name, name, UINPUT_MAX_NAME_SIZE);
	dev.id.bustype = BUS_VIRTUAL;
	dev.id.vendor  = 0x0666;
	dev.id.product = 0x8086;
	dev.id.version = 1;

	if (write (fd, &dev, sizeof (dev)) != sizeof (dev)) {
		perror (UINPUT);
		goto fail;
	}

	if (ioctl(fd, UI_DEV_CREATE) == -1) {
		perror ("Could not create the virtual keyboard");
		goto fail;
	}

	return fd;
fail:
	close (fd);
	return -1;
}

static int
open_input (char *dev)
{
	int version;
	int features;
	int fd;
	int norepeat[2] = { 0, 0 };

	fd = open (dev, O_RDWR);
	if (fd == -1) {
		perror (dev);
		return -1;
	}

	if (ioctl (fd, EVIOCGVERSION, &version) == -1) {
		perror ("Could not read input protocol version");
		goto fail;
	}
	if (version >> 16 != EV_VERSION >> 16) {
		fprintf (stderr, "Bad input subsystem version");
		goto fail;
	}

	if (ioctl (fd, EVIOCGBIT(0, EV_MAX), &features) == -1) {
		perror ("Could query device for supported features");
		goto fail;
	}
	if (!(features & EV_KEY)) {
		fprintf (stderr, "Device not capable of producing key press event.");
		goto fail;
	}

	if (ioctl (fd, EVIOCGRAB, 1) == -1) {
		perror ("Could not grab keyboard for exclusive use");
		goto fail;
	}

	if (ioctl (fd, EVIOCSREP, norepeat) == -1) {
		perror ("Could not disable autorepeat");
		goto fail;
	}

	return fd;
fail:
	close (fd);
	return -1;
}

int
main (int argc, char *argv[])
{
	int uinput[2], input;
	struct input_event event;
	int active = 0;
	int switching = 0;

	if (argc != 2) {
		fprintf (stderr, "Usage: %s /dev/input/event<n>\n", argv[0]);
		return 1;
	}

	input = open_input (argv[1]);
	if (input == -1)
		return 1;

	uinput[0] = open_uinput ("evmuxd primary");
	if (uinput[0] == -1)
		return 1;

	uinput[1] = open_uinput ("evmuxd secondary");
	if (uinput[1] == -1)
		return 1;

	while (1) {
		switch (read (input, &event, sizeof(event))) {
		case -1:
			perror ("Error reading from event device");
			return 1;
		case sizeof(event):
			break;
		default:
			fprintf (stderr, "Short read from the event device.\n");
			return 1;
		}

		switch (write (uinput[active], &event, sizeof(event))) {
		case -1:
			perror ("Error forwarding the event");
			return 1;
		case sizeof(event):
			break;
		default:
			fprintf (stderr, "Short write forwarding the event.\n");
			return 1;
		}

		if (event.type == EV_KEY
			&& event.code == KEY_SCROLLLOCK
			&& event.value == 0) {
			switching = 1;
		}
		if (event.type == EV_SYN && switching) {
			switching = 0;
			active = !active;
		}
	}

	return 0;
}
