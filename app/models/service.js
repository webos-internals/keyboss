service.identifier = 'palm://org.webosinternals.keyboss';

function service() {}

service.getRepeatRate = function(callback) {
  var request = new Mojo.Service.Request(service.identifier, {
    method: 'getRepeatRate',
    parameters: {},
    onSuccess: callback,
    onFailure: callback
  });
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
