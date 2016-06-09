#include <stdio.h>

#include "libusb-1.0/libusb.h"

static void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	libusb_device_handle *devHandle = NULL;
	int i = 0, j = 0;
	uint8_t path[8];
	int retVal = 0;
	unsigned char              strDesc[256];

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}
		retVal = libusb_open(dev, &devHandle);
		if (retVal != LIBUSB_SUCCESS){
			printf("break %d", retVal);
			goto next;
		} 

		printf("%04x:%04x (bus %d, device %d, speed %d)",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev), libusb_get_device_speed(dev));
		printf("iManufactore: %d", desc.iManufacturer);
		if (desc.iManufacturer > 0) {
			retVal = libusb_get_string_descriptor_ascii(devHandle,desc.iManufacturer, strDesc, 256);
			if (retVal < 0)
				break;
		         printf ("string = %s\n", strDesc);
		}
		if (desc.iProduct > 0) {
			retVal = libusb_get_string_descriptor_ascii(devHandle,desc.iProduct, strDesc, 256);
			if (retVal < 0)
				break;
			printf ("string = %s\n", strDesc);
		}
		r = libusb_get_port_numbers(dev, path, sizeof(path));
		if (r > 0) {
			printf(" path: %d", path[0]);
			for (j = 1; j < r; j++)
				printf(".%d", path[j]);
		}
next:
		printf("\n");
		libusb_close (devHandle);
		devHandle = NULL;
	}
}

int main(void)
{
	libusb_device **devs;
	int r;
	ssize_t cnt;

	r = libusb_init(NULL);
	if (r < 0)
		return r;
	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		return (int) cnt;

	print_devs(devs);
	libusb_free_device_list(devs, 1);

	libusb_exit(NULL);
	return 0;
}
