service.identifier = 'palm://org.webosinternals.keyboss';

function service() {}

service.getRepeatRate = function(callback) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'getStatus',
    parameters: {},
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
