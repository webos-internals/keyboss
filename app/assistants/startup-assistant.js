function StartupAssistant()
{
    // on first start, this message is displayed, along with the current version message from below
    this.firstMessage = $L('Here are some tips for first-timers:<ul><li>Start typing at any time to bring up preview header.  Tap the header to hide it.</li></ul>');
	
    this.secondMessage = $L('Take total control over your keyboard.<br>Please consider making a <a href=http://www.webos-internals.org/wiki/WebOS_Internals:Site_support>donation</a> if you wish to show your appreciation.');
	
    // on new version start
    this.newMessages =
	[
		// Don't forget the comma on all but the last entry
    { version: '0.2.9', log: [ 'Added fat finger filter control',
                               'Added persistent settings',
                               'Added ability to reset to defaults',
                               'Added code to disable after unclean shutdown' ]},
    { version: '0.2.6', log: [ 'Fixed removal of tap action' ]},
    { version: '0.2.5', log: [ 'Added preview header for testing settings',
                               'Changed logic to not cycle when only 1 action active',
                               'Fixed tap action bug that missed keys in universal search',
                               'Added start/changelog scene' ]},
    { version: '0.2.4', log: [ 'Added Enable/Disable toggle' ]},
    { version: '0.2.0', log: [ 'Re-designed with trigger/action lists']},
		{ version: '0.1.0', log: [ 'Initial Release!' ] }
	];
	
    // setup menu
    this.menuModel =
	{
	    visible: true,
	    items:
	    [
		    {
				label: $L("Help"),
				command: 'do-help'
		    }
	     ]
	};
	
    // setup command menu
    this.cmdMenuModel =
	{
	    visible: false, 
	    items:
		[
			{},
			{
				label: $L("Ok, I've read this. Let's continue ..."),
				command: 'do-continue'
			},
			{}
		]
	};
};

StartupAssistant.prototype.setup = function()
{
    // set theme because this can be the first scene pushed
    this.controller.document.body.className = prefs.get().theme;
	
    // get elements
    this.titleContainer = this.controller.get('title');
    this.dataContainer =  this.controller.get('data');
	
    // set title
    if (vers.isFirst)
	{
		this.titleContainer.innerHTML = $L('Welcome To KeyBoss');
    }
    else if (vers.isNew)
	{
		this.titleContainer.innerHTML = $L('KeyBoss Changelog');
    }
	
	
    // build data
    var html = '';
    if (vers.isFirst) {
	html += '<div class="text">' + this.firstMessage + '</div>';
    }
    if (vers.isNew) {
	html += '<div class="text">' + this.secondMessage + '</div>';
	for (var m = 0; m < this.newMessages.length; m++) {
	    html += Mojo.View.render({object: {title: 'v' + this.newMessages[m].version}, template: 'startup/changeLog'});
	    html += '<ul>';
	    for (var l = 0; l < this.newMessages[m].log.length; l++) {
		html += '<li>' + this.newMessages[m].log[l] + '</li>';
	    }
	    html += '</ul>';
	}
    }
    
    // set data
    this.dataContainer.innerHTML = html;
	
	
    // setup menu
    this.controller.setupWidget(Mojo.Menu.appMenu, { omitDefaultItems: true }, this.menuModel);
	
    // set command menu
    this.controller.setupWidget(Mojo.Menu.commandMenu, { menuClass: 'no-fade' }, this.cmdMenuModel);
	
    // set this scene's default transition
    this.controller.setDefaultTransition(Mojo.Transition.zoomFade);
};

StartupAssistant.prototype.activate = function(event)
{
    // start continue button timer
    this.timer = this.controller.window.setTimeout(this.showContinue.bind(this), 5 * 1000);
};

StartupAssistant.prototype.showContinue = function()
{
    // show the command menu
    this.controller.setMenuVisible(Mojo.Menu.commandMenu, true);
};

StartupAssistant.prototype.handleCommand = function(event)
{
    if (event.type == Mojo.Event.command) {
	switch (event.command) {
	case 'do-continue':
	this.controller.stageController.swapScene({name: 'settings', transition: Mojo.Transition.crossFade});
	break;
			
  /*
	case 'do-prefs':
	this.controller.stageController.pushScene('preferences');
	break;
			
	case 'do-help':
	this.controller.stageController.pushScene('help');
	break;
  */
	}
    }
}
