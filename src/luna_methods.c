#include <stdbool.h>
#include <syslog.h>
#include "luna_service.h"
#include "keyboss.h"

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
        "{\"returnValue: -1\", \"errorText\": \"Invalid key code\"}", &lserror);
    return false;
  }

  send_key(code, keydown);
  LSMessageRespond(message, "{\"returnValue: 0\"}", &lserror);

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
        "{\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", &lserror);
  }

  if (use_default)
    set_repeat(DEFAULT_DELAY, DEFAULT_PERIOD);
  else 
    set_repeat(delay, period);

  LSMessageRespond(message, "{\"returnValue: 0\"}", &lserror);

  return true;
}

bool set_key_hold(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, 
    "{\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", &lserror);
}

bool set_key_double(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, 
      "{\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", &lserror);
}

bool set_mode(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, 
      "{\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", &lserror);
}

LSMethod luna_methods[] = { 
  {"emulateKey", emulate_key},
  {"setRepeatRate", set_repeat_rate},
  {"setKeyHold", set_key_hold},
  {"setKeyDouble", set_key_double},
  {"setMode", set_mode},
  {0,0}
};

bool register_methods(LSPalmService *serviceHandle, LSError lserror) {
	return LSPalmServiceRegisterCategory(serviceHandle, "/", luna_methods,
			NULL, NULL, NULL, &lserror);
}
