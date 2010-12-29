#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <malloc.h>
#include <syslog.h>
#include "luna_service.h"
#include "keyboss.h"

struct action_map {
  ACTIONS action;
  char name[16];
};

static char message_buf[4048];
static char tmp[256];

static struct action_map available_actions[MAX_ACTIONS] = {
  {ACTION_DEFAULT, "Default"},
  {ACTION_FUNCTION, "Function"},
  {ACTION_CAPITALIZE, "Capitalize"},
  {ACTION_NONE, ""},
  {ACTION_NONE, ""},
  {ACTION_NONE, ""},
  {ACTION_NONE, ""},
  {ACTION_NONE, ""},
};

static get_action_code(char *name) {
  int i;

  if (!name)
    return ACTION_NONE;

  for (i=0; i<MAX_ACTIONS; i++) {
    if (available_actions[i].name[0] && !strcmp(available_actions[i].name, name)) {
      return available_actions[i].action;
    }
  }

  return ACTION_NONE;
}

bool emulate_key(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  int code = -1;
  bool keydown = false;
  json_t *object;
 
  object = json_parse_document(LSMessageGetPayload(message));
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

bool getFF(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  int threshold = 0;
  FILE *fd = NULL;
  
  fd = fopen(PROX_TIMEOUT, "r");

  if (!fd) {
    LSMessageRespond(message, "{\"returnValue\": false, \"errorCode\": -1}", &lserror);
    return true;
  }

  fscanf(fd, "%d", &threshold);
  fclose(fd);

  if (threshold < 0) {
    LSMessageRespond(message, "{\"returnValue\": false, \"errorCode\": -1}", &lserror);
    return true;
  }

  memset(message_buf, 0, sizeof message_buf);
  sprintf(message_buf, "{\"returnValue\": true, \"enable\": %s", (threshold) ? "true" : "false");

  LSMessageRespond(message, message_buf, &lserror);

  return true;
}

bool setFF(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  bool enable = false;
  FILE *fd = NULL;
  
  object = json_parse_document(LSMessageGetPayload(message));

  if (!json_find_first_label(object, "enable")) {
    LSMessageRespond(message, "{\"returnValue\": false, \"errorCode\": -1}", &lserror);
    return true;
  }

  json_get_bool(object, "enable", &enable);

  if (set_fffilter(enable)) {
    LSMessageRespond(message, "{\"returnValue\": false, \"errorCode\": -1}", &lserror);
    return true;
  }

  memset(message_buf, 0, sizeof message_buf);
  sprintf(message_buf, "{\"returnValue\": true, \"enable\": %s}", (enable) ? "true" : "false");

  LSMessageRespond(message, message_buf, &lserror);

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

  object = json_parse_document(LSMessageGetPayload(message));
  json_get_bool(object, "useDefault", &use_default);
  json_get_int(object, "delay", &delay);
  json_get_int(object, "period", &period);

  syslog(LOG_DEBUG, "useDefault %d, delay %d, period %d\n", use_default, delay, period);
  if (!use_default && (delay < 0 || delay > 3000) && (period < 0 || period > 3000)) {
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

bool change_action(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  char *param_trigger = NULL;
  char *param_action = NULL;
  int param_index = -1;
  ACTIONS action;
  int ret = 0;

  object = json_parse_document(LSMessageGetPayload(message));
  json_get_string(object, "trigger", &param_trigger);
  json_get_int(object, "index", &param_index);
  json_get_string(object, "action", &param_action);

  if (!param_trigger || !param_action || param_index < 0)
    goto err;

  action = get_action_code(param_action);
  if (action < ACTION_DEFAULT )
    goto err;

  if (!strcmp(param_trigger, "tap"))
    ret = change_tap_action(param_index, action);
  else if (!strcmp(param_trigger, "hold"))
    ret = change_hold_action(param_index, action);

  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);

  return true;

err:
  LSMessageRespond(message, "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Invalid or missing trigger/index/action\"}", &lserror);

  return true;
}
bool remove_action(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  char *param_trigger = NULL;
  char *param_action = NULL;
  int param_index = -1;
  ACTIONS action;
  int ret = 0;

  object = json_parse_document(LSMessageGetPayload(message));
  json_get_string(object, "trigger", &param_trigger);
  json_get_int(object, "index", &param_index);
  json_get_string(object, "action", &param_action);

  if (!param_trigger || (!param_action && param_index < 0))
    goto err;

  if (param_action) {
    syslog(LOG_DEBUG, "param action %s\n", param_action);
    action = get_action_code(param_action);
    if (action < ACTION_DEFAULT)
      goto err;
  }

  if (!strcmp(param_trigger, "tap"))
    ret = remove_tap_action(action, param_index);
  else if (!strcmp(param_trigger, "hold"))
    ret = remove_hold_action(action, param_index);

  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);

  return true;

err:
  LSMessageRespond(message, "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Invalid or missing trigger/index/action\"}", &lserror);

  return true;
}
bool install_action(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  char *param_trigger = NULL;
  char *param_action = NULL;
  ACTIONS action = ACTION_NONE;
  int ret = 0;

  object = json_parse_document(LSMessageGetPayload(message));
  json_get_string(object, "trigger", &param_trigger);
  json_get_string(object, "action", &param_action);

  if (!param_trigger || !param_action)
    goto err;

  action = get_action_code(param_action);
  if (action < ACTION_DEFAULT)
    goto err;

  if (!strcmp(param_trigger, "tap"))
    ret = install_tap_action(action);
  else if (!strcmp(param_trigger, "hold"))
    ret = install_hold_action(action);

  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);

  return true;

err:
  LSMessageRespond(message, "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Invalid or missing trigger/action\"}", &lserror);

  return true;
}

bool set_hold_timeout(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  int timeout = 0;

  object = json_parse_document(LSMessageGetPayload(message));
  json_get_int(object, "timeout", &timeout);

  syslog(LOG_DEBUG, "set hold timeout: %dms", timeout);
  if (timeout < 0 || timeout > 2000) {
    LSMessageRespond(message, 
        "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Bad paramater\"}", &lserror);
  }

  set_hold_timeout_ms(timeout);

  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);

  return true;
}

bool set_tap_timeout(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  int timeout = 0;

  object = json_parse_document(LSMessageGetPayload(message));
  json_get_int(object, "timeout", &timeout);

  syslog(LOG_DEBUG, "set tap timeout: %dms", timeout);
  if (timeout < 0 || timeout > 2000) {
    LSMessageRespond(message, 
        "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Bad paramater\"}", &lserror);
  }

  set_tap_timeout_ms(timeout);

  LSMessageRespond(message, "{\"returnValue\": true}", &lserror);

  return true;
}

#if 0
bool set_modifiers(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  json_t *object;
  char *hold = NULL;
  char *doubletap = NULL;

  object = json_parse_document(LSMessageGetPayload(message);
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
#endif

bool set_state(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit (&lserror);
  json_t *object;
  bool enable = false;
 
  object = json_parse_document(LSMessageGetPayload(message));
  json_get_bool(object, "enable", &enable);

  if (enable)
    start_pipe_thread();
  else
    stop_pipe_thread();

  LSMessageReply(lshandle, message, "{\"returnValue\": true}", &lserror);

  return true;
}

//TODO: Pixi/Pre
bool set_prox_timeout(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit (&lserror);
  FILE *fd;
  json_t *object;
  int prox_timeout = -1;
 
  object = json_parse_document(LSMessageGetPayload(message));
  json_get_int(object, "value", &prox_timeout);

  if (prox_timeout < 0) {
    LSMessageReply(lshandle, message, "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Must supply value\"}", &lserror);
    return true;
  }

  fd = fopen(PROX_TIMEOUT, "w");
  if (!fd || fprintf(fd, "%d", prox_timeout) < 0) {
    LSMessageReply(lshandle, message, "{\"returnValue\": false, \"errorCode\": -1, \"errorText\": \"Error writing to prox_timeout\"}", &lserror);
    return true;
  }

  LSMessageReply(lshandle, message, "{\"returnValue\": true}", &lserror);
  return true;
}

bool stickSettings(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  int i;
  int prox_timeout = -1;
  bool ffDisable = false;
  struct key_modifier *hold = &keystate.hold;
  struct key_modifier *tap = &keystate.tap;
  int tap_timeout = 0;
  int hold_timeout = 0;
  char *args = NULL;
  FILE *fd;

  fd = fopen(PROX_TIMEOUT, "r");
  if (fd) {
    fscanf(fd, "%d", &prox_timeout);
    fclose(fd);
    if (prox_timeout == 0)
      ffDisable = true;
  }

  for (i=0; i<hold->num_active; i++)
    asprintf(&args, "%s -h %d", (args) ? args : "", hold->actions[i]);

  for (i=0; i<tap->num_active; i++)
    asprintf(&args, "%s -t %d", (args) ? args : "", tap->actions[i]);

  hold_timeout = hold_timer.value.it_value.tv_sec * 1000 + hold_timer.value.it_value.tv_nsec / 1000000;
  tap_timeout = tap_timer.value.it_value.tv_sec * 1000 + tap_timer.value.it_value.tv_nsec / 1000000;

  asprintf(&args, "%s -H %d -T %d -r %d %d %s %s", 
      (args) ? args : "", 
      hold_timeout, 
      tap_timeout, 
      current_delay, 
      current_period, 
      (ffDisable) ? "-f" : "",
      (pipe_id) ? "-e" : "");

  fd = fopen(ARGS_FILE, "w");
  if (!fd) {
    LSMessageReply(lshandle, message, "{\"returnValue\": false, \"errorCode\": -1}", &lserror);
    free(args);
    return true;
  }

  fprintf(fd, "%s", args);
  fclose(fd);
  free(args);
  LSMessageReply(lshandle, message, "{\"returnValue\": true}", &lserror);
  return true;
}

bool unstickSettings(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  FILE *fd;

  fd = fopen(ARGS_FILE, "w");
  if (!fd) {
    LSMessageReply(lshandle, message, "{\"returnValue\": false, \"errorCode\": -1}", &lserror);
    return true;
  }

  fprintf(fd, "");
  fclose(fd);
  LSMessageReply(lshandle, message, "{\"returnValue\": true}", &lserror);
  return true;
}

bool resetToDefaults(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);

  reset_to_defaults();

  LSMessageReply(lshandle, message, "{\"returnValue\": true}", &lserror);
  return true;
}

// TODO: status should be subscription
bool get_status(LSHandle* lshandle, LSMessage *message, void *ctx) {
  LSError lserror;
  LSErrorInit(&lserror);
  int i;
  int count = 0;
  FILE *fd;
  int prox_timeout = -1;
  char buffer[4];
  struct key_modifier *hold = &keystate.hold;
  struct key_modifier *tap = &keystate.tap;
  char *actions = NULL;
  char *installed_hold = NULL;
  char *installed_tap = NULL;
  int tap_timeout = 0;
  int hold_timeout = 0;

  syslog(LOG_DEBUG, "in get status");
  fd = fopen(PROX_TIMEOUT, "r");
  if (fd) {
    fscanf(fd, "%d", &prox_timeout);
    fclose(fd);
  }

  syslog(LOG_DEBUG, "get actions");
  count = 0;
  asprintf(&actions, "[");
  for (i=0; i<MAX_ACTIONS; i++) {
    if (available_actions[i].name[0]) {
      if (count++) 
        asprintf(&actions, "%s,", actions);
      asprintf(&actions, "%s\"%s\"", actions, available_actions[i].name);
    }
  }
  asprintf(&actions, "%s]", actions);

  syslog(LOG_DEBUG, "get installed_hold");
  count = 0;
  asprintf(&installed_hold, "[");
  syslog(LOG_DEBUG, "hold %p", hold);
  for (i=0; i<hold->num_active; i++) {
    syslog(LOG_DEBUG, "i %d", i);
    if (count++) 
      asprintf(&installed_hold, "%s,", installed_hold);
    asprintf(&installed_hold, "%s%d", installed_hold, hold->actions[i]);
  }
  asprintf(&installed_hold, "%s]", installed_hold);

  syslog(LOG_DEBUG, "get installed_tap");
  count = 0;
  asprintf(&installed_tap, "[");
  for (i=0; i<tap->num_active; i++) {
    if (count++) 
      asprintf(&installed_tap, "%s,", installed_tap);
    asprintf(&installed_tap, "%s%d", installed_tap, tap->actions[i]);
  }
  asprintf(&installed_tap, "%s]", installed_tap);

  tap_timeout = tap_timer.value.it_value.tv_sec * 1000 + tap_timer.value.it_value.tv_nsec / 1000000;
  hold_timeout = hold_timer.value.it_value.tv_sec * 1000 + hold_timer.value.it_value.tv_nsec / 1000000;

  memset(message_buf, 0, sizeof message_buf);
  sprintf(message_buf, "{\"returnValue\": true, \"enabled\": %s, \"u_fd\": %d, \"k_fd\": %d, \"max_actions\": %d, \"actions\": %s, \"installed_hold\": %s, \"installed_tap\": %s, \"tap_timeout\": %d, \"hold_timeout\": %d, \"hold_delay\": %d, \"hold_interval\": %d, \"prox_timeout\": %d}", (pipe_id) ? "true" : "false", u_fd, k_fd, MAX_ACTIONS, actions, installed_hold, installed_tap, tap_timeout, hold_timeout, current_delay, current_period, prox_timeout);

  LSMessageRespond(message, message_buf, &lserror);
  if (actions) free(actions);
  if (installed_hold) free(installed_hold);
  if (installed_tap) free(installed_tap);

  return true;
}

LSMethod luna_methods[] = { 
  {"getStatus", get_status},
  {"setState", set_state},
  {"emulateKey", emulate_key},
  {"getRepeatRate", get_repeat_rate},
  {"setRepeatRate", set_repeat_rate},
  {"getFF", getFF},
  {"setFF", setFF},
  //{"setModifiers", set_modifiers},
  {"installAction", install_action},
  {"removeAction", remove_action},
  {"changeAction", change_action},
  {"setTapTimeout", set_tap_timeout},
  {"setHoldTimeout", set_hold_timeout},
  {"setProxTimeout", set_prox_timeout},
  {"stickSettings", stickSettings},
  {"unstickSettings", unstickSettings},
  {"resetToDefaults", resetToDefaults},
  {0,0}
};

bool register_methods(LSPalmService *serviceHandle, LSError lserror) {
	return LSPalmServiceRegisterCategory(serviceHandle, "/", luna_methods,
			NULL, NULL, NULL, &lserror);
}
