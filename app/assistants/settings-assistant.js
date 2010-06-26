function SettingsAssistant() {
	/* this is the creator function for your scene assistant object. It will be passed all the 
	   additional parameters (after the scene name) that were passed to pushScene. The reference
	   to the scene controller (this.controller) has not be established yet, so any initialization
	   that needs the scene controller should be done in the setup function below. */
}

SettingsAssistant.prototype.setup = function() {
	/* this function is for setup tasks that have to happen when the scene is first created */
		
  this.sliderAttributes = {
    modelProperty: 'value',
    maxValue: 3000,
    minValue: 0,
    round: false,
    updateInterval: 10
  };

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

  this.controller.setupWidget('delaySlider', this.sliderAttributes, {'value': 500});
  this.controller.setupWidget('periodSlider', this.sliderAttributes, {'value': 100});
  this.controller.setupWidget('preview', this.textAttributes, {});
  this.controller.setupWidget('defaultButton', {}, {buttonLabel: 'Reset'});
	
  this.periodSlider = this.controller.get('periodSlider');
  this.delaySlider = this.controller.get('delaySlider');
  this.defaultButton = this.controller.get('defaultButton');

	/* add event handlers to listen to events from widgets */
  this.handleRateChange = this.rateChange.bindAsEventListener(this);
  Mojo.Event.listen(this.delaySlider, 'mojo-property-change', this.handleRateChange);
  Mojo.Event.listen(this.periodSlider, 'mojo-property-change', this.handleRateChange);
};

SettingsAssistant.prototype.callback = function(payload) {
  for (p in payload) {
    Mojo.Log.error(p + ": " + payload[p]);
  }
}

SettingsAssistant.prototype.rateChange = function(event) {
  Mojo.Log.error("target " + event.target.id);
  if (event.target === this.periodSlider) {
    Mojo.Log.error("rate Change period");
    service.setRepeatRate(this.callback, -1, Math.floor(event.value), false);
  }
  else if (event.target === this.delaySlider) {
    Mojo.Log.error("rate Change delay");
    service.setRepeatRate(this.callback, Math.floor(event.value), -1, false);
  }
  else if (event.target === this.defaultButton) {
    Mojo.Log.error("rate Change default");
    service.setRepeatRate(this.callback, -1, -1, true);
  }
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
