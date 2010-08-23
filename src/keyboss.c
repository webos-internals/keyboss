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

static pthread_t pipe_id;
//static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int u_fd = -1;
int k_fd = -1;
int current_delay;
int current_period;
int hold_enabled = 0;
int double_enabled = 0;
int hold_key = 0;
int double_key = 0;
//int holding = INACTIVE;

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

  //syslog(LOG_INFO, "send event type: %d, code %d, value %d\n",
      //event.type, event.code, event.value);
  if (write(fd, &event, sizeof(event)) != sizeof(event)) {
    syslog(LOG_INFO, "write to fd %d failed\n", fd);
    return -1;
  }

  return 0;
}

static void action_capitalize(__u16 code) {
  send_key(KEY_LEFTSHIFT, 1);
  //send_key(KEY_LEFTSHIFT, 2);
  send_key(code, 1);
  //send_key(code, 0);
  send_key(KEY_LEFTSHIFT, 0);
}

static void action_function(__u16 code) {
  send_key(KEY_RIGHTALT, 1);
  //send_key(KEY_RIGHTALT, 2);
  send_key(code, 1);
  //send_key(code, 0);
  send_key(KEY_RIGHTALT, 0);
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

KEYSTATE keystate;
struct action_timer tap_timer;
struct input_event key_queue[KEYQ_SIZE];

static int stop_tap() {
  keystate.tap.count = 0;
}

static int flush_queue(ACTIONS action) {
  int i;
  
  switch (action) {
    case ACTION_FUNCTION:
      syslog(LOG_INFO, "action function %d", key_queue[0].code);
      action_function(key_queue[0].code);
      break;
    case ACTION_CAPITALIZE:
      syslog(LOG_INFO, "action cap");
      action_capitalize(key_queue[0].code);
      break;
    case ACTION_DEFAULT:
      send_key(key_queue[0].code, KEYDOWN);
      break;
    default:
      break;
  }

  // Now zero out the queue
  for (i=0; i<KEYQ_SIZE; i++) {
    if (key_queue[i].code) {
      if (action == ACTION_NONE) 
        send_key(key_queue[i].code, key_queue[i].value); 
      key_queue[i].code = 0; 
      key_queue[i].value = 0; 
    } 
  }
}

void tap_timeout(union sigval sig) {
  syslog(LOG_INFO, "tap timeout");
  switch (keystate.state) {
    case STATE_TAP:
      syslog(LOG_INFO, "send up");
      send_key(keystate.code, KEYUP);
      stop_tap();
      break;
    default:
      syslog(LOG_INFO, "flush");
      flush_queue(ACTION_DEFAULT);
      stop_tap();
      break;
  }

  keystate.state = STATE_IDLE;
}

int change_hold_action(int index, ACTIONS action) {
  struct key_modifier *hold = &keystate.hold;

  if (index < 0 || index >= hold->num_active)
   return -1;

 hold->actions[index] = action; 
}

int remove_hold_action(ACTIONS action, int index) {
  int i, j, ix;

  struct key_modifier *hold = &keystate.hold;

  if (index >= 0)
    ix = index;
  else {
    for (i=0; i<hold->num_active; i++) {
      if (hold->actions[i] = action) {
        ix = i;
      }
    }
  }

  if (ix < 0 || ix >= hold->num_active)
    return -1;

  for (j=ix+1; j<hold->num_active; j++) { 
    hold->actions[j-1] = hold->actions[j]; 
  } 

  hold->actions[j-1] = ACTION_NONE; 
  hold->num_active--; 

  return 0;
}

int install_hold_action(ACTIONS action) {
  struct key_modifier *hold = &keystate.hold;

  if (hold->num_active >= MAX_ACTIONS)
    return -1;

  syslog(LOG_INFO, "install hold action %d", action);
  hold->actions[hold->num_active++] = action;

  return 0;
}

int change_tap_action(int index, ACTIONS action) {
  struct key_modifier *tap = &keystate.tap;

  if (index < 0 || index >= tap->num_active)
   return -1;

  syslog(LOG_DEBUG, "change tap action %d to %d\n", index, action);
 tap->actions[index] = action; 
}

int remove_tap_action(ACTIONS action) {
  int i, j;

  struct key_modifier *tap = &keystate.tap;

  for (i=0; i<tap->num_active; i++) {
    if (tap->actions[i] == action) {
      for (j=i+1; j<tap->num_active; j++) {
        tap->actions[j-1] = tap->actions[j];
      }
      tap->actions[j-1] = ACTION_NONE;
      tap->num_active--;
      return 0;
    }
  }

  return -1;
}

int install_tap_action(ACTIONS action) {
  struct key_modifier *tap = &keystate.tap;

  if (tap->num_active >= MAX_ACTIONS)
    return -1;

  syslog(LOG_DEBUG, "installed tap action %d\n", action);
  tap->actions[tap->num_active++] = action;

  return 0;
}

static int hold_action() {
  struct key_modifier *hold = &keystate.hold;

  if (hold->num_active) {
    //keystate.state = HOLD_ACTION;
    send_key(KEY_BACKSPACE, KEYDOWN);
    send_key(KEY_BACKSPACE, KEYUP);
    send_key(keystate.code, KEYUP);
    flush_queue(hold->actions[hold->count]);
    hold->count = (hold->count + 1) % hold->num_active;
    syslog(LOG_INFO, "num %d, count %d\n", hold->count, hold->num_active);
  }
  else {
    flush_queue(ACTION_NONE);
  }
}

static int tap_action() {
  struct key_modifier *tap = &keystate.tap;

  if (tap->num_active) {
    keystate.state = STATE_TAP;
    send_key(KEY_BACKSPACE, KEYDOWN);
    send_key(KEY_BACKSPACE, KEYUP);
    send_key(keystate.code, KEYUP);
    flush_queue(tap->actions[tap->count]);
    tap->count = (tap->count + 1) % tap->num_active;
  }
  else {
    flush_queue(ACTION_NONE);
  }
}

static int add_key_to_queue(__u16 code, __s32 value) {
  int i;

  if (key_queue[KEYQ_SIZE - 1].code)
    flush_queue(ACTION_NONE);

  for (i=0; i<KEYQ_SIZE; i++) { 
    if (!key_queue[i].code) { 
      key_queue[i].code = code; 
      key_queue[i].value = value; 
      break; 
    } 
  }

  return i;
}

// set timeout in ms
int set_tap_timeout_ms(int ms) {
  syslog(LOG_DEBUG, "set tap timeout %d ms", ms);
  syslog(LOG_DEBUG, "tv_sec %d", ms / 1000);
  syslog(LOG_DEBUG, "tv_nsec %d", (ms % 1000) * 1000000);
  tap_timer.value.it_value.tv_sec = ms / 1000;
  tap_timer.value.it_value.tv_nsec = (ms % 1000) * 1000000;
}

static int initialize() {
  memset(&tap_timer, 0, sizeof(struct action_timer));
  memset(&keystate, 0, sizeof(KEYSTATE));

  tap_timer.evp.sigev_notify = SIGEV_THREAD;
  tap_timer.evp.sigev_notify_function = tap_timeout;
  tap_timer.value.it_value.tv_sec = 0;
  tap_timer.value.it_value.tv_nsec = DEFAULT_TIMEOUT * 1000000;
  timer_create(CLOCK_REALTIME, &tap_timer.evp, &tap_timer.timerid);
}

static int start_timeout() {
  syslog(LOG_DEBUG, "start timeout");
  timer_settime(&tap_timer.timerid, 0, &tap_timer.value, NULL);
}

static int process_event(struct input_event *event) {
  if (event->type != EV_KEY)
   return 0;

  add_key_to_queue(event->code, event->value);

  if (!modifiable(event->code)) {
    flush_queue(ACTION_NONE);
    keystate.code = event->code;
    return 0;
  }

  switch (event->value) {
    case KEYDOWN:
      syslog(LOG_DEBUG, "saved code %d, this code %d\n", keystate.code, event->code);
      if (keystate.code == event->code) {
        tap_action();
      }  
      else {
        flush_queue(ACTION_NONE);
        start_timeout();
      }
      break;
    case KEYUP:
      if (keystate.code != event->code) {
        flush_queue(ACTION_NONE);
        stop_tap();
      }
      break;
    case KEYHOLD:
      stop_tap();
      syslog(LOG_INFO, "hold action");
      hold_action();
      break;
    default:
      break;
  }

  keystate.code = event->code;
  return 0;
}

#if 0
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
#endif

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

  for (i=KEY_ESC; i<=KEY_SLIDER; i++) {
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
    //syslog(LOG_INFO, "event type: %d, code: %d, value: %d\n", 
        //event.type, event.code, event.value);
    process_event(&event);
    //send_event(u_fd, event.type, event.code, event.value);
  }

err:
  if (u_fd > 0)
    close(u_fd);
  if (k_fd > 0)
    close(k_fd);

  syslog(LOG_INFO, "err u_fd %d", u_fd);
  return NULL;

}

bool is_valid_code(int code) {
  return (code >= KEY_ESC && code <= KEY_SLIDER);
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
  exit(0);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGKILL, cleanup);

  //signal(SIGALRM, check_key_hold);

  pthread_create(&pipe_id, NULL, pipe_keys, NULL);

  initialize();

  if (luna_service_initialize(DBUS_ADDRESS))
    luna_service_start();

	return 0;
}
