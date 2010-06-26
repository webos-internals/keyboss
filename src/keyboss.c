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

static int u_fd = -1;
static int k_fd = -1;
static pthread_t pipe_id;

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

  if (write(fd, &event, sizeof(event)) != sizeof(event)) {
    syslog(LOG_INFO, "write to fd %d failed\n", fd);
    return -1;
  }

  return 0;
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
    syslog(LOG_INFO, "event type: %d, code: %d, value: %d\n", 
        event.type, event.code, event.value);
    send_event(u_fd, event.type, event.code, event.value);
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

  /* FIXME: use sysfs to find the keypad device by name */ 
  fd=open(KEYPAD_DEVICE, O_WRONLY); 
  if (fd < 0) {
    syslog(LOG_INFO, "Unable to open device %s in write mode\n", KEYPAD_DEVICE);
    return -1;
  }

  if (delay >= 0)
    send_event(fd, EV_REP, REP_DELAY, delay);

  if (period >= 0)
    send_event(fd, EV_REP, REP_PERIOD, period);

  return 0;
}

int send_key(__u16 code, __s32 value) {
  send_event(u_fd, EV_KEY, code, value);
  send_event(u_fd, EV_SYN, SYN_REPORT, 0);
}

void cleanup(int sig) {
  /* Restart hidd to let it re-initialize with the changed 
   * /etc/input/keypad0 symlink */
  syslog(LOG_INFO, "caught sig %d", sig);
  set_repeat(DEFAULT_DELAY, DEFAULT_PERIOD);
  restart_hidd();
  pthread_cancel(pipe_id);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGKILL, cleanup);

  pthread_create(&pipe_id, NULL, pipe_keys, NULL);

  if (luna_service_initialize(DBUS_ADDRESS))
    luna_service_start();

	return 0;
}
