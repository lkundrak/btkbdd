/*
 * Main Bluetooth software keyboard loop
 * Lubomir Rintel <lkundrak@v3.sk>
 * License: GPL
 */

#include <stdlib.h>
#include <stdio.h>

#include "btkbdd.h"

int
main (argc, argv)
	int argc;
	char *argv[];
{
	uint32_t save_class;

	if (argc < 2 || argc > 3) {
		fprintf (stderr, "Usage: %s <device> [<ho:st:_a:dd:re:ss>]\n", argv[0]);
		return EXIT_FAILURE;
	}

	sdp_open ();
	sdp_add_keyboard ();
	save_class = set_class (0, 0x002540UL);
	while (session (argv[1], argc > 2 ? argv[2] : NULL));
	set_class (0, save_class);
	sdp_remove ();

	/* Only returns on fatal failure */
	return EXIT_FAILURE;
}
