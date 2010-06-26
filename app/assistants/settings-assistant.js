function SettingsAssistant() {
	/* this is the creator function for your scene assistant object. It will be passed all the 
	   additional parameters (after the scene name) that were passed to pushScene. The reference
	   to the scene controller (this.controller) has not be established yet, so any initialization
	   that needs the scene controller should be done in the setup function below. */
}

SettingsAssistant.prototype.setup = function() {
	/* this function is for setup tasks that have to happen when the scene is first created */
		
  this.delayAttributes = {
    modelProperty: 'value',
    maxValue: 1000,
    minValue: 0,
    round: false,
    updateInterval: 10
  };

  this.freqAttributes = {
    modelProperty: 'value',
    maxValue: 90,
    minValue: 0,
    round: false,
    updateInterval: 5
  }

  this.textAttributes = {
    multiline: false,
    enterSubmits: false,
    hintText: '',
    maxLength: 100,
    textCase: Mojo.Widget.steModeLowerCase,
    focusMode: Mojo.Widget.focusInsertMode
  }

	/* use Mojo.View.render to render view templates and add them to the scene, if needed */
	
	/* setup widgets here */

  this.delayModel = {value: 500};
  this.freqModel = {value: 10};

  this.controller.setupWidget('delaySlider', this.delayAttributes, 
      this.delayModel);
  this.controller.setupWidget('freqSlider', this.freqAttributes, 
      this.freqModel);
  this.controller.setupWidget('preview', this.textAttributes, {});
  this.controller.setupWidget('defaultButton', {}, {buttonLabel: 'Reset'});
	
  this.freqSlider = this.controller.get('freqSlider');
  this.delaySlider = this.controller.get('delaySlider');
  this.defaultButton = this.controller.get('defaultButton');
  this.preview = this.controller.get('preview');

	/* add event handlers to listen to events from widgets */
  this.handleRateChange = this.rateChange.bindAsEventListener(this);
  Mojo.Event.listen(this.delaySlider, 'mojo-property-change', 
      this.handleRateChange);
  Mojo.Event.listen(this.freqSlider, 'mojo-property-change', 
      this.handleRateChange);
  Mojo.Event.listen(this.defaultButton, Mojo.Event.tap, 
      this.setRateDefault.bindAsEventListener(this));
  this.controller.listen(this.controller.sceneElement, Mojo.Event.keydown, this.keyDown.bind(this));
};

SettingsAssistant.prototype.callback = function(payload) {
  for (p in payload) {
    Mojo.Log.error(p + ": " + payload[p]);
  }
}

SettingsAssistant.prototype.handleGet = function(payload) {
  for (p in payload) {
    Mojo.Log.error(p + ": " + payload[p]);
  }

  this.delayModel.value = payload.delay;
  this.freqModel.value = Math.floor(1000/payload.period);
  this.controller.modelChanged(this.delayModel, this);
  this.controller.modelChanged(this.freqModel, this);
}

SettingsAssistant.prototype.keyDown = function(event) {
  this.preview.mojo.focus();
}

SettingsAssistant.prototype.setRateDefault = function(event) {
  this.delayModel.value = 500
  this.freqModel.value = 10;
  this.controller.modelChanged(this.delayModel, this);
  this.controller.modelChanged(this.freqModel, this);
  service.setRepeatRate(this.callback, -1, -1, true);
  service.getRepeatRate(this.handleGet.bind(this));
}

SettingsAssistant.prototype.rateChange = function(event) {
  Mojo.Log.error("value " + event.value);
  if (event.target === this.freqSlider)
    service.setRepeatRate(this.callback, -1, Math.floor(1000/event.value), false);
  else if (event.target === this.delaySlider)
    service.setRepeatRate(this.callback, Math.floor(event.value), -1, false);
}

SettingsAssistant.prototype.activate = function(event) {
	/* put in event handlers here that should only be in effect when this scene is active. For
	   example, key handlers that are observing the document */
};

SettingsAssistant.prototype.deactivate = function(event) {
	/* remove any event handlers you added in activate and do any other cleanup that should happen before
	   this scene is popped or another scene is pushed on top */
};

SettingsAssistant.prototype.cleanup = function(event) {
	/* this function should do any cleanup needed before the scene is destroyed as 
	   a result of being popped off the scene stack */
};
