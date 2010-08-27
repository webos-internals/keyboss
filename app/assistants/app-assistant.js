// get the cookies
var prefs = new preferenceCookie();
var vers =  new versionCookie();

// stage names
var mainStageName = 'settings';

function AppAssistant() {}

AppAssistant.prototype.handleLaunch = function(params) { 
  try {
    var mainStageController = this.controller.getStageController(mainStageName); 
    if (mainStageController) { 
      var scenes = mainStageController.getScenes(); 
      if (scenes[0].sceneName == 'settings') { 
        mainStageController.popScenesTo('settings'); 
      }
  			
      mainStageController.activate(); 
    } 
    else { 
      this.controller.createStageWithCallback({name: mainStageName, lightweight: true}, this.launchFirstScene.bind(this)); 
    } 
  } 
  catch (e) { 
    Mojo.Log.logException(e, "AppAssistant#handleLaunch"); 
  }
};

AppAssistant.prototype.launchFirstScene = function(controller) { 
  vers.init(); 
  if (vers.showStartupScene()) 
    controller.pushScene('startup'); 
  else 
    controller.pushScene('settings');
};

AppAssistant.prototype.cleanup = function() {};
