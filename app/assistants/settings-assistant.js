function SettingsAssistant() {
  this.holdList = false;
  this.holdListData = [];
  this.holdListCount = 0;
  this.tapList = false;
  this.tapListData = [];
  this.tapListCount = 0;

  this.loadingActions = true;
  this.actions = [];
  this.maxActions = 0;

  this.cookie = new preferenceCookie();
  this.prefs = this.cookie.get();
}

SettingsAssistant.prototype.setup = function() {
	/* this function is for setup tasks that have to happen when the scene is first created */
		
  this.sliderAttributes = {
    modelProperty: 'value',
    maxValue: 1000,
    minValue: 0,
    round: false,
    updateInterval: 1
  }

  this.actionsAttributes = {
    itemTemplate: "settings/actions-row",
    swipeToDelete: true,
    reorderable: false,
    addItemLabel: 'Add',

    multiline: false,
    enterSubmits: false,
    modelProperty: 'value',
    changeOnKeyPress: true,
    maxLength: 8,
    focusMode: Mojo.Widget.focusSelectMode
  };

  this.textAttributes = {
    focusMode: Mojo.Widget.focusSelectMode,
    autoReplace: false,
    textCase: Mojo.Widget.steModeLowerCase,
    changeOnKeyPress: true
  };

  this.holdListModel = {items:[]};
  this.tapListModel = {items:[]};

	/* use Mojo.View.render to render view templates and add them to the scene, if needed */
	
  this.holdTimeGroup = this.controller.get('holdTimeGroup');
  this.tapTimeGroup = this.controller.get('tapTimeGroup');
  this.holdList = this.controller.get('holdList');
  this.tapList = this.controller.get('tapList');
  this.freqSlider = this.controller.get('freqSlider');
  this.delaySlider = this.controller.get('delaySlider');
  this.tapSlider = this.controller.get('tapSlider');
  this.holdSlider = this.controller.get('holdSlider');
  this.enableToggle = this.controller.get('enableToggle');
  this.previewHeader = this.controller.get('previewHeader');
  this.preview = this.controller.get('preview');

	/* setup widgets here */

  //TODO: get default from service
  this.delayModel = {value: 500};
  this.freqModel = {value: 100};
  this.tapModel = {value: 250};
  this.holdModel = {value: 250};

  this.controller.setupWidget('delaySlider', this.sliderAttributes, this.delayModel);
  this.controller.setupWidget('freqSlider', this.sliderAttributes, this.freqModel);
  this.controller.setupWidget('tapSlider', this.sliderAttributes, this.tapModel);
  this.controller.setupWidget('holdSlider', this.sliderAttributes, this.holdModel);
  this.controller.setupWidget('holdList', this.actionsAttributes, this.holdListModel);
  this.controller.setupWidget('tapList', this.actionsAttributes, this.tapListModel);
  this.controller.setupWidget('enableToggle', {}, {value: true});
  this.controller.setupWidget('preview', this.textAttributes, {});

	/* add event handlers to listen to events from widgets */
  this.handleRateChange = this.rateChange.bindAsEventListener(this);
  this.handleTapTimeoutChange = this.tapTimeoutChange.bindAsEventListener(this);
  this.handleHoldTimeoutChange = this.holdTimeoutChange.bindAsEventListener(this);
  this.handleEnableChange = this.enableChange.bindAsEventListener(this);

  this.holdListFinishAdd = this.holdListFinishAdd.bind(this);
  this.holdListFinishChange = this.holdListFinishChange.bind(this);
  this.tapListFinishAdd = this.tapListFinishAdd.bind(this);
  this.tapListFinishChange = this.tapListFinishChange.bind(this);

  this.keyHandler = this.keyHandler.bindAsEventListener(this);
  this.tapHandler = this.tapHandler.bindAsEventListener(this);
  this.previewChange = this.previewChange.bindAsEventListener(this);
  this.hidePreview = this.hidePreview.bind(this);

  this.preview.hide();
  this.previewHeader.hide();

  Mojo.Event.listen(this.delaySlider, 'mojo-property-change', this.handleRateChange);
  Mojo.Event.listen(this.freqSlider, 'mojo-property-change', this.handleRateChange);
  Mojo.Event.listen(this.tapSlider, 'mojo-property-change', this.handleTapTimeoutChange);
  Mojo.Event.listen(this.holdSlider, 'mojo-property-change', this.handleHoldTimeoutChange);
  Mojo.Event.listen(this.enableToggle, 'mojo-property-change', this.handleEnableChange);
  Mojo.Event.listen(this.holdList, Mojo.Event.listAdd, this.holdListAdd.bindAsEventListener(this));
  Mojo.Event.listen(this.holdList, Mojo.Event.propertyChanged,	this.holdListChange.bindAsEventListener(this));
  Mojo.Event.listen(this.holdList, Mojo.Event.listDelete,			this.holdListDelete.bindAsEventListener(this));

  Mojo.Event.listen(this.tapList, Mojo.Event.listAdd, this.tapListAdd.bindAsEventListener(this));
  Mojo.Event.listen(this.tapList, Mojo.Event.propertyChanged,	this.tapListChange.bindAsEventListener(this));
  Mojo.Event.listen(this.tapList, Mojo.Event.listDelete,			this.tapListDelete.bindAsEventListener(this));

  Mojo.Event.listen(this.controller.window, 'keydown', this.keyHandler);
  Mojo.Event.listen(this.controller.window, 'keyup', this.keyHandler);

  Mojo.Event.listen(this.preview, Mojo.Event.tap, this.tapHandler);

  Mojo.Event.listen(this.preview, Mojo.Event.propertyChange, this.previewChange);
  service.getStatus(this.handleStatus.bind(this));
};

SettingsAssistant.prototype.tapHandler = function() {
  this.hidePreview();
}

SettingsAssistant.prototype.previewChange = function(event) {
  //TODO: Not working... why?
  if (event.value == '') {
    this.hidePreview();
  }
}

SettingsAssistant.prototype.hidePreview = function() {
  this.preview.mojo.blur();
  this.preview.hide();
  this.previewHeader.hide();
  this.controller.listen(this.controller.window, 'keydown', this.keyHandler);
  this.controller.listen(this.controller.window, 'keyup', this.keyHandler);
}

SettingsAssistant.prototype.keyHandler = function(event) {
  if (event.keyCode === Mojo.Char.metaKey) {
    return;
  }

  this.controller.stopListening(this.controller.window, 'keydown', this.keyHandler);
  this.controller.stopListening(this.controller.window, 'keyup', this.keyHandler);
  this.previewHeader.show();
  this.preview.show();
  this.preview.mojo.focus();
  if (this.previewTimer)
    clearTimeout(this.previewTimer);
  //this.previewTimer = setTimeout(this.hidePreview, 3000);
}

SettingsAssistant.prototype.setupActionWidgets = function(payload) {
  var choices = [];

  for (var i=0; i<this.maxActions; i++) {
    if (this.actions[i])
      choices.push({label: this.actions[i], value: i});
  }

  var attributes = {
    label: "Action", 
    multiline: false, 
    choices: choices
  };

  var model = {value: 0, disabled: false};

  this.holdListBuildList();
  this.tapListBuildList();

  this.controller.setupWidget('actionsField', attributes, model);
  this.controller.modelChanged(this.holdListModel, this);
  this.controller.modelChanged(this.tapListModel, this);
}

SettingsAssistant.prototype.holdListFinishChange = function(payload)
{
  if (payload.returnValue)
	  this.holdListSave();
  else
    Mojo.Log.error(payload.errorText);
}

SettingsAssistant.prototype.holdListChange = function(event) {
  var index = event.model.index;

  if (event.property === 'value')
    service.changeAction(this.holdListFinishChange, 'hold', index, this.actions[event.value]);
}

SettingsAssistant.prototype.holdListFinishDelete = function(id, index, payload)
{
  if (payload.returnValue) {
    this.holdListData = this.holdListData.without(this.holdListData[index]);
    this.holdListCount--;
    if (!this.holdListCount)
      this.holdTimeGroup.hide();
	  this.holdListSave();
  }
  else {
    Mojo.Log.error(payload.errorText);
  }
}

SettingsAssistant.prototype.holdListDelete = function(event)
{
  service.removeAction(this.holdListFinishDelete.bind(this,event.item.id,event.index), 'hold', event.index);
}

SettingsAssistant.prototype.holdListSave = function()
{
  return;
	if (this.holdListData.length > 0) 
	{
		if (this.holdListData.length > 1) 
		{
			this.holdListData.sort(function(a, b)
			{
				return a.index - b.index;
			});
		}
		
		for (var i = 0; i < this.holdListModel.items.length; i++) 
		{
			for (var d = 0; d < this.holdListData.length; d++) 
			{
				if (this.holdListData[d].id == this.holdListModel.items[i].id) 
				{
					this.holdListData[d].value = this.holdListModel.items[i].value;
				}
			}
		}
	}
	
	this.prefs.holdactions = [];
	if (this.holdListData.length > 0) 
	{
		for (var d = 0; d < this.holdListData.length; d++) 
		{
			if (this.holdListData[d].value) 
			{
				this.prefs.holdactions.push(this.holdListData[d].value);
			}
		}
	}
	
	this.cookie.put(this.prefs);
	//this.validateIdentity();
}

SettingsAssistant.prototype.holdListBuildList = function() {
  this.holdListModel.items = [];
  if (this.holdListData.length > 0) {
    this.holdTimeGroup.show();
    for (var i=0; i<this.holdListData.length; i++) {
      this.holdListModel.items.push(this.holdListData[i]);
    }
  }
  else {
    this.holdTimeGroup.hide();
  }
};

SettingsAssistant.prototype.holdListFinishAdd = function(payload) {
  if (payload.returnValue) {
    this.holdListCount++;
    this.holdListData.push({id: this.holdListCount, index: this.holdListData.length, value: 0});
    this.holdListBuildList();
    this.holdList.mojo.noticeUpdatedItems(0, this.holdListModel.items);
    this.holdList.mojo.setLength(this.holdListModel.items.length);
    //this.holdList.mojo.focusItem(this.holdListModel.items[this.holdListModel.items.length-1]);
    this.holdListSave();
  }
  else {
    Mojo.Log.error(payload.errorText);
  }
}

SettingsAssistant.prototype.holdListAdd = function(event) {
  if (this.loadingActions || this.holdListCount >= this.maxActions)
    return;

  service.installAction(this.holdListFinishAdd, 'hold', this.actions[0]);
};

SettingsAssistant.prototype.tapListFinishChange = function(payload)
{
  if (payload.returnValue)
	  this.tapListSave();
  else
    Mojo.Log.error(payload.errorText);
}

SettingsAssistant.prototype.tapListChange = function(event)
{
  var index = event.model.index;

  if (event.property === 'value')
    service.changeAction(this.tapListFinishChange, 'tap', index, this.actions[event.value]);
}

SettingsAssistant.prototype.tapListFinishDelete = function(id, index, payload)
{
  if (payload.returnValue) {
    this.tapListData = this.tapListData.without(this.tapListData[index]);
    this.tapListCount--;
    if (!this.tapListCount)
      this.tapTimeGroup.hide();
	  this.tapListSave();
  }
  else {
    Mojo.Log.error(payload.errorText);
  }
}

SettingsAssistant.prototype.tapListDelete = function(event)
{
  service.removeAction(this.tapListFinishDelete.bind(this,event.item.id,event.index), 'tap', event.index);
}


SettingsAssistant.prototype.tapListSave = function()
{
  return;
	if (this.tapListData.length > 0) 
	{
		if (this.tapListData.length > 1) 
		{
			this.tapListData.sort(function(a, b)
			{
				return a.index - b.index;
			});
		}
		
		for (var i = 0; i < this.tapListModel.items.length; i++) 
		{
			for (var d = 0; d < this.tapListData.length; d++) 
			{
				if (this.tapListData[d].id == this.tapListModel.items[i].id) 
				{
					this.tapListData[d].value = this.tapListModel.items[i].value;
				}
			}
		}
	}
	
	this.prefs.tapactions = [];
	if (this.tapListData.length > 0) 
	{
		for (var d = 0; d < this.tapListData.length; d++) 
		{
			if (this.tapListData[d].value) 
			{
				this.prefs.tapactions.push(this.tapListData[d].value);
			}
		}
	}
	
	this.cookie.put(this.prefs);
	//this.validateIdentity();
}

SettingsAssistant.prototype.tapListBuildList = function() {
  this.tapListModel.items = [];
  if (this.tapListData.length > 0) {
    this.tapTimeGroup.show();
    for (var i=0; i<this.tapListData.length; i++) {
      this.tapListModel.items.push(this.tapListData[i]);
    }
  }
  else {
    this.tapTimeGroup.hide();
  }
};

SettingsAssistant.prototype.tapListFinishAdd = function(payload) {
  if (payload.returnValue) {
    this.tapListCount++;
    this.tapListData.push({id: this.tapListCount, index: this.tapListData.length, value: 0});
    this.tapListBuildList();
    this.tapList.mojo.noticeUpdatedItems(0, this.tapListModel.items);
    this.tapList.mojo.setLength(this.tapListModel.items.length);
    //this.tapList.mojo.focusItem(this.tapListModel.items[this.tapListModel.items.length-1]);
    this.tapListSave();
  }
  else {
    Mojo.Log.error(payload.errorText);
  }
};

SettingsAssistant.prototype.tapListAdd = function(event) {
  if (this.loadingActions || this.tapListCount >= this.maxActions)
    return;

  service.installAction(this.tapListFinishAdd, 'tap', this.actions[0]);
}

SettingsAssistant.prototype.showError = function(message, callback) {
    this.controller.showAlertDialog(
        {
          title: $LL("Error"),
          message: message,
          onChoose: callback,
          choices: [{label: $LL("OK")}]
        });
}

SettingsAssistant.prototype.close = function() {
  window.close();
}

SettingsAssistant.prototype.handleStatus = function(payload) {
  if (Mojo.Environment.DeviceInfo.modelNameAscii === "Device")
    return;

  if (!payload || !payload.returnValue) {
    this.callback(payload);
    this.showError("Service does not seem to be running, try rebooting and then re-install if unsuccessful", this.close.bind(this));
  }
  else if (payload.k_fd < 0) {
    this.showError("Service reports keypad device cannot be opened, unfortunately NO functionality will work", this.close.bind(this));
  }
  else if (payload.u_fd < 0) {
    this.showError("Service reports uinput device cannot be opened.  The uinput module is required for KeyCaps functionality and keyboard emulation.  If you would like to use these functionalities, please make sure the Uinput module is installed via Preware and reboot.");
  }
  else {
    this.actions = payload.actions.clone();
    this.maxActions = payload.max_actions;

    if (payload.installed_hold && payload.installed_hold.length > 0) {
      for (var i = 0; i < payload.installed_hold.length; i++) {
        this.holdListCount++;
        this.holdListData.push({id: this.holdListCount, index: i, value: payload.installed_hold[i]});
      }
    }

    if (payload.installed_tap && payload.installed_tap.length > 0) {
      for (var i = 0; i < payload.installed_tap.length; i++) {
        this.tapListCount++;
        this.tapListData.push({id: this.tapListCount, index: i, value: payload.installed_tap[i]});
      }
    }

    if (payload.tap_timeout) {
      this.tapModel.value = payload.tap_timeout;
      this.controller.modelChanged(this.tapModel, this);
    }

    if (payload.hold_timeout) {
      this.holdModel.value = payload.hold_timeout;
      this.controller.modelChanged(this.holdModel, this);
    }

    if (payload.hold_delay) {
      this.delayModel.value = payload.hold_delay;
      this.controller.modelChanged(this.delayModel, this);
    }

    if (payload.hold_interval) {
      this.freqModel.value = payload.hold_interval;
      this.controller.modelChanged(this.freqModel, this);
    }
    this.setupActionWidgets();
    this.loadingActions = false;
  }
}

SettingsAssistant.prototype.callback = function(payload) {
  for (p in payload) {
    Mojo.Log.info(p + ": " + payload[p]);
  }
}

SettingsAssistant.prototype.handleGet = function(payload) {
  for (p in payload) {
    Mojo.Log.error(p + ": " + payload[p]);
  }

  this.delayModel.value = payload.delay;
  this.freqModel.value = Math.floor(payload.period);
  this.controller.modelChanged(this.delayModel, this);
  this.controller.modelChanged(this.freqModel, this);
}

SettingsAssistant.prototype.setRateDefault = function(event) {
  this.delayModel.value = 500
  this.freqModel.value = 100;
  this.controller.modelChanged(this.delayModel, this);
  this.controller.modelChanged(this.freqModel, this);
  service.setRepeatRate(this.callback, -1, -1, true);
  service.getRepeatRate(this.handleGet.bind(this));
}

SettingsAssistant.prototype.enableChange = function(event) {
  service.setState(this.callback, event.value);
}

SettingsAssistant.prototype.holdTimeoutChange = function(event) {
  service.setHoldTimeout(this.callback, Math.floor(event.value));
}

SettingsAssistant.prototype.tapTimeoutChange = function(event) {
  service.setTapTimeout(this.callback, Math.floor(event.value));
}

SettingsAssistant.prototype.rateChange = function(event) {
  if (event.target === this.freqSlider)
    service.setRepeatRate(this.callback, -1, Math.floor(event.value), false);
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
