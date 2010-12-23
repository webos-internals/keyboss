#ifndef KEYBOSS_H
#define KEYBOSS_H

#include <stdbool.h>
#include <linux/input.h>

#define DBUS_ADDRESS "org.webosinternals.keyboss"
#define UINPUT_DEVICE "/dev/input/uinput"
#define PROX_TIMEOUT "/sys/class/i2c-adapter/i2c-3/3-0038/prox_timeout"
#define ARGS_FILE "/var/preferences/org.webosinternals.keyboss/keyboss-args"

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

#define DEFAULT_HOLD_TIMEOUT (450)
#define DEFAULT_TAP_TIMEOUT (140)

#define MAX_ACTIONS (8)

#define KEYUP   0
#define KEYDOWN 1
#define KEYHOLD 2
#define KEYQ_SIZE 8

typedef enum { 
  ACTION_NONE = -1, 
  ACTION_DEFAULT, 
  ACTION_FUNCTION, 
  ACTION_CAPITALIZE
} ACTIONS;

#define IS_VALID_ACTION(action) (action == ACTION_DEFAULT || action == ACTION_FUNCTION || action == ACTION_CAPITALIZE)


struct key_modifier {
  int count;
  int num_active;
  int actions[MAX_ACTIONS];
  bool circular;
  __u16 code;
  __u16 state;
  int threshold; // in ms
  struct timeval time;
};

struct action_timer {
  timer_t timerid;
  struct itimerspec value;
  struct sigevent evp;
};

typedef struct {
  struct key_modifier tap;
  struct key_modifier hold;
} KEYSTATE;

enum {
  STATE_IDLE,
  STATE_WAITING
};

extern int current_delay;
extern int current_period;
extern int hold_enabled;
extern int double_enabled;
extern int u_fd;
extern int k_fd;
extern KEYSTATE keystate;
extern struct action_timer tap_timer;
extern struct action_timer hold_timer;
extern pthread_t pipe_id;
extern void reset_to_defaults(void);

/* function declarations */
bool is_valid_code(int code);
int send_key(__u16 code, __s32 value);
int set_repeat(__s32 delay, __s32 period);
int set_tap_timeout_ms(int ms);

#endif /* KEYBOSS_H */
