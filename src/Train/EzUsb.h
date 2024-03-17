#ifndef __ezusb_H
#define __ezusb_H
/*
 * Copyright (c) 2001 Stephen Williams (steve@icarus.com)
 * Copyright (c) 2002 David Brownell (dbrownell@users.sourceforge.net)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 */

// For explicit runtime linking to libusb functions
typedef int (*Prototype_EzUsb_control_msg)(usb_dev_handle *, int, int, int, int, char *, int, int);

/*
 * This function loads the firmware from the given file into RAM.
 * The file is assumed to be in Intel HEX format.  If fx2 is set, uses
 * appropriate reset commands.  Stage == 0 means this is a single stage
 * load (or the first of two stages).  Otherwise it's the second of
 * two stages; the caller preloaded the second stage loader.
 *
 * The target processor is reset at the end of this download.
 */
extern int ezusb_load_ram (usb_dev_handle *device, const char *path, int fx2, int stage, Prototype_EzUsb_control_msg uptr);

/*
 * This function performs a specialized firmware load for the
 * earlier 1902 model as used by the Tacx Imagic
 */
extern int ezusb_load_ram_imagic (usb_dev_handle *device, const char *path, Prototype_EzUsb_control_msg uptr);


/*
 * This function stores the firmware from the given file into EEPROM.
 * The file is assumed to be in Intel HEX format.  This uses the right
 * CPUCS address to terminate the EEPROM load with a reset command,
 * where FX parts behave differently than FX2 ones.  The configuration
 * byte is as provided here (zero for an21xx parts) and the EEPROM
 * type is set so that the microcontroller will boot from it.
 * 
 * The caller must have preloaded a second stage loader that knows
 * how to respond to the EEPROM write request.
 */
extern int ezusb_load_eeprom (
	usb_dev_handle	*dev,		/* usbfs device handle */
	const char *path,	/* path to hexfile */
	const char *type,	/* fx, fx2, an21 */
	int config		/* config byte for fx/fx2; else zero */
	);


/* boolean flag, says whether to write extra messages to stderr */
extern int verbose;


#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */



/*
 * $Log: ezusb.h,v $
 * Revision 1.1  2007/03/19 20:46:30  cfavi
 * fxload ported to use libusb
 *
 * Revision 1.3  2002/04/12 00:28:21  dbrownell
 * support "-t an21" to program EEPROMs for those microcontrollers
 *
 * Revision 1.2  2002/02/26 19:55:05  dbrownell
 * 2nd stage loader support
 *
 * Revision 1.1  2001/06/12 00:00:50  stevewilliams
 *  Added the fxload program.
 *  Rework root makefile and hotplug.spec to install in prefix
 *  location without need of spec file for install.
 *
 */
#endif
