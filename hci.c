/*
 * HCI interface routines
 * Lubomir Rintel <lkundrak@v3.sk>
 * License: GPL
 */

#include <stdint.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

uint32_t
set_class (dev, class)
	int dev;
	uint32_t class;
{
	uint8_t save_class[3];

	dev = hci_open_dev(dev);
	if (dev == -1) {
		perror ("Can not open the bluetooth device");
		return 0;
	}
	if (hci_read_class_of_dev(dev, save_class, 1000) == -1) {
		perror ("Can not read HCI class");
		return 0;
	}
	if (hci_write_class_of_dev(dev, class, 1000) == -1 ) {
		perror ("Can not set HCI class");
		return 0;
	}

	return save_class[0] | save_class[1] << 8 | save_class[2] << 16;
}
