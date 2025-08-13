/* ptpcam.h
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#ifndef __PTPCAM_H__
#define __PTPCAM_H__

#ifdef LINUX_OS
#define USB_BULK_READ myusb_bulk_read
#define USB_BULK_WRITE myusb_bulk_write
int myusb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,
	int timeout);
int myusb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int length,
	int timeout);
#else
#define USB_BULK_READ usb_bulk_read
#define USB_BULK_WRITE usb_bulk_write
#endif

/*
 * macros
 */

/* Check value and Return on error */
#define CR(o,error) {						\
			uint16_t result=o;				\
			if((result)!=PTP_RC_OK) {			\
				ptp_perror(&params,result);		\
				fprintf(stderr,"ERROR: "error);		\
				close_camera(&ptp_usb, &params, dev);   \
				return;					\
			}						\
}

/* Check value and Continue on error */
#define CC(result,error) {						\
			if((result)!=PTP_RC_OK) {			\
				fprintf(stderr,"ERROR: "error);		\
				usb_release_interface(ptp_usb.handle,	\
		dev->config->interface->altsetting->bInterfaceNumber);	\
				continue;					\
			}						\
}

/* error reporting macro */
#ifndef ERROR
#define ERROR(error) fprintf(stderr,"ERROR: "error);				
#endif

/* property value printing macros */
#define PRINT_PROPVAL_DEC(value)	\
		print_propval(dpd.DataType, value,			\
		PTPCAM_PRINT_DEC)

#define PRINT_PROPVAL_HEX(value)					\
		print_propval(dpd.DataType, value,			\
		PTPCAM_PRINT_HEX)




/*
 * defines
 */

/* requested actions */
#define ACT_DEVICE_RESET	0x1
#define ACT_LIST_DEVICES	0x2
#define ACT_LIST_PROPERTIES	0x3
#define ACT_LIST_OPERATIONS	0x4
#define ACT_GETSET_PROPERTY	0x5
#define ACT_SHOW_INFO		0x6
#define ACT_LIST_FILES		0x7
#define ACT_GET_FILE		0x8
#define ACT_GET_ALL_FILES	0x9
#define ACT_CAPTURE		0xA
#define ACT_DELETE_OBJECT	0xB
#define ACT_DELETE_ALL_FILES	0xC
#define ACT_LOOP_CAPTURE	0xD
#define ACT_SHOW_ALL_PROPERTIES	0xE
#define ACT_SHOW_UNKNOWN_PROPERTIES	0xF
#define ACT_SET_PROPBYNAME	0x10

#define ACT_NIKON_DC		0x101
#define ACT_NIKON_DC2		0x102
#define ACT_NIKON_IC		0x103

#define ACT_CHDK		0x1337

/* printing value type */
#define PTPCAM_PRINT_HEX	00
#define PTPCAM_PRINT_DEC	01

/* filename overwrite */
#define OVERWRITE_EXISTING	1
#define	SKIP_IF_EXISTS		0


/*
 * structures
 */

typedef struct _PTP_USB PTP_USB;
struct _PTP_USB {
	usb_dev_handle* handle;
	int inep;
	int outep;
	int intep;
};

/*
 * variables
 */

/* one global variable */
extern short verbose;


/*
 * functions
 */

void ptpcam_siginthandler(int signum);

void usage(void);
void help(void);
void list_devices(short force);
void show_info (int busn, int devn, short force);
void list_files (int busn, int devn, short force);
void get_file (int busn, int devn, short force, uint32_t handle, char* filename, int overwrite);
void get_all_files (int busn, int devn, short force, int overwrite);
void capture_image (int busn, int devn, short force);
void nikon_direct_capture (int busn, int devn, short force, char* filename, int overwrite);
void nikon_direct_capture2 (int busn, int devn, short force, char* filename, int overwrite);
void delete_object (int busn, int devn, short force, uint32_t handle);
void delete_all_files (int busn, int devn, short force);
void list_operations (int busn, int devn, short force);
void list_devices(short force);
void list_properties (int dev, int bus, short force);
void loop_capture (int busn, int devn, short force, int n,  int overwrite);
void save_object(PTPParams *params, uint32_t handle, char* filename, PTPObjectInfo oi, int overwrite);
void get_save_object (PTPParams *params, uint32_t handle, char* filename, int overwrite);


struct usb_bus* init_usb(void);
void close_usb(PTP_USB* ptp_usb, struct usb_device* dev);
void init_ptp_usb (PTPParams*, PTP_USB*, struct usb_device*);
void clear_stall(PTP_USB* ptp_usb);

int usb_get_endpoint_status(PTP_USB* ptp_usb, int ep, uint16_t* status);
int usb_clear_stall_feature(PTP_USB* ptp_usb, int ep);
int open_camera (int busn, int devn, short force, PTP_USB *ptp_usb, PTPParams *params, struct usb_device **dev);
void close_camera (PTP_USB *ptp_usb, PTPParams *params, struct usb_device *dev);

#endif /* __PTPCAM_H__ */
