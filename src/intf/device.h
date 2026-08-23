#pragma once

#include <stddef.h>
#include <stdint.h>

#define DEVICE_NAME_LENGTH 32

enum device_type {
    DEVICE_UNKNOWN = 0,
    DEVICE_CHAR,
    DEVICE_BLOCK,
    DEVICE_INPUT,
    DEVICE_DISPLAY,
    DEVICE_NETWORK,
    DEVICE_STORAGE
};

struct device;

typedef int (*device_read_function)(
    struct device* device,
    void* buffer,
    size_t size
);

typedef int (*device_write_function)(
    struct device* device,
    const void* buffer,
    size_t size
);

typedef int (*device_control_function)(
    struct device* device,
    uint64_t command,
    uint64_t argument
);

struct device_operations {
    device_read_function read;
    device_write_function write;
    device_control_function control;
};

struct device {
    char name[DEVICE_NAME_LENGTH];

    enum device_type type;

    uint32_t id;

    void* private_data;

    const struct device_operations* operations;

    struct device* next;
};

void device_subsystem_init(void);

int device_register(
    struct device* device
);

int device_unregister(
    const char* name
);

struct device* device_find(
    const char* name
);

struct device* device_first(void);

size_t device_count(void);
