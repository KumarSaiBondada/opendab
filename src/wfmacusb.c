#include "opendab.h"

#define DEBUG 1

#ifdef __APPLE__

struct wavefinder *wf_open(char *devname)
{
        struct wavefinder *wf;

        CFMutableDictionaryRef matchingDictionary = NULL;
        SInt32 idVendor = 0x09CD; // set vendor id
        SInt32 idProduct = 0x2001; // set product id
        io_iterator_t iterator = 0;
        io_service_t usbRef;
        SInt32 score;
        IOCFPlugInInterface** plugin;
        IOUSBDeviceInterface300** usbDevice = NULL;
        IOReturn ret;
        IOUSBConfigurationDescriptorPtr config;
        IOUSBFindInterfaceRequest interfaceRequest;
        IOUSBInterfaceInterface300** usbInterface;

        matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName);
        CFDictionaryAddValue(matchingDictionary,
                             CFSTR(kUSBVendorID),
                             CFNumberCreate(kCFAllocatorDefault,
                                            kCFNumberSInt32Type, &idVendor));
        CFDictionaryAddValue(matchingDictionary,
                             CFSTR(kUSBProductID),
                             CFNumberCreate(kCFAllocatorDefault,
                                            kCFNumberSInt32Type, &idProduct));
        IOServiceGetMatchingServices(kIOMasterPortDefault,
                                     matchingDictionary, &iterator);
        usbRef = IOIteratorNext(iterator);
        IOObjectRelease(iterator);
        if (usbRef == 0) {
                fprintf(stderr, "Device not found\n");
                return NULL;
        }

        IOCreatePlugInInterfaceForService(usbRef, kIOUSBDeviceUserClientTypeID,
                                          kIOCFPlugInInterfaceID, &plugin, &score);
        IOObjectRelease(usbRef);
        (*plugin)->QueryInterface(plugin,
                                  CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID300),
                                  (LPVOID)&usbDevice);
        (*plugin)->Release(plugin);

        ret = (*usbDevice)->USBDeviceOpen(usbDevice);
        if (ret == kIOReturnSuccess) {
                // set first configuration as active
                ret = (*usbDevice)->GetConfigurationDescriptorPtr(usbDevice, 0, &config);
                if (ret != kIOReturnSuccess) {
                        fprintf(stderr, "Could not set active configuration (error: %x)\n", ret);
                        return NULL;
                }
                (*usbDevice)->SetConfiguration(usbDevice, config->bConfigurationValue);
        }
        else if (ret == kIOReturnExclusiveAccess) {
                // this is not a problem as we can still do some things
        }
        else {
                fprintf(stderr, "Could not open device (error: %x)\n", ret);
                return NULL;
        }

        interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
        interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
        interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
        interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;
        (*usbDevice)->CreateInterfaceIterator(usbDevice,
                                              &interfaceRequest, &iterator);

        usbRef = IOIteratorNext(iterator);
        IOObjectRelease(iterator);
        IOCreatePlugInInterfaceForService(usbRef,
                                          kIOUSBInterfaceUserClientTypeID,
                                          kIOCFPlugInInterfaceID, &plugin, &score);
        IOObjectRelease(usbRef);
        (*plugin)->QueryInterface(plugin,
                                  CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),
                                  (LPVOID)&usbInterface);
        (*plugin)->Release(plugin);

        ret = (*usbInterface)->USBInterfaceOpen(usbInterface);
        if (ret != kIOReturnSuccess)
        {
                fprintf(stderr, "Could not open interface (error: %x)\n", ret);
                return NULL;
        }

        if ((wf = malloc(sizeof (struct wavefinder))) == NULL)
                return NULL;

        wf->usbInterface = usbInterface;
        wf->usbDevice = usbDevice;

        return wf;
}

int wf_close(struct wavefinder *wf)
{
        (*wf->usbInterface)->USBInterfaceClose(wf->usbInterface);
        (*wf->usbDevice)->USBDeviceClose(wf->usbDevice);
        free(wf);
        return 0;
}

int wf_read(struct wavefinder *wf, unsigned char *rdbuf, unsigned int* len)
{
        IOReturn kr;

        kr = (*wf->usbInterface)->ReadPipe(wf->usbInterface, 0x81, rdbuf, len); // hardcoded 

        if (kr != kIOReturnSuccess) {
                fprintf(stderr, "Unable to perform bulk read (%08x)\n", kr);
                exit(EXIT_FAILURE);
        }

        return 0;
}

int wf_usb_ctrl_msg(struct wavefinder *wf, int request,
                    int value, int index, unsigned char *bytes, int size)
{
        IOUSBDevRequest req;

        req.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
        req.bRequest = request;
        req.wValue = value;
        req.wIndex = index;
        req.wLength = size;
        req.pData = bytes;

        if ((*wf->usbDevice)->DeviceRequest(wf->usbDevice, &req) < 0) {
		perror("wf_usb_ctrl_msg");
                exit(EXIT_FAILURE);
        }

        return 0;
}

#endif
