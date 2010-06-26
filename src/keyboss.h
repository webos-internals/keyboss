#ifndef KEYBOSS_H
#define KEYBOSS_H

#include <stdbool.h>
#include <linux/input.h>

#define DBUS_ADDRESS "org.webosinternals.keyboss"
#define KEYPAD_DEVICE "/dev/input/event2"
#define UINPUT_DEVICE "/dev/input/uinput"

/* function declarations */
bool is_valid_code(int code);
int send_key(__u16 code, __s32 value);

#endif /* KEYBOSS_H */
