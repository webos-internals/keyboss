#include <stdbool.h>
#include "luna_service.h"

bool emulate_key(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, "\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", 
      &lserror);
}

bool repeat_rate(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, "\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", 
      &lserror);
}

bool set_key_hold(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, "\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", 
      &lserror);
}

bool set_key_double(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, "\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", 
      &lserror);
}

bool set_mode(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  LSMessageRespond(message, "\"returnValue: -1\", \"errorText\": \"Not implemented yet\"}", 
      &lserror);
}

LSMethod luna_methods[] = { 
  {"emulateKey", emulate_key},
  {"repeatRate", repeat_rate},
  {"setKeyHold", set_key_hold},
  {"setKeyDouble", set_key_double},
  {"setMode", set_mode},
  {0,0}
};

bool register_methods(LSPalmService *serviceHandle, LSError lserror) {
	return LSPalmServiceRegisterCategory(serviceHandle, "/", luna_methods,
			NULL, NULL, NULL, &lserror);
}
