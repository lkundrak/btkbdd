/*
 * Based on code from xkbd-bthid,
 * by Collin R. Mulliner <collin@betaversion.net>
 * http://www.mulliner.org/bluetooth/
 *
 * Modified by Lubomir Rintel <lkundrak@v3.sk>
 *
 * License: GPL
 */

#include <stdint.h>
#include <bluetooth/bluetooth.h>

#ifndef __VKBD_H
#define __VKBD_H

int l2cap_listen (const bdaddr_t *, unsigned short, int, int);
int l2cap_accept (int, bdaddr_t *);
int l2cap_connect (bdaddr_t *, bdaddr_t *, unsigned short);

void sdp_add_keyboard ();
void sdp_remove ();
void sdp_open ();

uint32_t set_class (int, uint32_t);
int session ();

#endif
