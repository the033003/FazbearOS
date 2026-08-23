#include "device.h"

#include "heap.h"

static struct device* device_list;
static size_t registered_devices;

void device_subsystem_init(void)
{
    device_list = 0;
    registered_devices = 0;
}

static int device_name_equals(
    const char* a,
    const char* b
)
{
    for (size_t i = 0;
         i < DEVICE_NAME_LENGTH;
         i++) {

        if (a[i] != b[i]) {
            return 0;
        }

        if (a[i] == '\0') {
            return 1;
        }
    }

    return 1;
}

int device_register(
    struct device* device
)
{
    if (device == 0 ||
        device->name[0] == '\0') {

        return -1;
    }

    if (device_find(device->name) != 0) {
        return -2;
    }

    device->next = device_list;

    device_list = device;

    registered_devices++;

    return 0;
}

int device_unregister(
    const char* name
)
{
    struct device* current =
        device_list;

    struct device* previous =
        0;

    while (current != 0) {
        if (device_name_equals(
                current->name,
                name
            )) {

            if (previous == 0) {
                device_list =
                    current->next;
            } else {
                previous->next =
                    current->next;
            }

            registered_devices--;

            return 0;
        }

        previous = current;
        current = current->next;
    }

    return -1;
}

struct device* device_find(
    const char* name
)
{
    struct device* current =
        device_list;

    while (current != 0) {
        if (device_name_equals(
                current->name,
                name
            )) {

            return current;
        }

        current = current->next;
    }

    return 0;
}

struct device* device_first(void)
{
    return device_list;
}

size_t device_count(void)
{
    return registered_devices;
}
