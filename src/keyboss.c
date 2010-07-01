#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include "luna_service.h"
#include "keyboss.h"

typedef enum {
  INACTIVE = 0,
  HOLDING,
  WAITING
} TIMERSTATE;

static int u_fd = -1;
static int k_fd = -1;
static pthread_t pipe_id;
int current_delay;
int current_period;
int hold_enabled = 0;
int double_enabled = 0;
int hold_key = 0;
int holding = INACTIVE;

static void restart_hidd(void) {
  system("/sbin/stop hidd");
  system("/sbin/start hidd");
}

static int send_event(int fd, __u16 type, __u16 code, __s32 value) {
  struct input_event event;

  memset(&event, 0, sizeof(event));
  event.type = type;
  event.code = code;
  event.value = value;

#if 0
  syslog(LOG_INFO, "send event type: %d, code %d, value %d\n",
      event.type, event.code, event.value);
#endif
  if (write(fd, &event, sizeof(event)) != sizeof(event)) {
    syslog(LOG_INFO, "write to fd %d failed\n", fd);
    return -1;
  }

  return 0;
}

void handle_double() {
  send_key(KEY_BACKSPACE, 1);
  send_key(KEY_BACKSPACE, 0);
  send_key(hold_key, 0);
  send_key(KEY_RIGHTALT, 1);
  send_key(KEY_RIGHTALT, 2);
  send_key(hold_key, 1);
  send_key(hold_key, 0);
  send_key(KEY_RIGHTALT, 0);
  holding = INACTIVE;
}

static int modifiable(__u16 code) {
  switch (code) {
    case KEY_Q:
    case KEY_W:
    case KEY_E:
    case KEY_R:
    case KEY_T:
    case KEY_Y:
    case KEY_U:
    case KEY_I:
    case KEY_O:
    case KEY_P:
    case KEY_A:
    case KEY_S:
    case KEY_D:
    case KEY_F:
    case KEY_G:
    case KEY_H:
    case KEY_J:
    case KEY_K:
    case KEY_L:
    case KEY_Z:
    case KEY_X:
    case KEY_C:
    case KEY_V:
    case KEY_B:
    case KEY_N:
    case KEY_M:
    case KEY_COMMA:
    case KEY_0:
      return 1;
      break;
    default:
      return 0;
      break;
  }
}

static int process_event(struct input_event *event) {
  if ((event->type != EV_KEY) || !modifiable(event->code) || (!hold_enabled && !double_enabled))
    goto send;

#if 0
  syslog(LOG_INFO, "code %d, value %d, hold %d, double %d, holding %d\n", event->code, event->value, hold_enabled, double_enabled, holding);
#endif
  if (event->value == 1) {
    if (double_enabled && holding && (event->code == hold_key)) {
      handle_double();
      return 0;
    }
    else {
      hold_key = event->code;
      ualarm(500 * 1000, 0);
      holding = HOLDING;
    }
  }
  else if (event->value == 0) {
    if (event->code == hold_key && holding) {
      if (double_enabled) {
        holding = WAITING;
        return 0;
      }
      else {
        holding = INACTIVE;
        ualarm(0, 0);
      }
    }
  }
  else {
    if (hold_enabled || holding)
      return 0;
  }

send:
  send_event(u_fd, event->type, event->code, event->value);
  return 0;
}

static void check_key_hold(int sig) {
  if (hold_enabled && holding == HOLDING) {
    send_key(KEY_BACKSPACE, 1);
    send_key(KEY_BACKSPACE, 0);
    send_key(hold_key, 0);
    send_key(KEY_LEFTSHIFT, 1);
    send_key(KEY_LEFTSHIFT, 2);
    send_key(hold_key, 1);
    send_key(hold_key, 0);
    send_key(KEY_LEFTSHIFT, 0);
  }
  holding = INACTIVE;
}

static void *pipe_keys(void *ptr) {
  int i; 
  struct uinput_user_dev device; 
  struct input_event event;

  memset(&device, 0, sizeof device);

  /* FIXME: use sysfs to find the keypad device by name */ 
  k_fd=open(KEYPAD_DEVICE, O_RDONLY); 
  if (k_fd < 0) 
    goto err;

  u_fd=open(UINPUT_DEVICE ,O_WRONLY); 
  if (u_fd < 0)
    goto err;

  restart_hidd(); 
  strcpy(device.name, "WebOS Internals KeyBoss");

  /* FIXME: any of this matter? */ 
  device.id.bustype=BUS_USB; 
  device.id.vendor=1; 
  device.id.product=1; 
  device.id.version=1;

  for (i=0; i < ABS_MAX; i++) {
    device.absmax[i] = -1;
    device.absmin[i] = -1;
    device.absfuzz[i] = -1;
    device.absflat[i] = -1;
  }

  device.absmin[ABS_X]=0;
  device.absmax[ABS_X]=255;
  device.absfuzz[ABS_X]=0;
  device.absflat[ABS_X]=0;

  if (write(u_fd, &device, sizeof(device)) != sizeof(device))
    goto err;

  if (ioctl(u_fd,UI_SET_EVBIT,EV_KEY) < 0)
    goto err;

  for (i=KEY_ESC; i<KEY_UNKNOWN; i++) {
    if (ioctl(u_fd, UI_SET_KEYBIT, i) < 0)
      goto err;
  }

#if 0
    if (ioctl(u_fd,UI_SET_EVBIT,EV_REL) < 0)
            fprintf(stderr, "error evbit rel\n");

    for(i=REL_X;i<REL_MAX;i++)
        if (ioctl(u_fd,UI_SET_RELBIT,i) < 0)
            fprintf(stderr, "error setrelbit %d\n", i);
#endif

  if (ioctl(u_fd,UI_DEV_CREATE) < 0)
    goto err;

  while (read(k_fd, &event, sizeof (struct input_event)) > 0) {
#if 0
    syslog(LOG_INFO, "event type: %d, code: %d, value: %d\n", 
        event.type, event.code, event.value);
#endif
    process_event(&event);
    //send_event(u_fd, event.type, event.code, event.value);
  }

err:
  if (u_fd > 0)
    close(u_fd);
  if (k_fd > 0)
    close(k_fd);

  return NULL;

}

bool is_valid_code(int code) {
  return (code >= KEY_ESC && code <KEY_UNKNOWN);
}

int set_repeat(__s32 delay, __s32 period) {
  int fd;

  /* FIXME: use sysfs to find the keypad device by name ? */ 
  fd=open(KEYPAD_DEVICE, O_WRONLY | O_SYNC); 
  if (fd < 0) {
    syslog(LOG_INFO, "Unable to open device %s in write mode\n", KEYPAD_DEVICE);
    return -1;
  }

  if (delay >= 0) {
    if (!send_event(fd, EV_REP, REP_DELAY, delay))
      current_delay = delay;
  }

  if (period >= 0) {
    if (!send_event(fd, EV_REP, REP_PERIOD, period))
      current_period = period;
  }

  return 0;
}

int send_key(__u16 code, __s32 value) {
  __s32 rel_val = 0;
  syslog(LOG_INFO, "send key %d, %d\n", code, value);
  send_event(u_fd, EV_KEY, code, value);
  if (value == 2)
    rel_val = 1;
  send_event(u_fd, EV_SYN, SYN_REPORT, rel_val);
}

void cleanup(int sig) {
  syslog(LOG_INFO, "cleanup (sig %d)", sig);
  pthread_cancel(pipe_id);
  set_repeat(DEFAULT_DELAY, DEFAULT_PERIOD);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGKILL, cleanup);

  signal(SIGALRM, check_key_hold);

  pthread_create(&pipe_id, NULL, pipe_keys, NULL);

  if (luna_service_initialize(DBUS_ADDRESS))
    luna_service_start();

	return 0;
}
