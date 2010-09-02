#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include "luna_service.h"
#include "keyboss.h"

char keypad_device[80] = "";
static pthread_t pipe_id = 0;
int u_fd = -1;
int k_fd = -1;
int current_delay = DEFAULT_DELAY;
int current_period = DEFAULT_PERIOD;
KEYSTATE keystate;
struct action_timer tap_timer;
struct action_timer hold_timer;
struct input_event key_queue[KEYQ_SIZE];

static int send_event(int fd, __u16 type, __u16 code, __s32 value) {
  struct input_event event;

  memset(&event, 0, sizeof(event));
  event.type = type;
  event.code = code;
  event.value = value;

  syslog(LOG_DEBUG, "send event type: %d, code %d, value %d\n", 
      event.type, event.code, event.value);
  if (write(fd, &event, sizeof(event)) != sizeof(event)) {
    syslog(LOG_ERR, "write to fd %d failed\n", fd);
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

static int flush_queue(ACTIONS action, __u16 code) {
  int i;
  
  switch (action) {
    case ACTION_FUNCTION:
      action_function(code);
      break;
    case ACTION_CAPITALIZE:
      action_capitalize(code);
      break;
    case ACTION_DEFAULT:
      send_key(code, KEYDOWN);
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

static int stop_tap() {
  struct itimerspec value;

  value.it_value.tv_sec = 0;
  value.it_value.tv_nsec = 0;

  timer_settime(tap_timer.timerid, 0, &value, NULL);
  flush_queue(ACTION_NONE, 0);
  keystate.tap.state = STATE_IDLE;
  return 0;
#if 0
  if (keystate.tap.state == STATE_WAITING) {
    keystate.tap.count = 0;
    keystate.tap.code = 0;
    keystate.tap.state = STATE_IDLE;
    timer_settime(tap_timer.timerid, 0, &value, NULL);

    if (keystate.hold.state == STATE_IDLE)
      flush_queue(ACTION_NONE, 0);
  }
#endif
}

static int start_tap_timeout() {
  keystate.tap.state = STATE_WAITING;
  timer_settime(tap_timer.timerid, 0, &tap_timer.value, NULL);
}

static int cancel_hold_timeout() {
  struct itimerspec value;
  value.it_value.tv_sec = 0;
  value.it_value.tv_nsec = 0;

  if (keystate.hold.state == STATE_WAITING) {
    keystate.hold.count = 0;
    timer_settime(hold_timer.timerid, 0, &value, NULL);
    keystate.hold.state = STATE_IDLE;
  }
}

static int start_hold_timeout() {
  keystate.hold.state = STATE_WAITING;
  timer_settime(hold_timer.timerid, 0, &hold_timer.value, NULL);
}

static int hold_action() {
  struct key_modifier *hold = &keystate.hold;

  if (hold->num_active) {
    if (hold->count == hold->num_active) {
      if (!hold->circular)
        return 0;
      hold->count -= hold->num_active;
    }
    send_key(KEY_BACKSPACE, KEYDOWN);
    send_key(KEY_BACKSPACE, KEYUP);
    send_key(keystate.hold.code, KEYUP);
    flush_queue(hold->actions[hold->count], hold->code);
    hold->count++;
    start_hold_timeout();
  }
  else {
    flush_queue(ACTION_NONE, 0);
  }

  return 0;
}

static int tap_action() {
  struct key_modifier *tap = &keystate.tap;

  if (tap->num_active) {
    if (!tap->circular && (tap->count == tap->num_active))
      return 0;
    send_key(KEY_BACKSPACE, KEYDOWN);
    send_key(KEY_BACKSPACE, KEYUP);
    send_key(keystate.tap.code, KEYUP);
    flush_queue(tap->actions[tap->count], tap->code);
    tap->count = (tap->count + 1) % tap->num_active;
    start_tap_timeout();
  }
  else {
    flush_queue(ACTION_NONE, 0);
  }

  return 0;
}

static int add_key_to_queue(__u16 code, __s32 value) {
  int i;

  if (key_queue[KEYQ_SIZE - 1].code)
    flush_queue(ACTION_NONE, 0);

  for (i=0; i<KEYQ_SIZE; i++) { 
    if (!key_queue[i].code) { 
      key_queue[i].code = code; 
      key_queue[i].value = value; 
      break; 
    } 
  }

  return i;
}

void hold_timeout(union sigval sig) {
  hold_action();
  stop_tap();
}

void tap_timeout(union sigval sig) {
  stop_tap();
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

  if (hold->num_active == 1)
    hold->circular = false;

  return 0;
}

int install_hold_action(ACTIONS action) {
  struct key_modifier *hold = &keystate.hold;

  if (hold->num_active >= MAX_ACTIONS)
    return -1;

  hold->actions[hold->num_active++] = action;
  
  if (hold->num_active > 1)
    hold->circular = true;

  return 0;
}

int change_tap_action(int index, ACTIONS action) {
  struct key_modifier *tap = &keystate.tap;

  if (index < 0 || index >= tap->num_active)
   return -1;

 tap->actions[index] = action; 
}

int remove_tap_action(ACTIONS action, int index) {
  int i, j, ix;

  struct key_modifier *tap = &keystate.tap;

  if (index >= 0)
    ix = index;
  else {
    for (i=0; i<tap->num_active; i++) {
      if (tap->actions[i] = action) {
        ix = i;
      }
    }
  }

  if (ix < 0 || ix >= tap->num_active)
    return -1;

  for (j=ix+1; j<tap->num_active; j++) { 
    tap->actions[j-1] = tap->actions[j]; 
  } 

  tap->actions[j-1] = ACTION_NONE; 
  tap->num_active--; 

  if (tap->num_active == 1)
    tap->circular = false;

  return 0;
}

int install_tap_action(ACTIONS action) {
  struct key_modifier *tap = &keystate.tap;

  if (tap->num_active >= MAX_ACTIONS)
    return -1;

  tap->actions[tap->num_active++] = action;

  if (tap->num_active > 1)
    tap->circular = true;

  return 0;
}

// set timeout in ms
int set_tap_timeout_ms(int ms) {
  keystate.tap.threshold = ms;

  tap_timer.value.it_value.tv_sec = ms / 1000;
  tap_timer.value.it_value.tv_nsec = (ms % 1000) * 1000000;
}

int set_hold_timeout_ms(int ms) {
  keystate.hold.threshold = ms;

  hold_timer.value.it_value.tv_sec = ms / 1000;
  hold_timer.value.it_value.tv_nsec = (ms % 1000) * 1000000;
}


static int initialize() {
  char keypad_link[20];
  int ret;

  memset(&tap_timer, 0, sizeof(struct action_timer));
  memset(&hold_timer, 0, sizeof(struct action_timer));
  memset(&keystate, 0, sizeof(KEYSTATE));

  // Read the keypad0 symlink to find out the actual keyboard device
  ret = readlink("/dev/input/keypad0", keypad_link, sizeof (keypad_link));
  if (ret < 0)
    syslog(LOG_ERR, "Unable to read symlink /dev/input/keypad0");
  sprintf(keypad_device, "/dev/input/%s", keypad_link);

  tap_timer.evp.sigev_notify = SIGEV_THREAD;
  tap_timer.evp.sigev_notify_function = tap_timeout;
  tap_timer.value.it_value.tv_sec = 0;
  tap_timer.value.it_value.tv_nsec = DEFAULT_TAP_TIMEOUT * 1000000;
  ret = timer_create(CLOCK_REALTIME, &tap_timer.evp, &tap_timer.timerid);

  hold_timer.evp.sigev_notify = SIGEV_THREAD;
  hold_timer.evp.sigev_notify_function = hold_timeout;
  hold_timer.value.it_value.tv_sec = 0;
  hold_timer.value.it_value.tv_nsec = DEFAULT_HOLD_TIMEOUT * 1000000;
  ret = timer_create(CLOCK_REALTIME, &hold_timer.evp, &hold_timer.timerid);

  keystate.tap.threshold = DEFAULT_TAP_TIMEOUT;
  keystate.hold.threshold = DEFAULT_HOLD_TIMEOUT;
}

static bool within_tap_threshold(struct input_event *event) {
  uint32_t elapsed;

  elapsed = (event->time.tv_sec - keystate.tap.time.tv_sec) * 1000;
  elapsed += (event->time.tv_usec - keystate.tap.time.tv_usec) / 1000;
  if (elapsed > keystate.tap.threshold)
    return false;

  return true;
}

static int process_event(struct input_event *event) {
  if (event->type != EV_KEY)
   return 0;

  if (event->value == KEYHOLD && keystate.hold.num_active && modifiable(event->code))
    return 0;

  add_key_to_queue(event->code, event->value);

  if (!modifiable(event->code)) {
    flush_queue(ACTION_NONE, 0);
    keystate.tap.code = event->code;
    keystate.hold.code = event->code;
    return 0;
  }

  switch (event->value) {
    case KEYDOWN:
      cancel_hold_timeout();
      if (keystate.tap.code == event->code && within_tap_threshold(event)) {
        if (keystate.tap.state == STATE_IDLE) {
          syslog(LOG_WARNING, "Missed tap action due to timeout");
        }
        else {
          tap_action();
          memcpy(&keystate.tap.time, &event->time, sizeof keystate.tap.time);
        }
      }  
      else {
        flush_queue(ACTION_NONE, 0);
        if (keystate.hold.num_active) {
          keystate.hold.code = event->code;
          start_hold_timeout();
        }
        if (keystate.tap.num_active) {
          keystate.tap.code = event->code;
          memcpy(&keystate.tap.time, &event->time, sizeof keystate.tap.time);
          start_tap_timeout();
        }
      }
      break;
    case KEYUP:
      cancel_hold_timeout();
      if (keystate.tap.code != event->code || keystate.tap.state == STATE_IDLE) {
        flush_queue(ACTION_NONE, 0);
        stop_tap();
      }
      break;
    case KEYHOLD:
      if (!keystate.hold.num_active) {
        stop_tap();
        flush_queue(ACTION_NONE, 0);
      }
      break;
    default:
      break;
  }

  return 0;
}

static void *pipe_keys(void *ptr) {
  int i; 
  int ret;
  struct uinput_user_dev device; 
  struct input_event event;

  if (!keypad_device[0]) {
    syslog(LOG_ERR, "Error: unknown keypad device");
    return NULL;
  }

  memset(&device, 0, sizeof device);

  k_fd=open(keypad_device, O_RDONLY); 
  if (k_fd < 0)  {
    syslog(LOG_ERR, "open keypad (%s) err: %s", keypad_device, strerror(errno));
    goto err;
  }

  system("/sbin/stop hidd");
  u_fd=open(UINPUT_DEVICE ,O_WRONLY); 
  system("/sbin/start hidd");
  if (u_fd < 0) {
    syslog(LOG_ERR, "open uinput err: %s", strerror(errno));
    goto err;
  }

  //restart_hidd(); 
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
    process_event(&event);
    //syslog(LOG_DEBUG, "Got Hardware event type: %d, code: %d, value: %d\n", 
        //event.type, event.code, event.value);
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
  return (code >= KEY_ESC && code <= KEY_SLIDER);
}

int set_repeat(__s32 delay, __s32 period) {
  int fd;

  if (keypad_device[0]) {
    fd=open(keypad_device, O_WRONLY | O_SYNC); 
    if (fd < 0) {
      syslog(LOG_ERR, "Unable to open device %s in write mode\n", keypad_device);
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
  }

  return 0;
}

int send_key(__u16 code, __s32 value) {
  __s32 rel_val = 0;

  send_event(u_fd, EV_KEY, code, value);
  if (value == 2)
    rel_val = 1;
  send_event(u_fd, EV_SYN, SYN_REPORT, rel_val);
}

void cleanup(int sig) {
  syslog(LOG_NOTICE, "cleanup (sig %d)", sig);
  system("/sbin/stop hidd");
  pthread_cancel(pipe_id);
  set_repeat(DEFAULT_DELAY, DEFAULT_PERIOD);
  system("/sbin/start hidd");
  exit(0);
}

int start_pipe_thread() {
  if (!pipe_id) {
    pthread_create(&pipe_id, NULL, pipe_keys, NULL);
  }
}

int stop_pipe_thread() {
  if (pipe_id)  {
    cancel_hold_timeout();
    stop_tap();
    system("/sbin/stop hidd");
    pthread_cancel(pipe_id);
    close(u_fd);
    set_repeat(DEFAULT_DELAY, DEFAULT_PERIOD);
    close(k_fd);
    pipe_id = 0;
    system("/sbin/start hidd");
  }
}

int main(int argc, char *argv[]) {
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGKILL, cleanup);

  initialize();
  start_pipe_thread();

  if (luna_service_initialize(DBUS_ADDRESS))
    luna_service_start();

	return 0;
}
