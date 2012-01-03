/*
 * Main Bluetooth software keyboard loop
 * Lubomir Rintel <lkundrak@v3.sk>
 * License: GPL
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "btkbdd.h"

int
main (argc, argv)
	int argc;
	char *argv[];
{
	uint32_t save_class;
	char *device = NULL;
	char *cable = NULL;
        bdaddr_t src, tgt;
	int hci = 0;
	int opt;
	FILE *cablef;
	char addr[] = "00:00:00:00:00:00";

	bacpy (&src, BDADDR_ANY);
	bacpy (&tgt, BDADDR_ANY);

	while ((opt = getopt(argc, argv, "s:t:c:dv")) != -1) {

		switch (opt) {
		case 's':
			if (bachk (optarg) == -1) {
				fprintf (stderr, "%s: Not a valid bluetooth address\n", optarg);
				return EXIT_FAILURE;
			}
			str2ba (optarg, &src);
			hci = hci_devid (optarg);
			if (hci == -1) {
				perror (optarg);
				return EXIT_FAILURE;
			}
			break;
		case 't':
			if (bachk (optarg) == -1) {
				fprintf (stderr, "%s: Not a valid bluetooth address\n", optarg);
				return EXIT_FAILURE;
			}
			str2ba (optarg, &tgt);
			break;
		case 'c':
			cable = optarg;
			cablef = fopen (cable, "r");
			if (!cablef) {
				perror (cable);
				break;
			}
			fscanf (cablef, "%17s", addr);
			fclose (cablef);
			if (bachk (addr) == -1) {
				fprintf (stderr, "%s: Not a valid bluetooth address\n", addr);
			} else {
				str2ba (addr, &tgt);
			}
			break;
		case 'd':
			if (daemon (0, 0) == -1) {
				perror ("daemon");
				return EXIT_FAILURE;
			}
			break;
		default:
			fprintf (stderr, "Unexpected '%c' option.", opt);
		case '?':
			return EXIT_FAILURE;
		}
	}

	if (optind + 1 != argc) {
		fprintf (stderr, "Usage: %s "
			"[-s <sr:c_:_a:dd:re:ss>] "
			"[-t <de:st:_a:dd:re:ss>] "
			"[-c <file>] [-d] <device>\n", argv[0]);
		return EXIT_FAILURE;
	}

	device = argv[optind];

	save_class = set_class (hci, 0x002540UL);
	while (session (device, src, &tgt)) {
		if (sdp_open () == 1)
			sdp_add_keyboard ();
		if (!cable)
			continue;
		cablef = fopen (cable, "w");
		if (!cablef) {
			perror (cable);
			continue;
		}
		ba2str (&tgt, addr);
		fprintf (cablef, "%s\n", addr);
		fclose (cablef);
	}
	set_class (hci, save_class);
	sdp_remove ();

	/* Only returns on fatal failure */
	return EXIT_FAILURE;
}
