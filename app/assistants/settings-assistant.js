function SettingsAssistant() {
  this.holdList = false;
  this.holdListData = [];
  this.holdListCount = 0;
  this.tapList = false;
  this.tapListData = [];
  this.tapListCount = 0;

  this.cookie = new preferenceCookie();
  this.prefs = this.cookie.get();

  if (this.prefs.holdactions && this.prefs.holdactions.length > 0) {
    for (var i = 0; i < this.prefs.holdactions.length; i++) {
      this.holdListCount++;
      this.holdListData.push({id: this.holdListCount, index: i, value: this.prefs.holdactions[i]});
    }
  }

  if (this.prefs.tapactions && this.prefs.tapactions.length > 0) {
    for (var i = 0; i < this.prefs.tapactions.length; i++) {
      this.tapListCount++;
      this.tapListData.push({id: this.tapListCount, index: i, value: this.prefs.tapactions[i]});
    }
  }
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

  this.toggleAttributes = {
    trueValue: "on",
    falseValue: "off",
    modelProperty: 'value',
    enabledProp: 'disabled'
  }

  this.toggleModel = {
    value: false,
  }

  this.actionsAttributes = {
    itemTemplate: "settings/actions-row",
    swipeToDelete: true,
    reorderable: true,
    addItemLabel: 'Add',

    multiline: false,
    enterSubmits: false,
    modelProperty: 'value',
    changeOnKeyPress: true,
    maxLength: 8,
    focusMode: Mojo.Widget.focusSelectMode
  };

  this.holdListModel = {items:[]};
  this.tapListModel = {items:[]};

	/* use Mojo.View.render to render view templates and add them to the scene, if needed */
	
  this.holdList = this.controller.get('holdList');
  this.tapList = this.controller.get('tapList');
  this.freqSlider = this.controller.get('freqSlider');
  this.delaySlider = this.controller.get('delaySlider');
  this.defaultButton = this.controller.get('defaultButton');
  this.preview = this.controller.get('preview');

  this.holdListBuildList();
  this.tapListBuildList();

	/* setup widgets here */

  this.tmpAttr = {
label: "Action",
       multiline: false,
choices: [ {label: "Function", value: 1},
         {label: "Capitalize", value: 2} ]};
  this.tmpModel = {value: 2, disabled: false};
  this.delayModel = {value: 500};
  this.freqModel = {value: 10};

  this.controller.setupWidget('delaySlider', this.delayAttributes, 
      this.delayModel);
  this.controller.setupWidget('freqSlider', this.freqAttributes, 
      this.freqModel);
  this.controller.setupWidget('preview', this.textAttributes, {});
  this.controller.setupWidget('defaultButton', {}, {buttonLabel: 'Reset'});
  this.controller.setupWidget('holdToggle', this.toggleAttributes, 
      this.toggleModel);
  this.controller.setupWidget('doubleToggle', this.toggleAttributes, 
      this.toggleModel);
  this.controller.setupWidget('holdList', this.actionsAttributes, this.holdListModel);
  this.controller.setupWidget('tapList', this.actionsAttributes, this.tapListModel);
  this.controller.setupWidget('actionsField', this.tmpAttr, this.tmpModel);

	/* add event handlers to listen to events from widgets */
  this.handleRateChange = this.rateChange.bindAsEventListener(this);
  this.controller.listen(this.controller.sceneElement, Mojo.Event.keydown, this.keyDown.bind(this));

  if (this.delaySlider)
    Mojo.Event.listen(this.delaySlider, 'mojo-property-change', this.handleRateChange);
  if (this.freqSlider)
    Mojo.Event.listen(this.freqSlider, 'mojo-property-change', this.handleRateChange);
  if (this.defaultButton)
    Mojo.Event.listen(this.defaultButton, Mojo.Event.tap, this.setRateDefault.bindAsEventListener(this));

  if ($('holdToggle'))
    $('holdToggle').observe('mojo-property-change', this.toggleHold.bindAsEventListener());
  if ($('doubleToggle')) 
    $('doubleToggle').observe('mojo-property-change', this.toggleDouble.bindAsEventListener());

  service.getStatus(this.handleStatus.bind(this));
  Mojo.Event.listen(this.holdList, Mojo.Event.listAdd, this.holdListAdd.bindAsEventListener(this));
  Mojo.Event.listen(this.holdList, Mojo.Event.propertyChanged,	this.holdListChange.bindAsEventListener(this));
  Mojo.Event.listen(this.holdList, Mojo.Event.listReorder,		this.holdListReorder.bindAsEventListener(this));
  Mojo.Event.listen(this.holdList, Mojo.Event.listDelete,			this.holdListDelete.bindAsEventListener(this));

  Mojo.Event.listen(this.tapList, Mojo.Event.listAdd, this.tapListAdd.bindAsEventListener(this));
  Mojo.Event.listen(this.tapList, Mojo.Event.propertyChanged,	this.tapListChange.bindAsEventListener(this));
  Mojo.Event.listen(this.tapList, Mojo.Event.listReorder,		this.tapListReorder.bindAsEventListener(this));
  Mojo.Event.listen(this.tapList, Mojo.Event.listDelete,			this.tapListDelete.bindAsEventListener(this));
};

SettingsAssistant.prototype.holdListChange = function(event)
{
	this.holdListSave();
}

SettingsAssistant.prototype.holdListReorder = function(event)
{
	for (var d = 0; d < this.holdListData.length; d++) 
	{
		if (this.holdListData[d].index == event.fromIndex) 
		{
			this.holdListData[d].index = event.toIndex;
		}
		else 
		{
			if (event.fromIndex > event.toIndex) 
			{
				if (this.holdListData[d].index < event.fromIndex &&
				this.holdListData[d].index >= event.toIndex) 
				{
					this.holdListData[d].index++;
				}
			}
			else if (event.fromIndex < event.toIndex) 
			{
				if (this.holdListData[d].index > event.fromIndex &&
				this.holdListData[d].index <= event.toIndex) 
				{
					this.holdListData[d].index--;
				}
			}
		}
	}
	this.holdListSave();
}

SettingsAssistant.prototype.holdListDelete = function(event)
{
	var newData = [];
  this.holdListCount--;
	if (this.holdListData.length > 0) 
	{
		for (var d = 0; d < this.holdListData.length; d++) 
		{
			if (this.holdListData[d].id == event.item.id) 
			{
				// ignore
			}
			else 
			{
				if (this.holdListData[d].index > event.index) 
				{
					this.holdListData[d].index--;
				}
				newData.push(this.holdListData[d]);
			}
		}
	}
	this.holdListData = newData;
	this.holdListSave();
}

SettingsAssistant.prototype.holdListSave = function()
{
  Mojo.Log.error("SAVING LIST");
	if (this.holdListData.length > 0) 
	{
    Mojo.Log.error("1");
		if (this.holdListData.length > 1) 
		{
    Mojo.Log.error("2");
			this.holdListData.sort(function(a, b)
			{
				return a.index - b.index;
			});
		}
		
		for (var i = 0; i < this.holdListModel.items.length; i++) 
		{
    Mojo.Log.error("3");
			for (var d = 0; d < this.holdListData.length; d++) 
			{
    Mojo.Log.error("4");
				if (this.holdListData[d].id == this.holdListModel.items[i].id) 
				{
    Mojo.Log.error("5");
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
    for (var i=0; i<this.holdListData.length; i++) {
      this.holdListModel.items.push(this.holdListData[i]);
    }
  }
};

SettingsAssistant.prototype.holdListAdd = function(event) {
  if (this.holdListCount >= 2)
    return;
  this.holdListCount++;
  this.holdListData.push({id: this.holdListCount, index: this.holdListData.length, value: ''});
  this.holdListBuildList();
  this.holdList.mojo.noticeUpdatedItems(0, this.holdListModel.items);
  this.holdList.mojo.setLength(this.holdListModel.items.length);
  //this.holdList.mojo.focusItem(this.holdListModel.items[this.holdListModel.items.length-1]);
  this.holdListSave();
};

SettingsAssistant.prototype.tapListChange = function(event)
{
	this.tapListSave();
}

SettingsAssistant.prototype.tapListReorder = function(event)
{
	for (var d = 0; d < this.tapListData.length; d++) 
	{
		if (this.tapListData[d].index == event.fromIndex) 
		{
			this.tapListData[d].index = event.toIndex;
		}
		else 
		{
			if (event.fromIndex > event.toIndex) 
			{
				if (this.tapListData[d].index < event.fromIndex &&
				this.tapListData[d].index >= event.toIndex) 
				{
					this.tapListData[d].index++;
				}
			}
			else if (event.fromIndex < event.toIndex) 
			{
				if (this.tapListData[d].index > event.fromIndex &&
				this.tapListData[d].index <= event.toIndex) 
				{
					this.tapListData[d].index--;
				}
			}
		}
	}
	this.tapListSave();
}

SettingsAssistant.prototype.tapListDelete = function(event)
{
	var newData = [];
  this.tapListCount--;
	if (this.tapListData.length > 0) 
	{
		for (var d = 0; d < this.tapListData.length; d++) 
		{
			if (this.tapListData[d].id == event.item.id) 
			{
				// ignore
			}
			else 
			{
				if (this.tapListData[d].index > event.index) 
				{
					this.tapListData[d].index--;
				}
				newData.push(this.tapListData[d]);
			}
		}
	}
	this.tapListData = newData;
	this.tapListSave();
}

SettingsAssistant.prototype.tapListSave = function()
{
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
    for (var i=0; i<this.tapListData.length; i++) {
      this.tapListModel.items.push(this.tapListData[i]);
    }
  }
};

SettingsAssistant.prototype.tapListAdd = function(event) {
  if (this.tapListCount >= 2)
    return;
  this.tapListCount++;
  this.tapListData.push({id: this.tapListCount, index: this.tapListData.length, value: ''});
  this.tapListBuildList();
  this.tapList.mojo.noticeUpdatedItems(0, this.tapListModel.items);
  this.tapList.mojo.setLength(this.tapListModel.items.length);
  //this.tapList.mojo.focusItem(this.tapListModel.items[this.tapListModel.items.length-1]);
  this.tapListSave();
};

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
  Mojo.Log.error("dev = " + Mojo.Environment.DeviceInfo.modelName);
  Mojo.Log.error("dev = " + Mojo.Environment.DeviceInfo.modelNameAscii);
  if (Mojo.Environment.DeviceInfo.modelNameAscii === "Device")
  return;
  if (!payload || !payload.returnValue) {
    this.showError("Service does not seem to be running, try rebooting and then re-install if unsuccessful", this.close.bind(this));
  }
  else if (payload.k_fd < 0) {
    this.showError("Service reports keypad device cannot be opened, unfortunately NO functionality will work", this.close.bind(this));
  }
  else if (payload.u_fd < 0) {
    this.showError("Service reports uinput device cannot be opened.  The uinput module is required for KeyCaps functionality and keyboard emulation.  If you would like to use these functionalities, please make sure the Uinput module is installed via Preware and reboot.");
    this.toggleModel.disabled = true;
    this.controller.modelChanged(this.toggleModel, this);
  }
}

SettingsAssistant.prototype.toggleHold = function(event) {
  service.setModifiers(this.callback, event.value, 0);
}

SettingsAssistant.prototype.toggleDouble = function(event) {
  service.setModifiers(this.callback, 0, event.value);
}

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
  if (this.preview)
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
