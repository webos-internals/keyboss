#ifndef KEYBOSS_H
#define KEYBOSS_H

#define DBUS_ADDRESS "org.webosinternals.keyboss"
#define KEYPAD_DEVICE "/dev/input/event2"
#define UINPUT_DEVICE "/dev/input/uinput"
pthread_t pipe_id;
int u_fd = -1;
int k_fd = -1;

#endif /* KEYBOSS_H */
