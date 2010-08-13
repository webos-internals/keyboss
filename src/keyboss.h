#ifndef KEYBOSS_H
#define KEYBOSS_H

#include <stdbool.h>
#include <linux/input.h>

#define DBUS_ADDRESS "org.webosinternals.keyboss"
#define KEYPAD_DEVICE "/dev/input/event2"
#define UINPUT_DEVICE "/dev/input/uinput"

/* FIXME: Get these macros from Palm patched input.h */

/* these keys are defined for Joplin */
#define KEY_CENTER		232
#define KEY_LAUNCHER		245
#define KEY_ALT			246
#define KEY_PTT         	247
#define KEY_PUSH_TO_TALK   	247
#define KEY_SLIDER		248

/* Default values for maxim7359 keypad can be seen in 
 * linux-2.6.24/arch/arm/mach-omap3pe/board-sirloin-3430.c
 */
#define DEFAULT_DELAY  (500)
#define DEFAULT_PERIOD (100)

typedef enum { 
  ACTION_NONE, 
  ACTION_DEFAULT, 
  ACTION_FUNCTION, 
  ACTION_CAPITALIZE
} ACTIONS;

extern int current_delay;
extern int current_period;
extern int hold_enabled;
extern int double_enabled;
extern int u_fd;
extern int k_fd;

/* function declarations */
bool is_valid_code(int code);
int send_key(__u16 code, __s32 value);
int set_repeat(__s32 delay, __s32 period);
int set_tap_timeout_ms(int ms);

#endif /* KEYBOSS_H */
