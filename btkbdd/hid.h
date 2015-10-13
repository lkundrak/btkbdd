#ifndef __HID_H__
#define __HID_H__

/* HIDP L2CAP PSMs */
#define L2CAP_PSM_HIDP_CTRL 0x11
#define L2CAP_PSM_HIDP_INTR 0x13

/* HIDP header masks */
#define HIDP_HEADER_TRANS_MASK                  0xf0
#define HIDP_HEADER_PARAM_MASK                  0x0f

/* HIDP transaction types */
#define HIDP_TRANS_HANDSHAKE                    0x00
#define HIDP_TRANS_HID_CONTROL                  0x10
#define HIDP_TRANS_GET_REPORT                   0x40
#define HIDP_TRANS_SET_REPORT                   0x50
#define HIDP_TRANS_GET_PROTOCOL                 0x60
#define HIDP_TRANS_SET_PROTOCOL                 0x70
#define HIDP_TRANS_GET_IDLE                     0x80
#define HIDP_TRANS_SET_IDLE                     0x90
#define HIDP_TRANS_DATA                         0xa0
#define HIDP_TRANS_DATC                         0xb0

/* HIDP handshake results */
#define HIDP_HSHK_SUCCESSFUL                    0x00
#define HIDP_HSHK_NOT_READY                     0x01
#define HIDP_HSHK_ERR_INVALID_REPORT_ID         0x02
#define HIDP_HSHK_ERR_UNSUPPORTED_REQUEST       0x03
#define HIDP_HSHK_ERR_INVALID_PARAMETER         0x04
#define HIDP_HSHK_ERR_UNKNOWN                   0x0e
#define HIDP_HSHK_ERR_FATAL                     0x0f

/* HIDP control operation parameters */
#define HIDP_CTRL_NOP                           0x00
#define HIDP_CTRL_HARD_RESET                    0x01
#define HIDP_CTRL_SOFT_RESET                    0x02
#define HIDP_CTRL_SUSPEND                       0x03
#define HIDP_CTRL_EXIT_SUSPEND                  0x04
#define HIDP_CTRL_VIRTUAL_CABLE_UNPLUG          0x05

/* HIDP data transaction headers */
#define HIDP_DATA_RTYPE_MASK                    0x03
#define HIDP_DATA_RSRVD_MASK                    0x0c
#define HIDP_DATA_RTYPE_OTHER                   0x00
#define HIDP_DATA_RTYPE_INPUT                   0x01
#define HIDP_DATA_RTYPE_OUTPUT                  0x02
#define HIDP_DATA_RTYPE_FEATURE                 0x03

/* HIDP protocol header parameters */
#define HIDP_PROTO_BOOT                         0x00
#define HIDP_PROTO_REPORT                       0x01

/* Indicator LEDs */
#define HIDP_NUML	0x01
#define HIDP_CAPSL	0x02
#define HIDP_SCROLLL	0x04
#define HIDP_COMPOSE	0x08
#define HIDP_KANA	0x10

/* Modifiers */
#define HIDP_LEFTCTRL	0x01
#define HIDP_LEFTSHIFT	0x02
#define HIDP_LEFTALT	0x04
#define HIDP_LEFTGUI	0x08
#define HIDP_RIGHTCTRL	0x10
#define HIDP_RIGHTSHIFT	0x20
#define HIDP_RIGHTALT	0x40
#define HIDP_RIGHTGUI	0x80

#endif
