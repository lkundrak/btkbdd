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

#ifndef __BTKBDD_H
#define __BTKBDD_H

//#define DEBUG
#ifdef DEBUG
#define DBG(...) fprintf (stderr, __VA_ARGS__)
#else
#define DBG(...)
#endif

int l2cap_listen (const bdaddr_t *, unsigned short, int, int);
int l2cap_accept (int, bdaddr_t *);
int l2cap_connect (bdaddr_t *, bdaddr_t *, unsigned short);

int sdp_open ();
void sdp_add_keyboard ();
void sdp_remove ();

uint32_t set_class (int, uint32_t);
int loop (char *, bdaddr_t, bdaddr_t *);

#endif
