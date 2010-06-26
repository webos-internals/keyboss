#include <stdbool.h>
#include <syslog.h>
#include "luna_service.h"
#include "keyboss.h"

bool emulate_key(LSHandle* lshandle, LSMessage *message, void *ctx) {
  int code;
  int keydown;
  json_t *object;
  LSError lserror;
  LSErrorInit(&lserror);
 
  object = LSMessageGetPayloadJSON(message);
  json_get_int(object, "code", &code);
  json_get_int(object, "keydown", &keydown);
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

  LSMessageRespond(message, 
    "{\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", &lserror);
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
