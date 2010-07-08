#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <syslog.h>
#include "luna_service.h"
#include "keyboss.h"

char message_buf[1024];

bool emulate_key(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  int code;
  bool keydown;
  json_t *object;
 
  object = LSMessageGetPayloadJSON(message);
  json_get_int(object, "code", &code);
  json_get_bool(object, "keydown", &keydown);
  if (!is_valid_code(code)) {
    LSMessageRespond(message, 
        "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Invalid key code\"}", &lserror);
    return true;
  }

  send_key(code, keydown);
  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);

  return true;
}

bool get_repeat_rate(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  memset(message_buf, 0, sizeof message_buf);
  sprintf(message_buf, "{\"returnValue\": true, \"delay\": %d, \"period\": %d}",
      current_delay, current_period);

  LSMessageRespond(message, message_buf, &lserror);

  return true;
}

bool set_repeat_rate(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  int delay = 0;
  int period = 0;
  bool use_default = false;

  object = LSMessageGetPayloadJSON(message);
  json_get_bool(object, "useDefault", &use_default);
  json_get_int(object, "delay", &delay);
  json_get_int(object, "period", &period);

  syslog(LOG_INFO, "useDefault %d, delay %d, period %d\n", use_default, delay, period);
  if (!use_default && (delay < 0 || period < 0 || delay > 3000 || period > 3000)) {
    LSMessageRespond(message, 
        "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Bad paramater\"}", &lserror);
  }

  if (use_default)
    set_repeat(DEFAULT_DELAY, DEFAULT_PERIOD);
  else 
    set_repeat(delay, period);

  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);

  return true;
}

bool set_modifiers(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  char *hold;
  char *doubletap;

  object = LSMessageGetPayloadJSON(message);
  json_get_string(object, "hold", &hold);
  json_get_string(object, "doubletap", &doubletap);

  if (hold) {
    if (!strcmp(hold, "on"))
      hold_enabled = 1;
    else if (!strcmp(hold, "off"))
      hold_enabled = 0;
  }

  if (doubletap) {
    if (!strcmp(doubletap, "on"))
      double_enabled = 1;
    else if (!strcmp(doubletap, "off"))
      double_enabled = 0;
  }

  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);
  
  return true;
}

bool get_status(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  memset(message_buf, 0, sizeof message_buf);
  sprintf(message_buf, "{\"returnValue\": true, \"u_fd\": %d, \"k_fd\": %d}", 
      u_fd, k_fd);

  LSMessageRespond(message, message_buf, &lserror);

  return true;
}

LSMethod luna_methods[] = { 
  {"getStatus", get_status},
  {"emulateKey", emulate_key},
  {"getRepeatRate", get_repeat_rate},
  {"setRepeatRate", set_repeat_rate},
  {"setModifiers", set_modifiers},
  {0,0}
};

bool register_methods(LSPalmService *serviceHandle, LSError lserror) {
	return LSPalmServiceRegisterCategory(serviceHandle, "/", luna_methods,
			NULL, NULL, NULL, &lserror);
}
