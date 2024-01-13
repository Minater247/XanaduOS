#ifndef _DEVICES_H
#define _DEVICES_H

#include <stdint.h>

typedef struct {
    uint32_t flags;
    uint32_t pos;
    //we only support reading from /dev, so we don't need much for device_dir_t
} __attribute__((packed)) device_dir_t;

typedef struct device {
    char name[64];
    uint32_t flags;
    int (*read)(void *ptr, uint32_t size);
    int (*write)(void *ptr, uint32_t size);
    int (*seek)(uint32_t offset, uint8_t whence);
    int (*tell)();
    //other functions generally return 0/NULL on success and -1/NULL on failure since devices are not files
    struct device *next;
} device_t;

typedef struct {
    uint32_t flags;
    device_t *device;
} __attribute__((packed)) device_file_t;

void devices_initialize();
int register_device(device_t *device);

#endif