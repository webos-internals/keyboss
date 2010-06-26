service.identifier = 'palm://org.webosinternals.keyboss';

function service() {}

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
