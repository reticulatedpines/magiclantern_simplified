/* myusb.c
 *
 * Copyright (C) 2001-2005 Mariusz Woloszyn <emsi@ipartners.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "config.h"
#ifdef LINUX_OS

/*
 * libusb has changed the kernel interface used for bulk read/write operations.
 * the new, threaded (URB) interface is not required in this application
 * especially that it fails sometimes unexpectedly.
 * to avoid using TheNewBetterInterface we redefine the old one in this place.
 * most of the code below is copied from libusb 0.1.8 or so.
 */

#include <sys/ioctl.h>
#include <errno.h>
#include <usb.h>

#define IOCTL_USB_CONTROL       _IOWR('U', 0, struct usb_ctrltransfer)
#define IOCTL_USB_BULK          _IOWR('U', 2, struct usb_bulktransfer)
#define IOCTL_USB_RESETEP       _IOR('U', 3, unsigned int)
#define IOCTL_USB_SETINTF       _IOR('U', 4, struct usb_setinterface)
#define IOCTL_USB_SETCONFIG     _IOR('U', 5, unsigned int)
#define IOCTL_USB_GETDRIVER     _IOW('U', 8, struct usb_getdriver)
#define IOCTL_USB_SUBMITURB     _IOR('U', 10, struct usb_urb)
#define IOCTL_USB_DISCARDURB    _IO('U', 11)
#define IOCTL_USB_REAPURB       _IOW('U', 12, void *)
#define IOCTL_USB_REAPURBNDELAY _IOW('U', 13, void *)
#define IOCTL_USB_CLAIMINTF     _IOR('U', 15, unsigned int)
#define IOCTL_USB_RELEASEINTF   _IOR('U', 16, unsigned int)
#define IOCTL_USB_IOCTL         _IOWR('U', 18, struct usb_ioctl)
#define IOCTL_USB_RESET         _IO('U', 20)
#define IOCTL_USB_CLEAR_HALT    _IOR('U', 21, unsigned int)
#define IOCTL_USB_DISCONNECT    _IO('U', 22)    /* via IOCTL_USB_IOCTL */
#define IOCTL_USB_CONNECT       _IO('U', 23)    /* via IOCTL_USB_IOCTL */


struct usb_bulktransfer {
	/* keep in sync with usbdevice_fs.h:usbdevfs_bulktransfer */
	unsigned int ep;
	unsigned int len;
	unsigned int timeout;   /* in milliseconds */
	
	/* pointer to data */
	void *data;
};

struct usb_dev_handle {
	int fd;
	
	struct usb_bus *bus;
	struct usb_device *device;
	
	int config;
	int interface;
	int altsetting;
	
	/* Added by RMT so implementations can store other per-open-device data */
	void *impl_info;
};


/* Linux usbdevfs has a limit of one page size per read/write. 4096 is */
/* the most portable maximum we can do for now */
#define MAX_READ_WRITE	4096

int myusb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int length,
	int timeout);
int myusb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int length,
	int timeout)
{
	struct usb_bulktransfer bulk;
	int ret, sent = 0;

	/* Ensure the endpoint address is correct */
	if (ep < 0 || ep > 0x0f)
		return (-EINVAL);

	do {
		bulk.ep = ep;
		bulk.len = length - sent;
		if (bulk.len > MAX_READ_WRITE)
			bulk.len = MAX_READ_WRITE;
		bulk.timeout = timeout;
		bulk.data = (unsigned char *)bytes + sent;

		ret = ioctl(dev->fd, IOCTL_USB_BULK, &bulk);
		if (ret < 0)
			return (-errno);

		sent += ret;
	} while (ret > 0 && sent < length);

	return sent;
}

int myusb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,
	int timeout);
int myusb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,
	int timeout)
{
	struct usb_bulktransfer bulk;
	int ret, retrieved = 0, requested;

	/* Ensure the endpoint address is correct */
	ep |= USB_ENDPOINT_IN;

	do {
		bulk.ep = ep;
		requested = size - retrieved;
		if (requested > MAX_READ_WRITE)
			requested = MAX_READ_WRITE;
		bulk.len = requested;
		bulk.timeout = timeout;
		bulk.data = (unsigned char *)bytes + retrieved;

		ret = ioctl(dev->fd, IOCTL_USB_BULK, &bulk);
		if (ret < 0)
			return (-errno);

		retrieved += ret;
	} while (ret > 0 && retrieved < size && ret == requested);

	return retrieved;
}
#endif /* LINUX_OS */
