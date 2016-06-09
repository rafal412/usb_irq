/*
 * generic_hid.c
 *
 *  Created on: Apr 22, 2011
 *      Author: Jan Axelson
 *
 * Demonstrates communicating with a device designed for use with a generic HID-class USB device.
 * Sends and receives 2-byte reports.
 * Requires: an attached HID-class device that supports 2-byte
 * Input, Output, and Feature reports.
 * The device firmware should respond to a received report by sending a report.
 * Change VENDOR_ID and PRODUCT_ID to match your device's Vendor ID and Product ID.
 * See Lvr.com/winusb.htm for example device firmware.
 * This firmware is adapted from code provided by Xiaofan.
 * Note: libusb error codes are negative numbers.
 
The application uses the libusb 1.0 API from libusb.org.
Compile the application with the -lusb-1.0 option. 
Use the -I option if needed to specify the path to the libusb.h header file. For example:
-I/usr/local/angstrom/arm/arm-angstrom-linux-gnueabi/usr/include/libusb-1.0 

 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

// Values for bmRequestType in the Setup transaction's Data packet.

static const int CONTROL_REQUEST_TYPE_IN = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;
static const int CONTROL_REQUEST_TYPE_OUT = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;

// From the HID spec:

static const int HID_GET_REPORT = 0x01;
static const int HID_SET_REPORT = 0x09;
static const int HID_REPORT_TYPE_INPUT = 0x01;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

// Assumes interrupt endpoint 2 IN and OUT:

static const int INTERRUPT_IN_ENDPOINT = 0x81;
static const int INTERRUPT_OUT_ENDPOINT = 0x01;

// With firmware support, transfers can be > the endpoint's max packet size.

static const int MAX_INTERRUPT_IN_TRANSFER_SIZE = 64;
static const int MAX_INTERRUPT_OUT_TRANSFER_SIZE = 64;

static const int INTERFACE_NUMBER = 0;
static const int TIMEOUT_MS = 5000;

int exchange_input_and_output_reports_via_interrupt_transfers(libusb_device_handle *devh);
int only_send_data_via_interrupt_transfers(libusb_device_handle *devh);
int main(void)
{
	// Change these as needed to match idVendor and idProduct in your device's device descriptor.

	static const int VENDOR_ID = 0x04d8;
	static const int PRODUCT_ID = 0x003f;

	struct libusb_device_handle *devh = NULL;
	int device_ready = 0;
	int result;

	result = libusb_init(NULL);
	if (result >= 0)
	{
		devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);

		if (devh != NULL)
		{
			// The HID has been detected.
			// Detach the hidusb driver from the HID to enable using libusb.

			libusb_detach_kernel_driver(devh, INTERFACE_NUMBER);
			{
				result = libusb_claim_interface(devh, INTERFACE_NUMBER);
				if (result >= 0)
				{
					device_ready = 1;
				}
				else
				{
					fprintf(stderr, "libusb_claim_interface error %d\n", result);
				}
			}
		}
		else
		{
			fprintf(stderr, "Unable to find the device.\n");
		}
	}
	else
	{
		fprintf(stderr, "Unable to initialize libusb.\n");
	}

	if (device_ready)
	{
		// Send and receive data.

		exchange_input_and_output_reports_via_interrupt_transfers(devh);
		only_send_data_via_interrupt_transfers(devh);
		// Finished using the device.

		libusb_release_interface(devh, 0);
	}
	libusb_close(devh);
	libusb_exit(NULL);
	return 0;
}
int only_send_data_via_interrupt_transfers(libusb_device_handle *devh)
{
	int bytes_transferred;
	int i = 0;
	int result = 0;

	unsigned char data_out[MAX_INTERRUPT_OUT_TRANSFER_SIZE];

	for (i=0;i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
	{
		data_out[i]=0x80+i;
	}
	// Write data to the device.

	result = libusb_interrupt_transfer(
			devh,
			INTERRUPT_OUT_ENDPOINT,
			data_out,
			MAX_INTERRUPT_OUT_TRANSFER_SIZE,
			&bytes_transferred,
			TIMEOUT_MS);

	if (result >= 0)
	{
	  	printf("Data sent via interrupt transfer:\n");
	  	for(i = 0; i < bytes_transferred; i++)
	  	{
	  		printf("%02x ",data_out[i]);
	  	}
	  	printf("\n");
	}else
	{
		fprintf(stderr, "Error sending data via interrupt transfer %d\n", result);
		return result;
	}
	return 0;


}
// Use interrupt transfers to to write data to the device and receive data from the device.
// Returns - zero on success, libusb error code on failure.

int exchange_input_and_output_reports_via_interrupt_transfers(libusb_device_handle *devh)
{

	int bytes_transferred;
	int i = 0;
	int result = 0;

	unsigned char data_in[MAX_INTERRUPT_IN_TRANSFER_SIZE];
	unsigned char data_out[MAX_INTERRUPT_OUT_TRANSFER_SIZE];

	// Store data in a buffer for sending.

	for (i=0;i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
	{
		data_out[i]=0x79+i;
	}
	// Write data to the device.

	result = libusb_interrupt_transfer(
			devh,
			INTERRUPT_OUT_ENDPOINT,
			data_out,
			MAX_INTERRUPT_OUT_TRANSFER_SIZE,
			&bytes_transferred,
			TIMEOUT_MS);

	if (result >= 0)
	{
	  	printf("Data sent via interrupt transfer:\n");
	  	for(i = 0; i < bytes_transferred; i++)
	  	{
	  		printf("%02x ",data_out[i]);
	  	}
	  	printf("\n");

		// Read data from the device.

		result = libusb_interrupt_transfer(
				devh,
				INTERRUPT_IN_ENDPOINT,
				data_in,
				MAX_INTERRUPT_OUT_TRANSFER_SIZE,
				&bytes_transferred,
				TIMEOUT_MS);

		if (result >= 0)
		{
			if (bytes_transferred > 0)
			{
			  	printf("Data received via interrupt transfer:\n");
			  	for(i = 0; i < bytes_transferred; i++)
			  	{
			  		printf("%02x ",data_in[i]);
			  	}
			  	printf("\n");
			}
			else
			{
				fprintf(stderr, "No data received in interrupt transfer (%d)\n", result);
				return -1;
			}
		}
		else
		{
			fprintf(stderr, "Error receiving data via interrupt transfer %d\n", result);
			return result;
		}
	}
	else
	{
		fprintf(stderr, "Error sending data via interrupt transfer %d\n", result);
		return result;
	}
  	return 0;
 }
