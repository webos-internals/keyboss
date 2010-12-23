service.identifier = 'palm://org.webosinternals.keyboss';

function service() {}

service.getStatus = function(callback) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'getStatus',
    parameters: {},
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}

service.setState = function(callback, enable) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'setState',
    parameters: {
      enable: enable
    },
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}
service.getRepeatRate = function(callback) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'getRepeatRate',
    parameters: {},
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}

service.setRepeatRate = function(callback, delay, period, useDefault) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'setRepeatRate',
    parameters: {
      delay: delay,
      period: period,
      useDefault: useDefault
    },
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}

service.setModifiers = function(callback, hold, doubletap) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'setModifiers',
    parameters: {
      hold: hold,
      doubletap: doubletap,
    },
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}

service.setTapTimeout = function(callback, timeout) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'setTapTimeout',
    parameters: {
      timeout: timeout
    },
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}

service.setHoldTimeout = function(callback, timeout) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'setHoldTimeout',
    parameters: {
      timeout: timeout
    },
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}

service.installAction = function(callback, trigger, action) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'installAction',
    parameters: {
      trigger: trigger,
      action: action
    },
    onSuccess: callback,
    onFailure: callback
  });
}

service.removeAction = function(callback, trigger, index, action) {
  Mojo.Log.error("remove action trigger " + trigger + " index " + index + " action " + action);
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'removeAction',
    parameters: {
      trigger: trigger,
      action: action,
      index: index
    },
    onSuccess: callback,
    onFailure: callback
  });
}

service.changeAction = function(callback, trigger, index, action) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'changeAction',
    parameters: {
      trigger: trigger,
      index: index,
      action: action
    },
    onSuccess: callback,
    onFailure: callback
  });
}

service.setFF = function(callback, enable) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'setFF',
    parameters: {
      enable: enable
    },
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}

service.getFF = function(callback) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'getFF',
    parameters: {},
    onSuccess: callback,
    onFailure: callback
  });

  return request;
}
