/* ptpcam.c
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
#include "ptp.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/mman.h>
#endif
#include <usb.h>

#ifdef WIN32
#define usleep(usec) Sleep((usec)/1000)
#define sleep(sec) Sleep(sec*1000)
#endif

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (GETTEXT_PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif

#include "ptpcam.h"

/* some defines comes here */

/* CHDK additions */
#define CHDKBUFS 65535
#define CHDK_MODE_INTERACTIVE 0
#define CHDK_MODE_CLI 1
#define MAXCONNRETRIES 10


/* USB interface class */
#ifndef USB_CLASS_PTP
#define USB_CLASS_PTP		6
#endif

/* USB control message data phase direction */
#ifndef USB_DP_HTD
#define USB_DP_HTD		(0x00 << 7)	/* host to device */
#endif
#ifndef USB_DP_DTH
#define USB_DP_DTH		(0x01 << 7)	/* device to host */
#endif

/* PTP class specific requests */
#ifndef USB_REQ_DEVICE_RESET
#define USB_REQ_DEVICE_RESET		0x66
#endif
#ifndef USB_REQ_GET_DEVICE_STATUS
#define USB_REQ_GET_DEVICE_STATUS	0x67
#endif

/* USB Feature selector HALT */
#ifndef USB_FEATURE_HALT
#define USB_FEATURE_HALT	0x00
#endif

/* OUR APPLICATION USB URB (2MB) ;) */
#define PTPCAM_USB_URB		2097152

#define USB_TIMEOUT		5000
#define USB_CAPTURE_TIMEOUT	20000

/* one global variable (yes, I know it sucks) */
short verbose=0;
/* the other one, it sucks definitely ;) */
int ptpcam_usb_timeout = USB_TIMEOUT;

/* we need it for a proper signal handling :/ */
PTPParams* globalparams;


void
usage()
{
	printf("USAGE: ptpcam [OPTION]\n\n");
}

void
help()
{
	printf("USAGE: ptpcam [OPTION]\n\n");
	printf("Options:\n"
	"  --bus=BUS-NUMBER             USB bus number\n"
	"  --dev=DEV-NUMBER             USB assigned device number\n"
	"  -r, --reset                  Reset the device\n"
	"  -l, --list-devices           List all PTP devices\n"
	"  -i, --info                   Show device info\n"
	"  -o, --list-operations        List supported operations\n"
	"  -p, --list-properties        List all PTP device properties\n"
	"                               "
				"(e.g. focus mode, focus distance, etc.)\n"
	"  -s, --show-property=NUMBER   Display property details "
					"(or set its value,\n"
	"                               if used in conjunction with --val)\n"
	"  --set-property=NUMBER        Set property value (--val required)\n"
	"  --set=PROP-NAME              Set property by name (abbreviations allowed)\n"
	"  --val=VALUE                  Property value (numeric for --set-property and\n"
	"                               string or numeric for --set)\n"
	"  --show-all-properties        Show all properties values\n"
	"  --show-unknown-properties    Show unknown properties values\n"
	"  -L, --list-files             List all files\n"
	"  -g, --get-file=HANDLE        Get file by given handler\n"
	"  -G, --get-all-files          Get all files\n"
	"  --overwrite                  Force file overwrite while saving"
					"to disk\n"
	"  -d, --delete-object=HANDLE   Delete object (file) by given handle\n"
	"  -D, --delete-all-files       Delete all files form camera\n"
	"  -c, --capture                Initiate capture\n"
	"  --nikon-ic, --nic            Initiate Nikon Direct Capture (no download!)\n"
	"  --nikon-dc, --ndc            Initiate Nikon Direct Capture and download\n"
	"  --loop-capture=N             Perform N times capture/get/delete\n"
	"  -f, --force                  Talk to non PTP devices\n"
	"  -v, --verbose                Be verbose (print more debug)\n"
	"  -h, --help                   Print this help message\n"
	"  --chdk[=command]             CHDK mode. Interactive shell unless optional\n"
	"                               command is given. Run interactive shell and\n"
	"                               press 'h' for a list of commands.\n"
	"\n");
}

void
ptpcam_siginthandler(int signum)
{
    PTP_USB* ptp_usb=(PTP_USB *)globalparams->data;
    struct usb_device *dev=usb_device(ptp_usb->handle);

    if (signum==SIGINT)
    {
	/* hey it's not that easy though... but at least we can try! */
	printf("Got SIGINT, trying to clean up and close...\n");
	usleep(5000);
	close_camera (ptp_usb, globalparams, dev);
	exit (-1);
    }
}

static short
ptp_read_func (unsigned char *bytes, unsigned int size, void *data)
{
	int result=-1;
	PTP_USB *ptp_usb=(PTP_USB *)data;
	int toread=0;
	signed long int rbytes=size;

	do {
		bytes+=toread;
		if (rbytes>PTPCAM_USB_URB) 
			toread = PTPCAM_USB_URB;
		else
			toread = rbytes;
		result=USB_BULK_READ(ptp_usb->handle, ptp_usb->inep,(char *)bytes, toread,ptpcam_usb_timeout);
		/* sometimes retry might help */
		if (result==0)
			result=USB_BULK_READ(ptp_usb->handle, ptp_usb->inep,(char *)bytes, toread,ptpcam_usb_timeout);
		if (result < 0)
			break;
		rbytes-=PTPCAM_USB_URB;
	} while (rbytes>0);

	if (result >= 0) {
		return (PTP_RC_OK);
	}
	else 
	{
		if (verbose) perror("usb_bulk_read");
		return PTP_ERROR_IO;
	}
}

static short
ptp_write_func (unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb=(PTP_USB *)data;

	result=USB_BULK_WRITE(ptp_usb->handle,ptp_usb->outep,(char *)bytes,size,ptpcam_usb_timeout);
	if (result >= 0)
		return (PTP_RC_OK);
	else 
	{
		if (verbose) perror("usb_bulk_write");
		return PTP_ERROR_IO;
	}
}

/* XXX this one is suposed to return the number of bytes read!!! */
static short
ptp_check_int (unsigned char *bytes, unsigned int size, void *data)
{
	int result;
	PTP_USB *ptp_usb=(PTP_USB *)data;

	result=USB_BULK_READ(ptp_usb->handle, ptp_usb->intep,(char *)bytes,size,ptpcam_usb_timeout);
	if (result==0)
	    result=USB_BULK_READ(ptp_usb->handle, ptp_usb->intep,(char *)bytes,size,ptpcam_usb_timeout);
	if (verbose>2) fprintf (stderr, "USB_BULK_READ returned %i, size=%i\n", result, size);

	if (result >= 0) {
		return result;
	} else {
		if (verbose) perror("ptp_check_int");
		return result;
	}
}


void
ptpcam_debug (void *data, const char *format, va_list args);
void
ptpcam_debug (void *data, const char *format, va_list args)
{
	if (verbose<2) return;
	vfprintf (stderr, format, args);
	fprintf (stderr,"\n");
	fflush(stderr);
}

void
ptpcam_error (void *data, const char *format, va_list args);
void
ptpcam_error (void *data, const char *format, va_list args)
{
/*	if (!verbose) return; */
	vfprintf (stderr, format, args);
	fprintf (stderr,"\n");
	fflush(stderr);
}



void
init_ptp_usb (PTPParams* params, PTP_USB* ptp_usb, struct usb_device* dev)
{
	usb_dev_handle *device_handle;

	params->write_func=ptp_write_func;
	params->read_func=ptp_read_func;
	params->check_int_func=ptp_check_int;
	params->check_int_fast_func=ptp_check_int;
	params->error_func=ptpcam_error;
	params->debug_func=ptpcam_debug;
	params->sendreq_func=ptp_usb_sendreq;
	params->senddata_func=ptp_usb_senddata;
	params->getresp_func=ptp_usb_getresp;
	params->getdata_func=ptp_usb_getdata;
	params->data=ptp_usb;
	params->transaction_id=0;
	params->byteorder = PTP_DL_LE;

	if ((device_handle=usb_open(dev))){
		if (!device_handle) {
			perror("usb_open()");
			exit(0);
		}
		ptp_usb->handle=device_handle;
		usb_set_configuration(device_handle, dev->config->bConfigurationValue);
		usb_claim_interface(device_handle,
			dev->config->interface->altsetting->bInterfaceNumber);
	}
	globalparams=params;
}

void
clear_stall(PTP_USB* ptp_usb)
{
	uint16_t status=0;
	int ret;

	/* check the inep status */
	ret=usb_get_endpoint_status(ptp_usb,ptp_usb->inep,&status);
	if (ret<0) perror ("inep: usb_get_endpoint_status()");
	/* and clear the HALT condition if happend */
	else if (status) {
		printf("Resetting input pipe!\n");
		ret=usb_clear_stall_feature(ptp_usb,ptp_usb->inep);
        	/*usb_clear_halt(ptp_usb->handle,ptp_usb->inep); */
		if (ret<0)perror ("usb_clear_stall_feature()");
	}
	status=0;

	/* check the outep status */
	ret=usb_get_endpoint_status(ptp_usb,ptp_usb->outep,&status);
	if (ret<0) perror ("outep: usb_get_endpoint_status()");
	/* and clear the HALT condition if happend */
	else if (status) {
		printf("Resetting output pipe!\n");
        	ret=usb_clear_stall_feature(ptp_usb,ptp_usb->outep);
		/*usb_clear_halt(ptp_usb->handle,ptp_usb->outep); */
		if (ret<0)perror ("usb_clear_stall_feature()");
	}

        /*usb_clear_halt(ptp_usb->handle,ptp_usb->intep); */
}

void
close_usb(PTP_USB* ptp_usb, struct usb_device* dev)
{
	//clear_stall(ptp_usb);
        usb_release_interface(ptp_usb->handle,
                dev->config->interface->altsetting->bInterfaceNumber);
	usb_reset(ptp_usb->handle);
        usb_close(ptp_usb->handle);
}


struct usb_bus*
init_usb()
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	return (usb_get_busses());
}

/*
   find_device() returns the pointer to a usb_device structure matching
   given busn, devicen numbers. If any or both of arguments are 0 then the
   first matching PTP device structure is returned. 
*/
struct usb_device*
find_device (int busn, int devicen, short force);
struct usb_device*
find_device (int busn, int devn, short force)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	bus=init_usb();
	for (; bus; bus = bus->next)
	for (dev = bus->devices; dev; dev = dev->next)
	if (dev->config)
	if ((dev->config->interface->altsetting->bInterfaceClass==
		USB_CLASS_PTP)||force)
	if (dev->descriptor.bDeviceClass!=USB_CLASS_HUB)
	{
		int curbusn, curdevn;

		curbusn=strtol(bus->dirname,NULL,10);
#ifdef WIN32
		curdevn=strtol(strchr(dev->filename,'-')+1,NULL,10);
#else    
		curdevn=strtol(dev->filename,NULL,10);
#endif

		if (devn==0) {
			if (busn==0) return dev;
			if (curbusn==busn) return dev;
		} else {
			if ((busn==0)&&(curdevn==devn)) return dev;
			if ((curbusn==busn)&&(curdevn==devn)) return dev;
		}
	}
	return NULL;
}

void
find_endpoints(struct usb_device *dev, int* inep, int* outep, int* intep);
void
find_endpoints(struct usb_device *dev, int* inep, int* outep, int* intep)
{
	int i,n;
	struct usb_endpoint_descriptor *ep;

	ep = dev->config->interface->altsetting->endpoint;
	n=dev->config->interface->altsetting->bNumEndpoints;

	for (i=0;i<n;i++) {
	if (ep[i].bmAttributes==USB_ENDPOINT_TYPE_BULK)	{
		if ((ep[i].bEndpointAddress&USB_ENDPOINT_DIR_MASK)==
			USB_ENDPOINT_DIR_MASK)
		{
			*inep=ep[i].bEndpointAddress;
			if (verbose>1)
				fprintf(stderr, "Found inep: 0x%02x\n",*inep);
		}
		if ((ep[i].bEndpointAddress&USB_ENDPOINT_DIR_MASK)==0)
		{
			*outep=ep[i].bEndpointAddress;
			if (verbose>1)
				fprintf(stderr, "Found outep: 0x%02x\n",*outep);
		}
		} else if ((ep[i].bmAttributes==USB_ENDPOINT_TYPE_INTERRUPT) &&
			((ep[i].bEndpointAddress&USB_ENDPOINT_DIR_MASK)==
				USB_ENDPOINT_DIR_MASK))
		{
			*intep=ep[i].bEndpointAddress;
			if (verbose>1)
				fprintf(stderr, "Found intep: 0x%02x\n",*intep);
		}
	}
}

int
open_camera (int busn, int devn, short force, PTP_USB *ptp_usb, PTPParams *params, struct usb_device **dev)
{
	int retrycnt=0;
	uint16_t ret=0;

#ifdef DEBUG
	printf("dev %i\tbus %i\n",devn,busn);
#endif
	
  // retry device find for a while (in case the user just powered it on or called restart)
	while ((retrycnt++ < MAXCONNRETRIES) && !ret) {
		*dev=find_device(busn,devn,force);
		if (*dev!=NULL) 
			ret=1;
		else {
			fprintf(stderr,"Could not find any device matching given bus/dev numbers, retrying in 1 s...\n");
			fflush(stderr);
			sleep(1);
		}
	}

	if (*dev==NULL) {
		fprintf(stderr,"could not find any device matching given "
		"bus/dev numbers\n");
		return -1;
	}
	find_endpoints(*dev,&ptp_usb->inep,&ptp_usb->outep,&ptp_usb->intep);
    init_ptp_usb(params, ptp_usb, *dev);   

  // first connection attempt often fails if some other app or driver has accessed the camera, retry for a while
	retrycnt=0;
	while ((retrycnt++ < MAXCONNRETRIES) && ((ret=ptp_opensession(params,1))!=PTP_RC_OK)) {
		printf("Failed to connect (attempt %d), retrying in 1 s...\n", retrycnt);
		close_usb(ptp_usb, *dev);
		sleep(1);
		find_endpoints(*dev,&ptp_usb->inep,&ptp_usb->outep,&ptp_usb->intep);
		init_ptp_usb(params, ptp_usb, *dev);   
	}  
	if (ret != PTP_RC_OK) {
		fprintf(stderr,"ERROR: Could not open session!\n");
		close_usb(ptp_usb, *dev);
		return -1;
	}

	if (ptp_getdeviceinfo(params,&params->deviceinfo)!=PTP_RC_OK) {
		fprintf(stderr,"ERROR: Could not get device info!\n");
		close_usb(ptp_usb, *dev);
		return -1;
	}
	return 0;
}

void
close_camera (PTP_USB *ptp_usb, PTPParams *params, struct usb_device *dev)
{
	if (ptp_closesession(params)!=PTP_RC_OK)
		fprintf(stderr,"ERROR: Could not close session!\n");
	close_usb(ptp_usb, dev);
}


void
list_devices(short force)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	int found=0;


	bus=init_usb();
  	for (; bus; bus = bus->next)
    	for (dev = bus->devices; dev; dev = dev->next) {
		/* if it's a PTP device try to talk to it */
		if (dev->config)
		if ((dev->config->interface->altsetting->bInterfaceClass==
			USB_CLASS_PTP)||force)
		if (dev->descriptor.bDeviceClass!=USB_CLASS_HUB)
		{
			PTPParams params;
			PTP_USB ptp_usb;
			PTPDeviceInfo deviceinfo;

			if (!found){
				printf("\nListing devices...\n");
				printf("bus/dev\tvendorID/prodID\tdevice model\n");
				found=1;
			}

			find_endpoints(dev,&ptp_usb.inep,&ptp_usb.outep,
				&ptp_usb.intep);
			init_ptp_usb(&params, &ptp_usb, dev);

			CC(ptp_opensession (&params,1),
				"Could not open session!\n"
				"Try to reset the camera.\n");
			CC(ptp_getdeviceinfo (&params, &deviceinfo),
				"Could not get device info!\n");

      			printf("%s/%s\t0x%04X/0x%04X\t%s\n",
				bus->dirname, dev->filename,
				dev->descriptor.idVendor,
				dev->descriptor.idProduct, deviceinfo.Model);

			CC(ptp_closesession(&params),
				"Could not close session!\n");
			close_usb(&ptp_usb, dev);
		}
	}
	if (!found) printf("\nFound no PTP devices\n");
	printf("\n");
}

void
show_info (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	printf("\nCamera information\n");
	printf("==================\n");
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
	printf("Model: %s\n",params.deviceinfo.Model);
	printf("  manufacturer: %s\n",params.deviceinfo.Manufacturer);
	printf("  serial number: '%s'\n",params.deviceinfo.SerialNumber);
	printf("  device version: %s\n",params.deviceinfo.DeviceVersion);
	printf("  extension ID: 0x%08lx\n",(long unsigned)
					params.deviceinfo.VendorExtensionID);
	printf("  extension description: %s\n",
					params.deviceinfo.VendorExtensionDesc);
	printf("  extension version: 0x%04x\n",
				params.deviceinfo.VendorExtensionVersion);
	printf("\n");
	close_camera(&ptp_usb, &params, dev);
}

void
capture_image (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	PTPContainer event;
	int ExposureTime=0;
	struct usb_device *dev;
	short ret;

	printf("\nInitiating captue...\n");
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	if (!ptp_operation_issupported(&params, PTP_OC_InitiateCapture))
	{
	    printf ("Your camera does not support InitiateCapture operation!\nSorry, blame the %s!\n", params.deviceinfo.Manufacturer);
	    goto out;
	}

	/* obtain exposure time in miliseconds */
	if (ptp_property_issupported(&params, PTP_DPC_ExposureTime))
	{
	    PTPDevicePropDesc dpd;
	    memset(&dpd,0,sizeof(dpd));
	    ret=ptp_getdevicepropdesc(&params,PTP_DPC_ExposureTime,&dpd);
	    if (ret==PTP_RC_OK) ExposureTime=(*(int32_t*)(dpd.CurrentValue))/10;
	}

	/* adjust USB timeout */
	if (ExposureTime>USB_TIMEOUT) ptpcam_usb_timeout=ExposureTime;

	CR(ptp_initiatecapture (&params, 0x0, 0), "Could not capture.\n");
	
	ret=ptp_usb_event_wait(&params,&event);
	if (ret!=PTP_RC_OK) goto err;
	if (verbose) printf ("Event received %08x, ret=%x\n", event.Code, ret);
	if (event.Code==PTP_EC_CaptureComplete) {
		printf ("Camera reported 'capture completed' but the object information is missing.\n");
		goto out;
	}
		
	while (event.Code==PTP_EC_ObjectAdded) {
		printf ("Object added 0x%08lx\n", (long unsigned) event.Param1);
		if (ptp_usb_event_wait(&params, &event)!=PTP_RC_OK)
			goto err;
		if (verbose) printf ("Event received %08x, ret=%x\n", event.Code, ret);
		if (event.Code==PTP_EC_CaptureComplete) {
			printf ("Capture completed successfully!\n");
			goto out;
		}
	}
	
err:
	printf("Events receiving error. Capture status unknown.\n");
out:

	ptpcam_usb_timeout=USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
}

void
loop_capture (int busn, int devn, short force, int n,  int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	PTPContainer event;
	struct usb_device *dev;
	int file;
	PTPObjectInfo oi;
	uint32_t handle=0;
	char *image;
	int ret;
	char *filename;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	/* capture timeout should be longer */
	ptpcam_usb_timeout=USB_CAPTURE_TIMEOUT;

	printf("Camera: %s\n",params.deviceinfo.Model);


	/* local loop */
	while (n>0) {
		/* capture */
		printf("\nInitiating captue...\n");
		CR(ptp_initiatecapture (&params, 0x0, 0),"Could not capture\n");
		n--;

		ret=ptp_usb_event_wait(&params,&event);
		if (verbose) printf ("Event received %08x, ret=%x\n", event.Code, ret);
		if (ret!=PTP_RC_OK) goto err;
		if (event.Code==PTP_EC_CaptureComplete) {
			printf ("CANNOT DOWNLOAD: got 'capture completed' but the object information is missing.\n");
			goto out;
		}
			
		while (event.Code==PTP_EC_ObjectAdded) {
			printf ("Object added 0x%08lx\n",(long unsigned) event.Param1);
			handle=event.Param1;
			if (ptp_usb_event_wait(&params, &event)!=PTP_RC_OK)
				goto err;
			if (verbose) printf ("Event received %08x, ret=%x\n", event.Code, ret);
			if (event.Code==PTP_EC_CaptureComplete)
				goto download;
		}
download:	

		memset(&oi, 0, sizeof(PTPObjectInfo));
		if (verbose) printf ("Downloading: 0x%08lx\n",(long unsigned) handle);
		if ((ret=ptp_getobjectinfo(&params,handle, &oi))!=PTP_RC_OK){
			fprintf(stderr,"ERROR: Could not get object info\n");
			ptp_perror(&params,ret);
			if (ret==PTP_ERROR_IO) clear_stall(&ptp_usb);
			continue;
		}
	
		if (oi.ObjectFormat == PTP_OFC_Association)
				goto out;
		filename=(oi.Filename);
#ifdef WIN32
                goto out;
#else
		file=open(filename, (overwrite==OVERWRITE_EXISTING?0:O_EXCL)|O_RDWR|O_CREAT|O_TRUNC,S_IRWXU|S_IRGRP);
#endif
		if (file==-1) {
			if (errno==EEXIST) {
				printf("Skipping file: \"%s\", file exists!\n",filename);
				goto out;
			}
			perror("open");
			goto out;
		}
		lseek(file,oi.ObjectCompressedSize-1,SEEK_SET);
		ret=write(file,"",1);
		if (ret==-1) {
			perror("write");
			goto out;
		}
#ifndef WIN32
		image=mmap(0,oi.ObjectCompressedSize,PROT_READ|PROT_WRITE,MAP_SHARED,
			file,0);
		if (image==MAP_FAILED) {
			perror("mmap");
			close(file);
			goto out;
		}
#endif
		printf ("Saving file: \"%s\" ",filename);
		fflush(NULL);
		ret=ptp_getobject(&params,handle,&image);
		munmap(image,oi.ObjectCompressedSize);
		close(file);
		if (ret!=PTP_RC_OK) {
			printf ("error!\n");
			ptp_perror(&params,ret);
			if (ret==PTP_ERROR_IO) clear_stall(&ptp_usb);
		} else {
			/* and delete from camera! */
			printf("is done...\nDeleting from camera.\n");
			CR(ptp_deleteobject(&params, handle,0),
					"Could not delete object\n");
			printf("Object 0x%08lx (%s) deleted.\n",(long unsigned) handle, oi.Filename);
		}
out:
		;
	}
err:

	ptpcam_usb_timeout=USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
}

void
nikon_initiate_dc (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;
    
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	printf("Camera: %s\n",params.deviceinfo.Model);
	printf("\nInitiating direct captue...\n");
	
	if (params.deviceinfo.VendorExtensionID!=PTP_VENDOR_NIKON)
	{
	    printf ("Your camera is not Nikon!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	if (!ptp_operation_issupported(&params,PTP_OC_NIKON_DirectCapture)) {
	    printf ("Sorry, your camera dows not support Nikon DirectCapture!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	/* perform direct capture */
	result=ptp_nikon_directcapture (&params, 0xffffffff);
	if (result!=PTP_RC_OK) {
	    ptp_perror(&params,result);
	    fprintf(stderr,"ERROR: Could not capture.\n");
	    if (result!=PTP_RC_StoreFull) {
		close_camera(&ptp_usb, &params, dev);
		return;
	    }
	}
	usleep(300*1000);
out:	
	close_camera(&ptp_usb, &params, dev);

}

void
nikon_direct_capture (int busn, int devn, short force, char* filename,int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;
	uint16_t nevent=0;
	PTPUSBEventContainer* events=NULL;
	int ExposureTime=0;	/* exposure time in miliseconds */
	int BurstNumber=1;
	PTPDevicePropDesc dpd;
	PTPObjectInfo oi;
	int i;
    
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	printf("Camera: %s\n",params.deviceinfo.Model);

	if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))==PTP_RC_OK) {
	    if (filename==NULL) filename=oi.Filename;
	    save_object(&params, 0xffff0001, filename, oi, overwrite);
	    goto out;
	}

	printf("\nInitiating direct captue...\n");
	
	if (params.deviceinfo.VendorExtensionID!=PTP_VENDOR_NIKON)
	{
	    printf ("Your camera is not Nikon!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	if (!ptp_operation_issupported(&params,PTP_OC_NIKON_DirectCapture)) {
	    printf ("Sorry, your camera dows not support Nikon DirectCapture!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	/* obtain exposure time in miliseconds */
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_ExposureTime,&dpd);
	if (result==PTP_RC_OK) ExposureTime=(*(int32_t*)(dpd.CurrentValue))/10;

	/* obtain burst number */
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_BurstNumber,&dpd);
	if (result==PTP_RC_OK) BurstNumber=*(uint16_t*)(dpd.CurrentValue);
/*
	if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))==PTP_RC_OK)
	{
	    if (filename==NULL) filename=oi.Filename;
	    save_object(&params, 0xffff0001, filename, oi, overwrite);
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);
	}
*/

	/* perform direct capture */
	result=ptp_nikon_directcapture (&params, 0xffffffff);
	if (result!=PTP_RC_OK) {
	    ptp_perror(&params,result);
	    fprintf(stderr,"ERROR: Could not capture.\n");
	    if (result!=PTP_RC_StoreFull) {
		close_camera(&ptp_usb, &params, dev);
		return;
	    }
	}
	if (BurstNumber>1) printf("Capturing %i frames in burst.\n",BurstNumber);

	/* sleep in case of exposure longer than 1/100 */
	if (ExposureTime>10) {
	    printf ("sleeping %i miliseconds\n", ExposureTime);
	    usleep (ExposureTime*1000);
	}

	while (BurstNumber>0) {

#if 0	    /* Is this really needed??? */
	    ptp_nikon_keepalive(&params);
#endif

	    result=ptp_nikon_checkevent (&params, &events, &nevent);
	    if (result != PTP_RC_OK) {
		fprintf(stderr, "Error checking Nikon events\n");
		ptp_perror(&params,result);
		goto out;
	    }
	    for(i=0;i<nevent;i++) {
	    ptp_nikon_keepalive(&params);
		void *prop;
		if (events[i].code==PTP_EC_DevicePropChanged) {
		    printf ("Checking: %s\n", ptp_prop_getname(&params, events[i].param1));
		    ptp_getdevicepropvalue(&params, events[i].param1, &prop, PTP_DTC_UINT64);
		}

		printf("Event [%i] = 0x%04x,\t param: %08x\n",i, events[i].code ,events[i].param1);
		if (events[i].code==PTP_EC_NIKON_CaptureOverflow) {
		    printf("Ram cache overflow? Shooting to fast!\n");
		    if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))!=PTP_RC_OK) {
		        fprintf(stderr, "Could not get object info\n");
		        ptp_perror(&params,result);
		        goto out;
		    }
		    if (filename==NULL) filename=oi.Filename;
		    save_object(&params, 0xffff0001, filename, oi, overwrite);
		    BurstNumber=0;
		    usleep(100);
		} else
		if (events[i].code==PTP_EC_NIKON_ObjectReady) 
		{
		    if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))!=PTP_RC_OK) {
		        fprintf(stderr, "Could not get object info\n");
		        ptp_perror(&params,result);
		        goto out;
		    }
		    if (filename==NULL) filename=oi.Filename;
		    save_object(&params, 0xffff0001, filename, oi, overwrite);
		    BurstNumber--;
		}
	    }
	    free (events);
	}

out:	
	ptpcam_usb_timeout=USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
}


void
nikon_direct_capture2 (int busn, int devn, short force, char* filename, int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;
	PTPObjectInfo oi;

	dev=find_device(busn,devn,force);
	if (dev==NULL) {
		fprintf(stderr,"could not find any device matching given "
		"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(dev,&ptp_usb.inep,&ptp_usb.outep,&ptp_usb.intep);

	init_ptp_usb(&params, &ptp_usb, dev);

	if (ptp_opensession(&params,1)!=PTP_RC_OK) {
		fprintf(stderr,"ERROR: Could not open session!\n");
		close_usb(&ptp_usb, dev);
		return ;
	}
/*
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_BurstNumber,&dpd);
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_ExposureTime,&dpd);
*/

	/* perform direct capture */
	result=ptp_nikon_directcapture (&params, 0xffffffff);
	if (result!=PTP_RC_OK) {
	    ptp_perror(&params,result);
	    fprintf(stderr,"ERROR: Could not capture.\n");
	    if (result!=PTP_RC_StoreFull) {
	        close_camera(&ptp_usb, &params, dev);
	        return;
	    }
	}

	if (ptp_closesession(&params)!=PTP_RC_OK)
	{
	    fprintf(stderr,"ERROR: Could not close session!\n");
	    return;
	}

	usleep(300*1000);

	if (ptp_opensession(&params,1)!=PTP_RC_OK) {
    		fprintf(stderr,"ERROR: Could not open session!\n");
    		close_usb(&ptp_usb, dev);
    		return;
    	}
loop:
    	if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))==PTP_RC_OK) {
    	    if (filename==NULL) filename=oi.Filename;
    	    save_object(&params, 0xffff0001, filename, oi, overwrite);
    	} else {
	    ptp_nikon_keepalive(&params);
	    goto loop;
	}

/*out:	*/
	close_camera(&ptp_usb, &params, dev);


#if 0
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t result;
	uint16_t nevent=0;
	PTPUSBEventContainer* events=NULL;
	int ExposureTime=0;	/* exposure time in miliseconds */
	int BurstNumber=1;
	PTPDevicePropDesc dpd;
	PTPObjectInfo oi;
	int i;
	char *filename=NULL;
    
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	printf("Camera: %s\n",params.deviceinfo.Model);
	printf("\nInitiating direct captue...\n");
	
	if (params.deviceinfo.VendorExtensionID!=PTP_VENDOR_NIKON)
	{
	    printf ("Your camera is not Nikon!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	if (!ptp_operation_issupported(&params,PTP_OC_NIKON_DirectCapture)) {
	    printf ("Sorry, your camera dows not support Nikon DirectCapture!\nDo not buy from %s!\n",params.deviceinfo.Manufacturer);
	    goto out;
	}

	/* obtain exposure time in miliseconds */
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_ExposureTime,&dpd);
	if (result==PTP_RC_OK) ExposureTime=(*(int32_t*)(dpd.CurrentValue))/10;

	/* obtain burst number */
	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(&params,PTP_DPC_BurstNumber,&dpd);
	if (result==PTP_RC_OK) BurstNumber=*(uint16_t*)(dpd.CurrentValue);

	if (BurstNumber>1) printf("Capturing %i frames in burst.\n",BurstNumber);
#if 0
	/* sleep in case of exposure longer than 1/100 */
	if (ExposureTime>10) {
	    printf ("sleeping %i miliseconds\n", ExposureTime);
	    usleep (ExposureTime*1000);
	}
#endif

	while (num>0) {
	    /* perform direct capture */
	    result=ptp_nikon_directcapture (&params, 0xffffffff);
	    if (result!=PTP_RC_OK) {
	        if (result==PTP_ERROR_IO) {
	    	close_camera(&ptp_usb, &params, dev);
	    	return;
	        }
	    }

#if 0	    /* Is this really needed??? */
	    ptp_nikon_keepalive(&params);
#endif
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);
	    ptp_nikon_keepalive(&params);

	    result=ptp_nikon_checkevent (&params, &events, &nevent);
	    if (result != PTP_RC_OK) goto out;
	    
	    for(i=0;i<nevent;i++) {
		printf("Event [%i] = 0x%04x,\t param: %08x\n",i, events[i].code ,events[i].param1);
		if (events[i].code==PTP_EC_NIKON_ObjectReady) 
		{
		    num--;
		    if ((result=ptp_getobjectinfo(&params,0xffff0001, &oi))!=PTP_RC_OK) {
		        fprintf(stderr, "Could not get object info\n");
		        ptp_perror(&params,result);
		        goto out;
		    }
		    if (filename==NULL) filename=oi.Filename;
		    save_object(&params, 0xffff0001, filename, oi, overwrite);
		}
		if (events[i].code==PTP_EC_NIKON_CaptureOverflow) {
		    printf("Ram cache overflow, capture terminated\n");
		    //BurstNumber=0;
		}
	    }
	    free (events);
	}

out:	
	ptpcam_usb_timeout=USB_TIMEOUT;
	close_camera(&ptp_usb, &params, dev);
#endif
}




void
list_files (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	int i;
	PTPObjectInfo oi;
	struct tm *tm;

	printf("\nListing files...\n");
	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
	printf("Camera: %s\n",params.deviceinfo.Model);
	CR(ptp_getobjecthandles (&params,0xffffffff, 0x000000, 0x000000,
		&params.handles),"Could not get object handles\n");
	printf("Handler:           Size: \tCaptured:      \tname:\n");
	for (i = 0; i < params.handles.n; i++) {
		CR(ptp_getobjectinfo(&params,params.handles.Handler[i],
			&oi),"Could not get object info\n");
		if (oi.ObjectFormat == PTP_OFC_Association)
			continue;
		tm=gmtime(&oi.CaptureDate);
		printf("0x%08lx: %12u\t%4i-%02i-%02i %02i:%02i\t%s\n",
			(long unsigned)params.handles.Handler[i],
			(unsigned) oi.ObjectCompressedSize, 
			tm->tm_year+1900, tm->tm_mon+1,tm->tm_mday,
			tm->tm_hour, tm->tm_min,
			oi.Filename);
	}
	printf("\n");
	close_camera(&ptp_usb, &params, dev);
}

void
delete_object (int busn, int devn, short force, uint32_t handle)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPObjectInfo oi;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
	CR(ptp_getobjectinfo(&params,handle,&oi),
		"Could not get object info\n");
	CR(ptp_deleteobject(&params, handle,0), "Could not delete object\n");
	printf("\nObject 0x%08lx (%s) deleted.\n",(long unsigned) handle, oi.Filename);
	close_camera(&ptp_usb, &params, dev);
}

void
delete_all_files (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPObjectInfo oi;
	uint32_t handle;
	int i;
	int ret;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
	printf("Camera: %s\n",params.deviceinfo.Model);
	CR(ptp_getobjecthandles (&params,0xffffffff, 0x000000, 0x000000,
		&params.handles),"Could not get object handles\n");

	for (i=0; i<params.handles.n; i++) {
		handle=params.handles.Handler[i];
		if ((ret=ptp_getobjectinfo(&params,handle, &oi))!=PTP_RC_OK){
			fprintf(stderr,"Handle: 0x%08lx\n",(long unsigned) handle);
			fprintf(stderr,"ERROR: Could not get object info\n");
			ptp_perror(&params,ret);
			if (ret==PTP_ERROR_IO) clear_stall(&ptp_usb);
			continue;
		}
		if (oi.ObjectFormat == PTP_OFC_Association)
			continue;
		CR(ptp_deleteobject(&params, handle,0),
				"Could not delete object\n");
		printf("Object 0x%08lx (%s) deleted.\n",(long unsigned) handle, oi.Filename);
	}
	close_camera(&ptp_usb, &params, dev);
}

void
save_object(PTPParams *params, uint32_t handle, char* filename, PTPObjectInfo oi, int overwrite)
{
	int file;
	char *image;
	int ret;
	struct utimbuf timebuf;

#ifdef WIN32
        goto out;
#else
	file=open(filename, (overwrite==OVERWRITE_EXISTING?0:O_EXCL)|O_RDWR|O_CREAT|O_TRUNC,S_IRWXU|S_IRGRP);
#endif
	if (file==-1) {
		if (errno==EEXIST) {
			printf("Skipping file: \"%s\", file exists!\n",filename);
			goto out;
		}
		perror("open");
		goto out;
	}
	lseek(file,oi.ObjectCompressedSize-1,SEEK_SET);
	ret=write(file,"",1);
	if (ret==-1) {
	    perror("write");
	    goto out;
	}
#ifndef WIN32
	image=mmap(0,oi.ObjectCompressedSize,PROT_READ|PROT_WRITE,MAP_SHARED,
		file,0);
	if (image==MAP_FAILED) {
		perror("mmap");
		close(file);
		goto out;
	}
#endif
	printf ("Saving file: \"%s\" ",filename);
	fflush(NULL);
	ret=ptp_getobject(params,handle,&image);
	munmap(image,oi.ObjectCompressedSize);
	if (close(file)==-1) {
	    perror("close");
	}
	timebuf.actime=oi.ModificationDate;
	timebuf.modtime=oi.CaptureDate;
	utime(filename,&timebuf);
	if (ret!=PTP_RC_OK) {
		printf ("error!\n");
		ptp_perror(params,ret);
		if (ret==PTP_ERROR_IO) clear_stall((PTP_USB *)(params->data));
	} else {
		printf("is done.\n");
	}
out:
	return;
}

void
get_save_object (PTPParams *params, uint32_t handle, char* filename, int overwrite)
{

	PTPObjectInfo oi;
	int ret;

    	memset(&oi, 0, sizeof(PTPObjectInfo));
	if (verbose)
		printf ("Handle: 0x%08lx\n",(long unsigned) handle);
	if ((ret=ptp_getobjectinfo(params,handle, &oi))!=PTP_RC_OK) {
	    fprintf(stderr, "Could not get object info\n");
	    ptp_perror(params,ret);
	    if (ret==PTP_ERROR_IO) clear_stall((PTP_USB *)(params->data));
	    goto out;
	}
	if (oi.ObjectFormat == PTP_OFC_Association)
			goto out;
	if (filename==NULL) filename=(oi.Filename);

	save_object(params, handle, filename, oi, overwrite);
out:
	return;

}

void
get_file (int busn, int devn, short force, uint32_t handle, char* filename,
int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
	printf("Camera: %s\n",params.deviceinfo.Model);

	get_save_object(&params, handle, filename, overwrite);

	close_camera(&ptp_usb, &params, dev);

}

void
get_all_files (int busn, int devn, short force, int overwrite)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	int i;

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
	printf("Camera: %s\n",params.deviceinfo.Model);

	CR(ptp_getobjecthandles (&params,0xffffffff, 0x000000, 0x000000,
		&params.handles),"Could not get object handles\n");

	for (i=0; i<params.handles.n; i++) {
	    get_save_object (&params, params.handles.Handler[i], NULL,
		    overwrite);
	}
	close_camera(&ptp_usb, &params, dev);
}

void
list_operations (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	int i;
	const char* name;

	printf("\nListing supported operations...\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
	printf("Camera: %s\n",params.deviceinfo.Model);
	for (i=0; i<params.deviceinfo.OperationsSupported_len; i++)
	{
		name=ptp_get_operation_name(&params,
			params.deviceinfo.OperationsSupported[i]);

		if (name==NULL)
			printf("  0x%04x: UNKNOWN\n",
				params.deviceinfo.OperationsSupported[i]);
		else
			printf("  0x%04x: %s\n",
				params.deviceinfo.OperationsSupported[i],name);
	}
	close_camera(&ptp_usb, &params, dev);

}

void
list_properties (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	const char* propname;
	int i;

	printf("\nListing properties...\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;
/* XXX */
#if 0
	CR(ptp_nikon_setcontrolmode(&params, 0x01),
		"Unable to set Nikon PC controll mode\n");
#endif
	printf("Camera: %s\n",params.deviceinfo.Model);
	for (i=0; i<params.deviceinfo.DevicePropertiesSupported_len;i++){
		propname=ptp_prop_getname(&params,
			params.deviceinfo.DevicePropertiesSupported[i]);
		if (propname!=NULL) 
			printf("  0x%04x: %s\n",
				params.deviceinfo.DevicePropertiesSupported[i],
				propname);
		else
			printf("  0x%04x: UNKNOWN\n",
				params.deviceinfo.DevicePropertiesSupported[i]);
	}
	close_camera(&ptp_usb, &params, dev);
}

short
print_propval (uint16_t datatype, void* value, short hex);
short
print_propval (uint16_t datatype, void* value, short hex)
{
	switch (datatype) {
		case PTP_DTC_INT8:
			printf("%hhi",*(char*)value);
			return 0;
		case PTP_DTC_UINT8:
			printf("%hhu",*(unsigned char*)value);
			return 0;
		case PTP_DTC_INT16:
			printf("%hi",*(int16_t*)value);
			return 0;
		case PTP_DTC_UINT16:
			if (hex==PTPCAM_PRINT_HEX)
				printf("0x%04hX (%hi)",*(uint16_t*)value,
					*(uint16_t*)value);
			else
				printf("%hi",*(uint16_t*)value);
			return 0;
		case PTP_DTC_INT32:
			printf("%li",(long int)*(int32_t*)value);
			return 0;
		case PTP_DTC_UINT32:
			if (hex==PTPCAM_PRINT_HEX)
				printf("0x%08lX (%lu)",
					(long unsigned) *(uint32_t*)value,
					(long unsigned) *(uint32_t*)value);
			else
				printf("%lu",(long unsigned)*(uint32_t*)value);
			return 0;
		case PTP_DTC_STR:
			printf("\"%s\"",(char *)value);
	}
	return -1;
}

uint16_t
set_property (PTPParams* params,
		uint16_t property, const char* value, uint16_t datatype);
uint16_t
set_property (PTPParams* params,
		uint16_t property, const char* value, uint16_t datatype)
{
	void* val=NULL;

	switch(datatype) {
	case PTP_DTC_INT8:
		val=malloc(sizeof(int8_t));
		*(int8_t*)val=(int8_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_UINT8:
		val=malloc(sizeof(uint8_t));
		*(uint8_t*)val=(uint8_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_INT16:
		val=malloc(sizeof(int16_t));
		*(int16_t*)val=(int16_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_UINT16:
		val=malloc(sizeof(uint16_t));
		*(uint16_t*)val=(uint16_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_INT32:
		val=malloc(sizeof(int32_t));
		*(int32_t*)val=(int32_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_UINT32:
		val=malloc(sizeof(uint32_t));
		*(uint32_t*)val=(uint32_t)strtol(value,NULL,0);
		break;
	case PTP_DTC_STR:
		val=(void *)value;
	}
	return(ptp_setdevicepropvalue(params, property, val, datatype));
	free(val);
	return 0;
}
void
getset_property_internal (PTPParams* params, uint16_t property,const char* value);
void
getset_property_internal (PTPParams* params, uint16_t property,const char* value)
{
	PTPDevicePropDesc dpd;
	const char* propname;
	const char *propdesc;
	uint16_t result;


	memset(&dpd,0,sizeof(dpd));
	result=ptp_getdevicepropdesc(params,property,&dpd);
	if (result!=PTP_RC_OK) {
		ptp_perror(params,result); 
		fprintf(stderr,"ERROR: "
		"Could not get device property description!\n"
		"Try to reset the camera.\n");
		return ;
	}
	/* until this point dpd has to be free()ed */
	propdesc=ptp_prop_getdesc(params, &dpd, NULL);
	propname=ptp_prop_getname(params,property);

	if (value==NULL) { /* property GET */
		if (!verbose) { /* short output, default */
			printf("'%s' is set to: ", propname==NULL?"UNKNOWN":propname);
			if (propdesc!=NULL)
				printf("[%s]", propdesc);
			else 
			{
				if (dpd.FormFlag==PTP_DPFF_Enumeration)
					PRINT_PROPVAL_HEX(dpd.CurrentValue);
				else 
					PRINT_PROPVAL_DEC(dpd.CurrentValue);
			}
			printf("\n");
	
		} else { /* verbose output */
	
			printf("%s: [0x%04x, ",propname==NULL?"UNKNOWN":propname,
					property);
			if (dpd.GetSet==PTP_DPGS_Get)
				printf ("readonly, ");
			else
				printf ("readwrite, ");
			printf ("%s] ",
				ptp_get_datatype_name(params, dpd.DataType));

			printf ("\n  Current value: ");
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else 
				PRINT_PROPVAL_DEC(dpd.CurrentValue);

			if (propdesc!=NULL)
				printf(" [%s]", propdesc);
			printf ("\n  Factory value: ");
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.FactoryDefaultValue);
			else 
				PRINT_PROPVAL_DEC(dpd.FactoryDefaultValue);
			propdesc=ptp_prop_getdesc(params, &dpd,
						dpd.FactoryDefaultValue);
			if (propdesc!=NULL)
				printf(" [%s]", propdesc);
			printf("\n");

			switch (dpd.FormFlag) {
			case PTP_DPFF_Enumeration:
				{
					int i;
					printf ("Enumerated:\n");
					for(i=0;i<dpd.FORM.Enum.NumberOfValues;i++){
						PRINT_PROPVAL_HEX(
						dpd.FORM.Enum.SupportedValue[i]);
						propdesc=ptp_prop_getdesc(params, &dpd, dpd.FORM.Enum.SupportedValue[i]);
						if (propdesc!=NULL) printf("\t[%s]", propdesc);
						printf("\n");
					}
				}
				break;
			case PTP_DPFF_Range:
				printf ("Range [");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MinimumValue);
				printf(" - ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MaximumValue);
				printf("; step ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.StepSize);
				printf("]\n");
				break;
			case PTP_DPFF_None:
				break;
			}
		}
	} else {
		uint16_t r;
		propdesc=ptp_prop_getdesc(params, &dpd, NULL);
		printf("'%s' is set to: ", propname==NULL?"UNKNOWN":propname);
		if (propdesc!=NULL)
			printf("[%s]", propdesc);
		else
		{
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else 
				PRINT_PROPVAL_DEC(dpd.CurrentValue);
		}
		printf("\n");

		propdesc=ptp_prop_getdescbystring(params, &dpd, value);
/*
		if (propdesc==NULL) {
			fprintf(stderr, "ERROR: Unable to set property to unidentified value: '%s'\n",
				value);
			goto out;
		}
*/
		printf("Changing property value to %s [%s] ",
			value,propdesc);
		r=(set_property(params, property, value, dpd.DataType));
		if (r!=PTP_RC_OK)
		{
			printf ("FAILED!!!\n");
			fflush(NULL);
		        ptp_perror(params,r);
		}
		else 
			printf ("succeeded.\n");
	}
/*	out: */

	ptp_free_devicepropdesc(&dpd);
}

void
getset_propertybyname (int busn,int devn,char* property,char* value,short force);
void
getset_propertybyname (int busn,int devn,char* property,char* value,short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	char *p;
	uint16_t dpc;
	const char *propval=NULL;

	printf("\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	printf("Camera: %s",params.deviceinfo.Model);
	if ((devn!=0)||(busn!=0)) 
		printf(" (bus %i, dev %i)\n",busn,devn);
	else
		printf("\n");

	if (property==NULL) {
		fprintf(stderr,"ERROR: no such property\n");
		return;
	}
		
	/* 
	1. change all '-' in property and value to ' '
	2. change all '  ' in property and value to '-'
	3. get property code by name
	4. get value code by name
	5. set property
	*/
	while ((p=strchr(property,'-'))!=NULL) {
		*p=' ';
	}

	dpc=ptp_prop_getcodebyname(&params, property);
	if (dpc==0) {
		fprintf(stderr, "ERROR: Could not find property '%s'\n",
			property);
		close_camera(&ptp_usb, &params, dev);
		return;
	}

	if (!ptp_property_issupported(&params, dpc))
	{
		fprintf(stderr,"The device does not support this property!\n");
		close_camera(&ptp_usb, &params, dev);
		return;
	}

	if (value!=NULL) {
		while ((p=strchr(value,'-'))!=NULL) {
			*p=' ';
		}
		propval=ptp_prop_getvalbyname(&params, value, dpc);
		if (propval==NULL) propval=value;
	}

	getset_property_internal (&params, dpc,propval);

	close_camera(&ptp_usb, &params, dev);
}
void
getset_property (int busn,int devn,uint16_t property,char* value,short force);
void
getset_property (int busn,int devn,uint16_t property,char* value,short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;

	printf ("\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	printf("Camera: %s",params.deviceinfo.Model);
	if ((devn!=0)||(busn!=0)) 
		printf(" (bus %i, dev %i)\n",busn,devn);
	else
		printf("\n");
	if (!ptp_property_issupported(&params, property))
	{
		fprintf(stderr,"The device does not support this property!\n");
		close_camera(&ptp_usb, &params, dev);
		return;
	}

	getset_property_internal (&params, property,value);
#if 0
	memset(&dpd,0,sizeof(dpd));
	CR(ptp_getdevicepropdesc(&params,property,&dpd),
		"Could not get device property description!\n"
		"Try to reset the camera.\n");
	propdesc= ptp_prop_getdesc(&params, &dpd, NULL);
	propname=ptp_prop_getname(&params,property);

	if (value==NULL) { /* property GET */
		if (!verbose) { /* short output, default */
			printf("'%s' is set to: ", propname==NULL?"UNKNOWN":propname);
			if (propdesc!=NULL)
				printf("%s [%s]", ptp_prop_tostr(&params, &dpd,
							NULL), propdesc);
			else 
			{
				if (dpd.FormFlag==PTP_DPFF_Enumeration)
					PRINT_PROPVAL_HEX(dpd.CurrentValue);
				else 
					PRINT_PROPVAL_DEC(dpd.CurrentValue);
			}
			printf("\n");
	
		} else { /* verbose output */
	
			printf("%s: [0x%04x, ",propname==NULL?"UNKNOWN":propname,
					property);
			if (dpd.GetSet==PTP_DPGS_Get)
				printf ("readonly, ");
			else
				printf ("readwrite, ");
			printf ("%s] ",
				ptp_get_datatype_name(&params, dpd.DataType));

			printf ("\n  Current value: ");
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else 
				PRINT_PROPVAL_DEC(dpd.CurrentValue);

			if (propdesc!=NULL)
				printf(" [%s]", propdesc);
			printf ("\n  Factory value: ");
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.FactoryDefaultValue);
			else 
				PRINT_PROPVAL_DEC(dpd.FactoryDefaultValue);
			propdesc=ptp_prop_getdesc(&params, &dpd,
						dpd.FactoryDefaultValue);
			if (propdesc!=NULL)
				printf(" [%s]", propdesc);
			printf("\n");

			switch (dpd.FormFlag) {
			case PTP_DPFF_Enumeration:
				{
					int i;
					printf ("Enumerated:\n");
					for(i=0;i<dpd.FORM.Enum.NumberOfValues;i++){
						PRINT_PROPVAL_HEX(
						dpd.FORM.Enum.SupportedValue[i]);
						propdesc=ptp_prop_getdesc(&params, &dpd, dpd.FORM.Enum.SupportedValue[i]);
						if (propdesc!=NULL) printf("\t[%s]", propdesc);
						printf("\n");
					}
				}
				break;
			case PTP_DPFF_Range:
				printf ("Range [");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MinimumValue);
				printf(" - ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.MaximumValue);
				printf("; step ");
				PRINT_PROPVAL_DEC(dpd.FORM.Range.StepSize);
				printf("]\n");
				break;
			case PTP_DPFF_None:
				break;
			}
		}
	} else {
		uint16_t r;
		propdesc= ptp_prop_getdesc(&params, &dpd, NULL);
		printf("'%s' is set to: ", propname==NULL?"UNKNOWN":propname);
		if (propdesc!=NULL)
			printf("%s [%s]", ptp_prop_tostr(&params, &dpd, NULL), propdesc);
		else
		{
			if (dpd.FormFlag==PTP_DPFF_Enumeration)
				PRINT_PROPVAL_HEX(dpd.CurrentValue);
			else 
				PRINT_PROPVAL_DEC(dpd.CurrentValue);
		}
		printf("\n");
		printf("Changing property value to '%s' ",value);
		r=(set_property(&params, property, value, dpd.DataType));
		if (r!=PTP_RC_OK)
		{
			printf ("FAILED!!!\n");
			fflush(NULL);
		        ptp_perror(&params,r);
		}
		else 
			printf ("succeeded.\n");
	}
	ptp_free_devicepropdesc(&dpd);
#endif

	close_camera(&ptp_usb, &params, dev);
}

void
show_all_properties (int busn,int devn,short force, int unknown);
void
show_all_properties (int busn,int devn,short force, int unknown)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	PTPDevicePropDesc dpd;
	const char* propname;
	const char *propdesc;
	int i;

	printf ("\n");

	if (open_camera(busn, devn, force, &ptp_usb, &params, &dev)<0)
		return;

	printf("Camera: %s",params.deviceinfo.Model);
	if ((devn!=0)||(busn!=0)) 
		printf(" (bus %i, dev %i)\n",busn,devn);
	else
		printf("\n");

	for (i=0; i<params.deviceinfo.DevicePropertiesSupported_len;i++) {
		propname=ptp_prop_getname(&params,
				params.deviceinfo.DevicePropertiesSupported[i]);
		if ((unknown) && (propname!=NULL)) continue;

		printf("0x%04x: ",
				params.deviceinfo.DevicePropertiesSupported[i]);
		memset(&dpd,0,sizeof(dpd));
		CR(ptp_getdevicepropdesc(&params,
			params.deviceinfo.DevicePropertiesSupported[i],&dpd),
			"Could not get device property description!\n"
			"Try to reset the camera.\n");
		propdesc= ptp_prop_getdesc(&params, &dpd, NULL);

		PRINT_PROPVAL_HEX(dpd.CurrentValue);
		if (verbose) {
			printf (" (%s",propname==NULL?"UNKNOWN":propname);
			if (propdesc!=NULL)
				printf(": %s)",propdesc);
			else
				printf(")");
		}
	
		printf("\n");
		ptp_free_devicepropdesc(&dpd);
	}

	close_camera(&ptp_usb, &params, dev);
}

int
usb_get_endpoint_status(PTP_USB* ptp_usb, int ep, uint16_t* status)
{
	 return (usb_control_msg(ptp_usb->handle,
		USB_DP_DTH|USB_RECIP_ENDPOINT, USB_REQ_GET_STATUS,
		USB_FEATURE_HALT, ep, (char *)status, 2, 3000));
}

int
usb_clear_stall_feature(PTP_USB* ptp_usb, int ep)
{

	return (usb_control_msg(ptp_usb->handle,
		USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE, USB_FEATURE_HALT,
		ep, NULL, 0, 3000));
}

int
usb_ptp_get_device_status(PTP_USB* ptp_usb, uint16_t* devstatus);
int
usb_ptp_get_device_status(PTP_USB* ptp_usb, uint16_t* devstatus)
{
	return (usb_control_msg(ptp_usb->handle,
		USB_DP_DTH|USB_TYPE_CLASS|USB_RECIP_INTERFACE,
		USB_REQ_GET_DEVICE_STATUS, 0, 0,
		(char *)devstatus, 4, 3000));
}

int
usb_ptp_device_reset(PTP_USB* ptp_usb);
int
usb_ptp_device_reset(PTP_USB* ptp_usb)
{
	return (usb_control_msg(ptp_usb->handle,
		USB_TYPE_CLASS|USB_RECIP_INTERFACE,
		USB_REQ_DEVICE_RESET, 0, 0, NULL, 0, 3000));
}

void
reset_device (int busn, int devn, short force);
void
reset_device (int busn, int devn, short force)
{
	PTPParams params;
	PTP_USB ptp_usb;
	struct usb_device *dev;
	uint16_t status;
	uint16_t devstatus[2] = {0,0};
	int ret;

#ifdef DEBUG
	printf("dev %i\tbus %i\n",devn,busn);
#endif
	dev=find_device(busn,devn,force);
	if (dev==NULL) {
		fprintf(stderr,"could not find any device matching given "
		"bus/dev numbers\n");
		exit(-1);
	}
	find_endpoints(dev,&ptp_usb.inep,&ptp_usb.outep,&ptp_usb.intep);

	init_ptp_usb(&params, &ptp_usb, dev);
	
	/* get device status (devices likes that regardless of its result)*/
	usb_ptp_get_device_status(&ptp_usb,devstatus);
	
	/* check the in endpoint status*/
	ret = usb_get_endpoint_status(&ptp_usb,ptp_usb.inep,&status);
	if (ret<0) perror ("usb_get_endpoint_status()");
	/* and clear the HALT condition if happend*/
	if (status) {
		printf("Resetting input pipe!\n");
		ret=usb_clear_stall_feature(&ptp_usb,ptp_usb.inep);
		if (ret<0)perror ("usb_clear_stall_feature()");
	}
	status=0;
	/* check the out endpoint status*/
	ret = usb_get_endpoint_status(&ptp_usb,ptp_usb.outep,&status);
	if (ret<0) perror ("usb_get_endpoint_status()");
	/* and clear the HALT condition if happend*/
	if (status) {
		printf("Resetting output pipe!\n");
		ret=usb_clear_stall_feature(&ptp_usb,ptp_usb.outep);
		if (ret<0)perror ("usb_clear_stall_feature()");
	}
	status=0;
	/* check the interrupt endpoint status*/
	ret = usb_get_endpoint_status(&ptp_usb,ptp_usb.intep,&status);
	if (ret<0)perror ("usb_get_endpoint_status()");
	/* and clear the HALT condition if happend*/
	if (status) {
		printf ("Resetting interrupt pipe!\n");
		ret=usb_clear_stall_feature(&ptp_usb,ptp_usb.intep);
		if (ret<0)perror ("usb_clear_stall_feature()");
	}

	/* get device status (now there should be some results)*/
	ret = usb_ptp_get_device_status(&ptp_usb,devstatus);
	if (ret<0) 
		perror ("usb_ptp_get_device_status()");
	else	{
		if (devstatus[1]==PTP_RC_OK) 
			printf ("Device status OK\n");
		else
			printf ("Device status 0x%04x\n",devstatus[1]);
	}
	
	/* finally reset the device (that clears prevoiusly opened sessions)*/
	ret = usb_ptp_device_reset(&ptp_usb);
	if (ret<0)perror ("usb_ptp_device_reset()");
	/* get device status (devices likes that regardless of its result)*/
	usb_ptp_get_device_status(&ptp_usb,devstatus);

	close_usb(&ptp_usb, dev);

}

/* main program  */

int chdk(int busn, int devn, short force);
uint8_t chdkmode=0;
char chdkarg[CHDKBUFS];

int
main(int argc, char ** argv)
{
	int busn=0,devn=0;
	int action=0;
	short force=0;
	int overwrite=SKIP_IF_EXISTS;
	uint16_t property=0;
	char* value=NULL;
	char* propstr=NULL;
	uint32_t handle=0;
	char *filename=NULL;
	int num=0;
	/* parse options */
	int option_index = 0,opt;
	static struct option loptions[] = {
		{"help",0,0,'h'},
		{"bus",1,0,0},
		{"dev",1,0,0},
		{"reset",0,0,'r'},
		{"list-devices",0,0,'l'},
		{"list-files",0,0,'L'},
		{"list-operations",1,0,'o'},
		{"list-properties",0,0,'p'},
		{"show-all-properties",0,0,0},
		{"show-unknown-properties",0,0,0},
		{"show-property",1,0,'s'},
		{"set-property",1,0,'s'},
		{"set",1,0,0},
		{"get-file",1,0,'g'},
		{"get-all-files",0,0,'G'},
		{"capture",0,0,'c'},
		{"nikon-dc",0,0,0},
		{"ndc",0,0,0},
		{"nikon-ic",0,0,0},
		{"nic",0,0,0},
		{"nikon-dc2",0,0,0},
		{"ndc2",0,0,0},
		{"loop-capture",1,0,0},
		{"delete-object",1,0,'d'},
		{"delete-all-files",1,0,'D'},
		{"info",0,0,'i'},
		{"val",1,0,0},
		{"filename",1,0,0},
		{"overwrite",0,0,0},
		{"force",0,0,'f'},
		{"verbose",2,0,'v'},
		{"chdk",2,0,0},
		{0,0,0,0}
	};

	/* register signal handlers */
	signal(SIGINT, ptpcam_siginthandler);
	
	while(1) {
		opt = getopt_long (argc, argv, "LhlcipfroGg:Dd:s:v::", loptions, &option_index);
		if (opt==-1) break;
	
		switch (opt) {
		/* set parameters */
		case 0:
			if (!(strcmp("chdk",loptions[option_index].name)))
			{
				action=ACT_CHDK;
				if (optarg) {
					chdkmode=CHDK_MODE_CLI;
					strncpy(chdkarg, optarg, CHDKBUFS-1);
					chdkarg[CHDKBUFS-1]='\0';
				} else {
					chdkmode=CHDK_MODE_INTERACTIVE;
				}
			}
			if (!(strcmp("val",loptions[option_index].name)))
				value=strdup(optarg);
			if (!(strcmp("filename",loptions[option_index].name)))
				filename=strdup(optarg);
			if (!(strcmp("overwrite",loptions[option_index].name)))
				overwrite=OVERWRITE_EXISTING;
			if (!(strcmp("bus",loptions[option_index].name)))
				busn=strtol(optarg,NULL,10);
			if (!(strcmp("dev",loptions[option_index].name)))
				devn=strtol(optarg,NULL,10);
			if (!(strcmp("loop-capture",loptions[option_index].name)))
			{
				action=ACT_LOOP_CAPTURE;
				num=strtol(optarg,NULL,10);
			}
			if (!(strcmp("show-all-properties", loptions[option_index].name)))
				action=ACT_SHOW_ALL_PROPERTIES;
			if (!(strcmp("show-unknown-properties", loptions[option_index].name)))
				action=ACT_SHOW_UNKNOWN_PROPERTIES;
			if (!(strcmp("set",loptions[option_index].name)))
			{
				propstr=strdup(optarg);
				action=ACT_SET_PROPBYNAME;
			}
			if (!strcmp("nikon-dc", loptions[option_index].name) ||
			    !strcmp("ndc", loptions[option_index].name))
			{
			    action=ACT_NIKON_DC;
			}
			if (!strcmp("nikon-ic", loptions[option_index].name) ||
			    !strcmp("nic", loptions[option_index].name))
			{
			    action=ACT_NIKON_IC;
			}
			if (!strcmp("nikon-dc2", loptions[option_index].name) ||
			    !strcmp("ndc2", loptions[option_index].name))
			{
			    action=ACT_NIKON_DC2;
			}
			break;
		case 'f':
			force=~force;
			break;
		case 'v':
			if (optarg) 
				verbose=strtol(optarg,NULL,10);
			else
				verbose=1;
			/*printf("VERBOSE LEVEL  = %i\n",verbose);*/
			break;
		/* actions */
		case 'h':
			help();
			break;
		case 'r':
			action=ACT_DEVICE_RESET;
			break;
		case 'l':
			action=ACT_LIST_DEVICES;
			break;
		case 'p':
			action=ACT_LIST_PROPERTIES;
			break;
		case 's':
			action=ACT_GETSET_PROPERTY;
			property=strtol(optarg,NULL,16);
			break;
		case 'o':
			action=ACT_LIST_OPERATIONS;
			break;
		case 'i':
			action=ACT_SHOW_INFO;
			break;
		case 'c':
			action=ACT_CAPTURE;
			break;
		case 'L':
			action=ACT_LIST_FILES;
			break;
		case 'g':
			action=ACT_GET_FILE;
			handle=strtol(optarg,NULL,16);
			break;
		case 'G':
			action=ACT_GET_ALL_FILES;
			break;
		case 'd':
			action=ACT_DELETE_OBJECT;
			handle=strtol(optarg,NULL,16);
			break;
		case 'D':
			action=ACT_DELETE_ALL_FILES;
		case '?':
			break;
		default:
			fprintf(stderr,"getopt returned character code 0%o\n",
				opt);
			break;
		}
	}
	if (argc==1) {
		usage();
		return 0;
	}
	switch (action) {
                case ACT_CHDK:
                        chdk(busn,devn,force);
                        break;
		case ACT_DEVICE_RESET:
			reset_device(busn,devn,force);
			break;
		case ACT_LIST_DEVICES:
			list_devices(force);
			break;
		case ACT_LIST_PROPERTIES:
			list_properties(busn,devn,force);
			break;
		case ACT_GETSET_PROPERTY:
			getset_property(busn,devn,property,value,force);
			break;
		case ACT_SHOW_INFO:
			show_info(busn,devn,force);
			break;
		case ACT_LIST_OPERATIONS:
			list_operations(busn,devn,force);
			break;
		case ACT_LIST_FILES:
			list_files(busn,devn,force);
			break;
		case ACT_GET_FILE:
			get_file(busn,devn,force,handle,filename,overwrite);
			break;
		case ACT_GET_ALL_FILES:
			get_all_files(busn,devn,force,overwrite);
			break;
		case ACT_CAPTURE:
			capture_image(busn,devn,force);
			break;
		case ACT_DELETE_OBJECT:
			delete_object(busn,devn,force,handle);
			break;
		case ACT_DELETE_ALL_FILES:
			delete_all_files(busn,devn,force);
			break;
		case ACT_LOOP_CAPTURE:
			loop_capture (busn,devn,force,num,overwrite);
			break;
		case ACT_SHOW_ALL_PROPERTIES:
			show_all_properties(busn,devn,force,0);
			break;
		case ACT_SHOW_UNKNOWN_PROPERTIES:
			show_all_properties(busn,devn,force,1);
			break;
		case ACT_SET_PROPBYNAME:
			getset_propertybyname(busn,devn,propstr,value,force);
			break;
		case ACT_NIKON_DC:
			nikon_direct_capture(busn,devn,force,filename,overwrite);
			break;
		case ACT_NIKON_IC:
			nikon_initiate_dc (busn,devn,force);
			break;
		case ACT_NIKON_DC2:
			nikon_direct_capture2(busn,devn,force,filename,overwrite);
	}

	return 0;

}





static int camera_bus = 0;
static int camera_dev = 0;
static int camera_force = 0;
static PTP_USB ptp_usb;
static PTPParams params;
static struct usb_device *dev;
static int connected = 0;

static void open_connection()
{
  connected = (0 == open_camera(camera_bus,camera_dev,camera_force,&ptp_usb,&params,&dev));
  if ( connected )
  {
    int major,minor;
    if ( !ptp_chdk_get_version(&params,&params.deviceinfo,&major,&minor) )
    {
      printf("error: cannot get camera CHDK PTP version; either it has an unsupported version or no CHDK PTP support at all\n");
    } else if ( major != PTP_CHDK_VERSION_MAJOR || minor < PTP_CHDK_VERSION_MINOR )
    {
      printf("error: camera has unsupported camera version %i.%i; some functionality may be missing or cause unintented consequences\n",major,minor);
    }
  }
}

static void close_connection()
{
  close_camera(&ptp_usb,&params,dev);
}


int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

static void reset_connection()
{
  if ( connected )
  {
    close_connection();
  }
  open_connection();
}

static void print_safe(char *buf, int size)
{
  int i;
  for (i=0; i<size; i++)
  {
    if ( buf[i] < ' ' || buf[i] > '~' )
    {
      printf(".");
    } else {
      printf("%c",buf[i]);
    }
  }
}

static void hexdump(char *buf, unsigned int size, unsigned int offset)
{
  unsigned int start_offset = offset;
  unsigned int i;
  char s[16];

  if ( offset % 16 != 0 )
  {
      printf("0x%08X (+0x%04X)  ",offset, offset-start_offset);
      for (i=0; i<(offset%16); i++)
      {
        printf("   ");
      }
      if ( offset % 16 > 8 )
      {
        printf(" ");
      }
      memset(s,' ',offset%16);
  }
  for (i=0; ; i++, offset++)
  {
    if ( offset % 16 == 0 )
    {
      if ( i > 0 )
      {
        printf(" |");
        print_safe(s,16);
        printf("|\n");
      }
      printf("0x%08X (+0x%04X)",offset, offset-start_offset);
      if (i < size)
      {
        printf(" ");
      }
    }
    if ( offset % 8 == 0 )
    {
      printf(" ");
    }
    if ( i == size )
    {
      break;
    }
    printf("%02x ",(unsigned char) buf[i]);
    s[offset%16] = buf[i];
  }
  if ( offset % 16 != 0 )
  {
      for (i=0; i<16-(offset%16); i++)
      {
        printf("   ");
      }
      if ( offset % 16 < 8 )
      {
        printf(" ");
      }
      memset(s+(offset%16),' ',16-(offset%16));
      printf(" |");
      print_safe(s,16);
      printf("|\n%08x",offset);
  }
  printf("\n");
}

static void hexdump4(char *buf, unsigned int size, unsigned int offset)
{
  unsigned int i;
  char s[16];

  if ( offset % 16 != 0 )
  {
      printf("%08x  ",offset);
      for (i=0; i<(offset%16); i++)
      {
        printf("   ");
      }
      if ( offset % 16 > 8 )
      {
        printf(" ");
      }
      memset(s,' ',offset%16);
  }
  for (i=0; ; i+=4, offset+=4)
  {
    if ( offset % 32 == 0 )
    {
      if ( i > 0 )
      {
        printf("\n");
      }
      printf("%08x",offset);
      if (i < size)
      {
        printf(" ");
      }
    }
    if ( i == size )
    {
      break;
    }
    printf("%02x",(unsigned char) buf[i+3]);
    printf("%02x",(unsigned char) buf[i+2]);
    printf("%02x",(unsigned char) buf[i+1]);
    printf("%02x ",(unsigned char) buf[i]);
  }
  if ( offset % 16 != 0 )
  {
      printf("\n%08x",offset);
  }
  printf("\n");
}

int engio_dump(unsigned char * data_buf, int length, int addr)
{
    unsigned int reg = 0;
    unsigned int data = 0;
    unsigned int pos = 0;
    
    do
    {
        data =  data_buf[pos + 0] | (data_buf[pos + 1] << 8)  | (data_buf[pos + 2] << 16)  | (data_buf[pos + 3] << 24);
        pos += 4;
        reg = data_buf[pos + 0] | (data_buf[pos + 1] << 8)  | (data_buf[pos + 2] << 16)  | (data_buf[pos + 3] << 24);
        pos += 4;
        if(reg != 0xFFFFFFFF)
        {
            printf("[0x%08X] <- [0x%08X]\r\n", reg, data);
        }
    } while (reg != 0xFFFFFFFF && pos <= length - 8);
    
    printf("\r\n");
}

int adtg_dump(unsigned char * data_buf, int length, int addr)
{
    unsigned int reg = 0;
    unsigned int data = 0;
    unsigned int pos = 0;
    
    do
    {
        data =  data_buf[pos + 0] | (data_buf[pos + 1] << 8);
        pos += 2;
        reg = data_buf[pos + 0] | (data_buf[pos + 1] << 8);
        pos += 2;
        if(reg != 0xFFFF && data != 0xFFFF)
        {
            printf("[0x%04X] <- [0x%04X]\r\n", reg, data);
        }
    } while (reg != 0xFFFF && data != 0xFFFF && pos <= length - 4);
    
    printf("\r\n");
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define INVALID_SOCKET -1
int gdb_port = 23946;

int accepttimeout ( int s, struct sockaddr *addr, int *addrlen, int timeout )
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = 0;
    tv.tv_usec = timeout * 1000;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return accept ( s, addr, addrlen );
}


int recvtimeout ( int s, char *buf, int len, int timeout )
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = 0;
    tv.tv_usec = timeout * 1000;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return recv ( s, buf, len, 0 );
}


unsigned int gdb_loop (int socket)
{
    char buffer[8192];
    while(1)
    {
        {
            int recvStatus = recv ( socket, buffer, 8192, MSG_DONTWAIT );//recvtimeout (socket, buffer, 8192, 1);

            if(recvStatus > 0)
            {
                buffer[recvStatus] = 0;
                printf("Download: '%s'\n", buffer);
                
                if (ptp_chdk_gdb_download(buffer,&params,&params.deviceinfo) == 0)
                {
                    printf("error sending command\n");
                    return;
                }
            }
            else if((recvStatus == EAGAIN ) || (recvStatus == EWOULDBLOCK))//-2)
            {
            }
            else if((recvStatus == -1 ) || (recvStatus == EWOULDBLOCK))//-2)
            {
            }
            else
            {
                printf("error %i during recvtimeout\n", recvStatus );
                return;
            }
        }
        
        /* upload */
        {
            char *buf;
            
            buf = ptp_chdk_gdb_upload(&params,&params.deviceinfo);
            
            if(buf != NULL && strlen(buf) > 0)
            {
                printf("Upload: '%s'\n", buf);
                send(socket, buf, strlen(buf), 0 );
            }
        }
    }
}

unsigned int gdb_listen ( )
{	
    struct sockaddr_in local;
    struct sockaddr_in remote;
    int remotelen = sizeof ( remote );
    int server_fd = INVALID_SOCKET;
    int client_fd = INVALID_SOCKET;

#ifdef WIN32
    if ( WSAStartup ( 0x101, &gdb_wsadata ) != 0 )
        return E_FAIL;
#endif

	local.sin_family = AF_INET; 
	local.sin_addr.s_addr = INADDR_ANY; 
	local.sin_port = htons ( (u_short)gdb_port );

	server_fd = socket ( AF_INET, SOCK_STREAM, 0 );
	if ( server_fd == INVALID_SOCKET )
		printf ("(socket error)\n");
	else if ( bind ( server_fd, (struct sockaddr*)&local, sizeof(local) ) !=0 )
		printf ("(port already used)\n" );
	else if ( listen ( server_fd, 10 ) !=0 )
		printf ("(listen error)\n");
	else
	{
		while ( 1 )
		{
			client_fd = accepttimeout ( server_fd, (struct sockaddr*)&remote, &remotelen, 500 );
			if ( client_fd >= 0 )
			{
				close ( server_fd );
				printf ("remote connected: %s\n", inet_ntoa ( remote.sin_addr )  );
				gdb_loop(client_fd);
				return 1;
			}
		}
	}
    return 0;
}


int chdk(int busn, int devn, short force)
{
  char buf[CHDKBUFS], *s;
  int i;
  buf[CHDKBUFS-1] = '\0';

  camera_bus = busn;
  camera_dev = devn;
  camera_force = force;
  
  open_connection();
  if (chdkmode==CHDK_MODE_CLI) {
    // printf("CHDK command line mode. Argument:%s\n", chdkarg);
    if (!connected) {
      printf("error: couldn't connect to camera.\n");
      return(-1);
    }
  }
  
  while (1) {
    if (chdkmode==CHDK_MODE_CLI) {
      for (i=0; i<CHDKBUFS; i++) {
        buf[i]=chdkarg[i];
      }
    } else { // CHDK_MODE_INTERACTIVE
      printf("%s ",connected?"<conn>":"<    >"); fflush(stdout);
      if ( fgets(buf,CHDKBUFS-1,stdin) == NULL )
      {
        printf("\n");
        break;
      }
    }
    s = buf+strlen(buf)-1;
    while ( s >= buf && ( *s == '\n' || *s == '\r' ) )
    {
      *s = '\0';
      s--;
    }

    if ( !strcmp("q",buf) || !strcmp("quit",buf) )
    {
      break;
      
    } else if ( !strcmp("h",buf) || !strcmp("help",buf) )
    {
      printf(
          "q quit                         quit program\n"
          "h help                         list commands\n"
          "r reset                        reconnect to camera\n"
          "  version                      get CHDK PTP version (ptpcam and camera)\n"
//          "  shutdown-hard                shutdown camera (hard)\n"
          "  shutdown                     shutdown camera (soft)\n"
          "  reboot                       reboot camera\n"
          "  reboot <filename>            reboot camera using specified firmware update\n"
          "  reboot-fi2                   reboot camera using default firmware update\n"
          "m memory <address>             get byte at address\n" 
          "m memory <address>-<address>   get bytes at given range\n" 
          "m memory <address> <num>       get num bytes at given address\n"
          "  set <address> <long>         set long value at address\n"
          "c call <address> <arg1> ...    call function at address with given arguments\n"
//          "  prop <id>                    get value of property\n"
//          "  prop <id>-<id>               get values in property range\n"
//          "  prop <id> <num>              get num values of properties starting at id\n"
//          "  param <id>                   get value of parameter\n"
//          "  param <id>-<id>              get values in parameter range\n"
//          "  param <id> <num>             get num values of parameters starting at id\n"
          "  upload <local> <remote>      upload local file to camera\n"
          "  download <remote> <local>    download file from camera\n"
          "  mode <val>                   set mode (0=playback,1=record)\n"
          "  lua <code>                   execute lua code\n"
          "  luar <code>                  execute \"return <code>\" and retreive result\n"
          "  script-support               show supported script interfaces\n"
          "  script-status                show script execution and message status\n"
          "  getm                         get messages / return values from script\n"
          "  putm <message>               send <message> to running script\n"
          "  gdbproxy                     forward gdb commands between a network socket and the camera\n"
          "  gdb s <command>              send gdb <command> to camera\n"          
          "  gdb r                        receive the response to 'gdb s'\n"
          );
      
    } else if ( !strcmp("r",buf) || !strcmp("reset",buf) )
    {
      reset_connection();

    } else if ( !strcmp("version",buf) )
    {
      int major, minor;
      printf("ptpcam: %i.%i\n",PTP_CHDK_VERSION_MAJOR,PTP_CHDK_VERSION_MINOR);
      if ( ptp_chdk_get_version(&params,&params.deviceinfo,&major,&minor) )
      {
        printf("camera: %i.%i\n",major,minor);
      }

    } else if ( !strcmp("shutdown-hard",buf) )
    {
      if ( ptp_chdk_shutdown_hard(&params,&params.deviceinfo) )
      {
        connected = 0;
      }

    } else if ( !strcmp("shutdown",buf) )
    {
      if ( ptp_chdk_shutdown_soft(&params,&params.deviceinfo) )
      {
        connected = 0;
      }

    } else if ( !strcmp("reboot",buf) )
    {
      if ( ptp_chdk_reboot(&params,&params.deviceinfo) )
      {
        connected = 0;
        sleep(2);
        open_connection();
      }

    } else if ( !strcmp("reboot-fi2",buf) )
    {
      if ( ptp_chdk_reboot_fw_update("A/PS.FI2",&params,&params.deviceinfo) )
      {
        connected = 0;
        sleep(2);
        open_connection();
      }

    } else if ( !strncmp("reboot ",buf,7) )
    {
      char *s;
      if ( (s = strchr(buf,'\r')) != NULL )
      {
        *s = '\0';
      }
      if ( (s = strchr(buf,'\n')) != NULL )
      {
        *s = '\0';
      }
      if ( ptp_chdk_reboot_fw_update(buf+7,&params,&params.deviceinfo) )
      {
        connected = 0;
        sleep(2);
        open_connection();
      }
      
    }
    else if ( !strncmp("m ",buf,2) || !strncmp("w ",buf,2) || !strncmp("memory ",buf,7) )
    {
      int width = 1;
      int start;
      int end;
      char *s;
      char *buf2;
      
      if( !strncmp("w ",buf,2))
      {
        width = 4;
      }
      
      buf2 = strchr(buf,' ')+1;

      if ( (s = strchr(buf2,'-')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf2,NULL,0);
        end = strtoul(s+1,NULL,0)+1;
      } else if ( (s = strchr(buf2,' ')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf2,NULL,0);
        end = start+strtoul(s+1,NULL,0);
      } else {
        start = strtoul(buf2,NULL,0);
        end = start + 0x100;
      }
     
      if ( (buf2 = ptp_chdk_get_memory(start,end-start,&params,&params.deviceinfo)) == NULL )
      {
        printf("error getting memory\n");
      } else {
        if(width == 4)
        {
            hexdump4(buf2,end-start,start);
        }
        else
        {
            hexdump(buf2,end-start,start);
        }
        free(buf2);
      }
      
    } else if ( !strncmp("engio",buf,3) )
    {
        unsigned int addr = 0;
        char *buf2;
        unsigned int block_size = 0x200;
        unsigned char *data_buf;

        buf2 = strchr(buf,' ')+1;

        while((*buf2 != '\000') && (*buf2 == ' '))
        {
            buf2++;
        }
        addr = strtoul(buf2,NULL,0);
        printf("Reading engio buffers from 0x%08X\r\n", addr);
        
        if(addr > 0)
        {
            unsigned int reg = 0;
            unsigned int data = 0;
            
            while(reg != 0xFFFFFFFF)
            {
                if ( (data_buf = ptp_chdk_get_memory(addr,block_size,&params,&params.deviceinfo)) == NULL )
                {
                    printf("error getting memory\n");
                    break;
                } 
                else 
                {
                    unsigned int pos = 0;
                    do
                    {
                        reg =  data_buf[pos + 0] | (data_buf[pos + 1] << 8)  | (data_buf[pos + 2] << 16)  | (data_buf[pos + 3] << 24);
                        pos += 4;
                        data = data_buf[pos + 0] | (data_buf[pos + 1] << 8)  | (data_buf[pos + 2] << 16)  | (data_buf[pos + 3] << 24);
                        pos += 4;
                        if(reg != 0xFFFFFFFF)
                        {
                            printf("[0x%08X] <- [0x%08X]\r\n", reg, data);
                        }
                    } while (reg != 0xFFFFFFFF && pos <= block_size - 8);
                    free(data_buf);
                    
                    addr += block_size;
                }        
            }            
        }

      
    } else if ( !strncmp("dump ",buf,5) )
    {
        unsigned long long start;
        unsigned long long end;
        unsigned long long addr;
        unsigned long long len;
        unsigned long long block_size = 0x1000;
        unsigned char *s;
        unsigned char *buf2;
        unsigned char *data_buf;
        FILE *dumpfile;

        buf2 = strchr(buf,' ')+1;

        if ( (s = strchr(buf2,'-')) != NULL )
        {
            *s = '\0';
            start = strtoul(buf2,NULL,0);
            end = strtoul(s+1,NULL,0)+1;
        }
        else if ( (s = strchr(buf2,' ')) != NULL )
        {
            *s = '\0';
            start = strtoul(buf2,NULL,0);
            end = start+strtoul(s+1,NULL,0);
        }
        else {
            start = strtoul(buf2,NULL,0);
            end = start + 0x100;
        }
        
        addr = start;
        
        dumpfile = fopen("dump.bin", "wb");
        if(dumpfile != NULL)
        {
            while(addr < end)
            {
                /* check for maximum block size */
                if(addr + block_size > end)
                {
                    len = end - addr;
                }
                else
                {
                    len = block_size;
                }
                
                printf("\rReading: 0x%08X...", (unsigned int)addr);
                fflush(stdout);
                
                if ( (data_buf = ptp_chdk_get_memory(addr,block_size,&params,&params.deviceinfo)) == NULL )
                {
                    printf("error getting memory\n");
                    break;
                } 
                else 
                {
                    fwrite(data_buf, len, 1, dumpfile);
                    free(data_buf);
                }
                addr += len;
            }
            fclose(dumpfile);
            printf("\r\nDone\r\n");
        }
      
    }else if ( !strncmp("ptr",buf,3) )
    {
        unsigned int current_address = 0;
        int done = 0;
        char *buf2;
        unsigned char *data_buf;
        
        while(!done)
        {
            int addr = 0;
            
            if ( fgets(buf,CHDKBUFS-1,stdin) == NULL )
            {
                printf("\n");
                break;
            }
            
            if(strlen(buf) < 3)
            {
                break;
            }
            
            buf2 = buf;
      
            while((*buf2 != '\000') && (*buf2 == ' '))
            {
                buf2++;
            }
            addr = strtoul(buf2,NULL,0);
            printf("offset: 0x%08X -> ", addr);
            
            current_address += addr;
            printf("read: 0x%08X -> ", current_address);
            
            if(current_address > 0)
            {
                if ( (data_buf = ptp_chdk_get_memory(current_address,0x20,&params,&params.deviceinfo)) == NULL )
                {
                    printf("error getting memory\n");
                    break;
                } 
                else 
                {
                    current_address = data_buf[0] | (data_buf[1] << 8)  | (data_buf[2] << 16)  | (data_buf[3] << 24);
                    printf("next: 0x%08X\r\n", current_address);
                    free(data_buf);
                }            
            }
        }
    } 
    else if (!strncmp("gdb s ",buf,6))
    {
        char *cmd = &(buf[6]);
        
        if (ptp_chdk_gdb_download(cmd,&params,&params.deviceinfo) == 0)
        {
            printf("error sending command\n");
        } 
    }
    else if (!strncmp("gdbproxy",buf,8))
    {
        gdb_listen();
    }
    else if (!strncmp("gdb r",buf,5))
    {
        char *buf;
        
        buf = ptp_chdk_gdb_upload(&params,&params.deviceinfo);
        
        if(buf == NULL)
        {
            printf("error receiving response\n");
        } 
        else
        {
            printf("gdb> '%s'\n", buf);
        }
    }
    else if ( !strncmp("delta ",buf,6) || !strncmp("adtgdelta ",buf,6) || !strncmp("engiodelta ",buf,6))
    {
      int regdump = 0;
      int start;
      int end;
      int deltaPos;
      int deltaNum = 0;
      char *s;
      unsigned char *buf2;
      unsigned int *deltaCount;
      
      if( !strncmp("adtgdelta ",buf,6))
      {
          regdump = 1;
      }
      if( !strncmp("engiodelta ",buf,6))
      {
          regdump = 2;
      }
      buf2 = strchr(buf,' ')+1;

      if ( (s = strchr(buf2,'-')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf2,NULL,0);
        end = strtoul(s+1,NULL,0)+1;
      } 
      else if ( (s = strchr(buf2,' ')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf2,NULL,0);
        end = start+strtoul(s+1,NULL,0);
      } 
      else
      {
        start = strtoul(buf2,NULL,0);
        end = start+0x100;
      }
     
      /* get reference buffer */
      if ( (buf2 = ptp_chdk_get_memory(start,end-start,&params,&params.deviceinfo)) == NULL )
      {
        printf("error getting memory\n");
      } 
      else 
      {
        if(regdump == 1)
        {
            adtg_dump(buf2, end-start, start);
        }
        else if(regdump == 2)
        {
            engio_dump(buf2, end-start, start);
        }
        else
        {
            hexdump(buf2,end-start,start);
        }
      }
      
      
        deltaCount = malloc(sizeof(unsigned int) * (end-start));

        for(deltaPos = 0; deltaPos < (end-start); deltaPos++)
        {
            deltaCount[deltaPos] = 0;
        }
      
        while(!kbhit())
        {
            unsigned char *buf3;

            /* get reference buffer */
            if ( (buf3 = ptp_chdk_get_memory(start,end-start,&params,&params.deviceinfo)) == NULL )
            {
                printf("error getting memory\n");
            } 
            else 
            {
                int pos = 0;
                int deltas = 0;
                
                for(pos = 0; pos < (end-start); pos++)
                {
                    if(buf2[pos] != buf3[pos] && deltaCount[pos] < 10)
                    {                    
                        printf("Ignoring delta at 0x%08X: 0x%02X -> 0x%02X\r\n", start + pos, buf2[pos], buf3[pos]);
                        deltaCount[pos]++;
                        deltas++;
                    }
                }
                
                if(deltas)
                {
                    memcpy(buf2, buf3, end-start);
                }
                free(buf3);
            }        
            usleep(100000);
        }
        fgetc(stdin);

        
      
        while(!kbhit())
        {
            unsigned char *buf3;

            /* get reference buffer */
            if ( (buf3 = ptp_chdk_get_memory(start,end-start,&params,&params.deviceinfo)) == NULL )
            {
                printf("error getting memory\n");
            } 
            else 
            {
                int pos = 0;
                int deltas = 0;
                
                for(pos = 0; pos < (end-start); pos++)
                {
                    if(deltaCount[pos] >= 10)
                    {
                        buf3[pos] = 0;
                        buf2[pos] = 0;
                    }
                    
                    if(buf2[pos] != buf3[pos] )
                    {                    
                        printf("Delta #%i at 0x%08X: 0x%02X -> 0x%02X\r\n", deltaNum++, start + pos, buf2[pos], buf3[pos]);
                        deltas++;
                    }
                    
                }
                if(deltas)
                {
                    memcpy(buf2, buf3, end-start);
                    if(regdump == 1)
                    {
                        adtg_dump(buf2, end-start, start);
                    }
                    else if(regdump == 2)
                    {
                        engio_dump(buf2, end-start, start);
                    }
                    else
                    {
                        hexdump(buf2,end-start,start);
                    }
                }
                free(buf3);
            }       
            usleep(100000);     
        }
      
        free(buf2);
        fgetc(stdin);

    } 
    else if ( !strncmp("rate ",buf,5))
    {
        int regdump = 0;
        int start;
        int deltaPos;
        int deltaNum = 0;
        char *s;
        unsigned char *buf2;
        unsigned int *deltaCount;
        unsigned int oldVal = 0;
        unsigned int maxVal = 0;
        struct timeval oldTv;
        struct timeval nextTv;
        unsigned int loops = 0;
        double avgDelta = -1.0f;

        buf2 = strchr(buf,' ') + 1;
      
        start = strtoul(buf2,NULL,0);
        
        while(!kbhit())
        {
            unsigned char *buf3;

            /* get reference buffer */
            if ( (buf3 = ptp_chdk_get_memory(start,4,&params,&params.deviceinfo)) == NULL )
            {
                printf("error getting memory\n");
            } 
            else 
            {
                unsigned int nextVal = 0;
                
                nextVal = *((unsigned int*)buf3);
                if(maxVal < nextVal)
                {
                    maxVal = nextVal;
                }
                
                if(nextVal < oldVal)
                {
                    unsigned int delta = 0;
                    
                    gettimeofday(&nextTv, NULL);
                    delta = ((nextTv.tv_sec - oldTv.tv_sec) * 1000000) + (nextTv.tv_usec - oldTv.tv_usec);
                    
                    
                    if(loops++ > 0)
                    {
                        double rate = 0;
                        
                        if(avgDelta < 0)
                        {
                            avgDelta = delta;
                        }
                        else
                        {
                            avgDelta = avgDelta - avgDelta/loops + delta / loops;
                        }
                        
                        rate = 1000000.0f / (avgDelta / maxVal);
                        printf("Overflows: %i µsec (avg: %f), maxVal = 0x%08X, tickRate = %f Hz\n", delta, avgDelta, maxVal, rate);
                    }
                }
                
                oldVal = nextVal;
                oldTv = nextTv;
                
                free(buf3);
            }            
        }
        fgetc(stdin);
    }
    else if ( !strncmp("set ",buf,4) )
    {
      int addr;
      int val;
      char *s;

      if ( (s = strchr(buf+4,' ')) == NULL )
      {
        printf("invalid arguments\n");
      } else {
        *s = '\0';
        addr = strtoul(buf+4,NULL,0);
        val = strtoul(s+1,NULL,0);
      
        if ( !ptp_chdk_set_memory_long(addr,val,&params,&params.deviceinfo) )
        {
          printf("set failed!\n");
        }
      }

    } else if ( !strncmp("c ",buf,2) || !strncmp("call ",buf,5) )
    {
      int num_args,i,ret;
      char *buf2;
      int *args;
      
      buf2 = buf;
      num_args = 0;
      while ( (buf2 = strchr(buf2,' ')) != NULL )
      {
        num_args++;
        buf2++;
      }
      args = malloc(num_args*sizeof(int));
      buf2 = buf;
      i = 0;
      while ( (buf2 = strchr(buf2,' ')) != NULL )
      {
        buf2++;
        args[i] = strtoul(buf2,NULL,0);
        i++;
      }

      if ( !ptp_chdk_call(args,num_args,&ret,&params,&params.deviceinfo) )
      {
        printf("error making call\n");
      } else {
        printf("%08x %i\n",ret,ret);
      }
      free(args);
      
    } else if ( !strncmp("prop ",buf,5) )
    {
      int start;
      int end;
      char *s;
      int *vals;

      if ( (s = strchr(buf+5,'-')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf+5,NULL,0);
        end = strtoul(s+1,NULL,0)+1;
      } else if ( (s = strchr(buf+5,' ')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf+5,NULL,0);
        end = start+strtoul(s+1,NULL,0);
      } else {
        start = strtoul(buf+5,NULL,0);
        end = start+1;
      }
      
      if ( (vals = ptp_chdk_get_propcase(start,end-start,&params,&params.deviceinfo)) == NULL )
      {
        printf("error getting properties\n");
      } else {
        int i;
        for (i=start; i<end; i++)
        {
          printf("%3i: %i\n",i,vals[i-start]);
        }
        hexdump((char *) vals,(end-start)*4,start*4);
        free(vals);
      }
      
    } else if ( !strncmp("param ",buf,6) )
    {
      int start;
      int end;
      char *s;
      char *buf2;

      if ( (s = strchr(buf+6,'-')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf+6,NULL,0);
        end = strtoul(s+1,NULL,0)+1;
      } else if ( (s = strchr(buf+6,' ')) != NULL )
      {
        *s = '\0';
        start = strtoul(buf+6,NULL,0);
        end = start+strtoul(s+1,NULL,0);
      } else {
        start = strtoul(buf+6,NULL,0);
        end = start+1;
      }
      
      if ( (buf2 = ptp_chdk_get_paramdata(start,end-start,&params,&params.deviceinfo)) == NULL )
      {
        printf("error getting parameter data\n");
      } else {
        int i;
        char *p = buf2;
        for (i=start; i<end; i++)
        {
          int t = *((int *) p);
          p += 4;
          printf("%03i: ",i);
          print_safe(p,t);
          printf(" (len=%i",t);
          if ( t == 1 )
          {
            printf(",val=%u)",*p);
          } else if ( t == 2 )
          {
            printf(",val=%u)",*((short *) p));
          } else if ( t == 4 )
          {
            printf(",val=%u)",*((int *) p));
          }
          printf(")\n");
          p += t;
        }
        hexdump(buf2,p-buf2,0);
        free(buf2);
      }

    } else if ( !strncmp("upload ",buf,7) )
    {
      char *s;

      if ( (s = strchr(buf,'\r')) != NULL )
      {
        *s = '\0';
      }
      if ( (s = strchr(buf,'\n')) != NULL )
      {
        *s = '\0';
      }

      if ( (s = strchr(buf+7,' ')) == NULL )
      {
        printf("invalid arguments\n");
      } else {
        *s = '\0';
        s++;
      
        if ( !ptp_chdk_upload(buf+7,s,&params,&params.deviceinfo) )
        {
          printf("upload failed!\n");
        }
      }

    } else if ( !strncmp("download ",buf,9) )
    {
      char *s;

      if ( (s = strchr(buf,'\r')) != NULL )
      {
        *s = '\0';
      }
      if ( (s = strchr(buf,'\n')) != NULL )
      {
        *s = '\0';
      }

      if ( (s = strchr(buf+9,' ')) == NULL )
      {
        printf("invalid arguments\n");
      } else {
        *s = '\0';
        s++;
      
        if ( !ptp_chdk_download(buf+9,s,&params,&params.deviceinfo) )
        {
          printf("download failed!\n");
        }
      }

    } else if ( !strncmp("mode ",buf,5) )
    {
      if ( !ptp_chdk_switch_mode(strtoul(buf+5,NULL,0),&params,&params.deviceinfo) )
      {
        printf("mode switch failed!\n");
      }

    } else if ( !strncmp("lua ",buf,4) )
    {
      if ( !ptp_chdk_exec_lua(buf+4,0,&params,&params.deviceinfo) )
      {
        printf("execution failed!\n");
      }

    } else if ( !strncmp("luar ",buf,5) )
    {
      if ( !ptp_chdk_exec_lua(buf+5,2,&params,&params.deviceinfo) )
      {
        printf("execution failed!\n");
      }
    } else if ( !strcmp("script-support",buf) )
    {
	  int status;
      if ( ptp_chdk_get_script_support(&params,&params.deviceinfo,&status) )
      {
        printf("script-support:0x%x lua=%s\n",status,(status & PTP_CHDK_SCRIPT_SUPPORT_LUA) ? "yes":"no");
      }
    } else if ( !strcmp("script-status",buf) )
    {
      int status;
      if ( ptp_chdk_get_script_status(&params,&params.deviceinfo,&status) )
      {
        printf("script-status:0x%x run=%s msg=%s\n",status,
                (status & PTP_CHDK_SCRIPT_STATUS_RUN) ? "yes":"no",
                (status & PTP_CHDK_SCRIPT_STATUS_MSG) ? "yes":"no");
      }
    } else if ( !strcmp("getm",buf) )
    {
      ptp_chdk_print_all_script_messages(&params,&params.deviceinfo);
    } else if ( !strncmp("putm ",buf,5) )
    {
      int status;
      if ( !ptp_chdk_write_script_msg(&params,&params.deviceinfo,buf+5,strlen(buf+5),&status) )
      {
        printf("putm failed!\n");
      } else {
        switch(status) {
          case PTP_CHDK_S_MSGSTATUS_OK:
          break;
          case PTP_CHDK_S_MSGSTATUS_NOTRUN:
            printf("no script running\n");
          break;
          case PTP_CHDK_S_MSGSTATUS_QFULL:
            printf("message queue full\n");
          break;
          case PTP_CHDK_S_MSGSTATUS_BADID:
            printf("running script id mismatch\n");
          break;
          default:
            printf("unexpected status code %d\n",status);
        }
      }
    } else {
      printf("unknown command\n");
    }
    // in command line mode parse command once and then exit:
    if (chdkmode==CHDK_MODE_CLI)
      break;
  }

  if ( connected )
  {
    close_connection();
  }

  return 0;
}
