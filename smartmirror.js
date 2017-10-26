const PASSWORD_LENGTH = 6;
const WEATHER_INDEX = 0;
const YOUTUBE_INDEX = 1;
const CALENDAR_INDEX = 2;
const MIROSLAV_INDEX = 3;
const NO_USER = "-1";
const GUEST_ID = "0";
const USER1_ID = "1";
const USER2_ID = "2";
const USER3_ID = "3";
const GUEST_NAME = "Guest";
const JEREMY_NAME = "Jeremy";
const SAEED_NAME = "Matthew";
const MATTHEW_NAME = "Saeed";
const GUEST_APP_FUNCTIONS = [showWeather, showYoutube];
const APP_FUNCTIONS = [showWeather, showYoutube, showCalendar];

// Your Client ID can be retrieved from your project in the Google Developer Console, https://console.developers.google.com
var CLIENT_ID = 'CLIENT_ID';
var CLIENT_SECRET = 'CLIENT_SECRET';

// Users

var users = {};
users[GUEST_ID] = {
    "name" : GUEST_NAME
  };
users[USER1_ID] = {
    "name" : JEREMY_NAME,
    "access_token" : "YOUR ACCESS TOKEN HERE",
    "refresh_token" :  "YOUR REFRESH TOKEN HERE",
  };
users[USER2_ID] = {
    "name" : SAEED_NAME,
    "access_token" : "YOUR ACCESS TOKEN HERE",
    "refresh_token" : "YOUR REFRESH TOKEN HERE",
  };
users[USER3_ID] = {
    "name" : MATTHEW_NAME,
    "access_token" : "YOUR ACCESS TOKEN HERE",
    "refresh_token" : "YOUR REFRESH TOKEN HERE",
  };

var currentUserId = NO_USER;

// Apps
var currentAppIndex = 0;
var player;

// Welcome
var numStars = 0;
var isAsleep = false;


// Initialization
$(document).ready(function() {
	document.onkeyup = KeyCheck;
  showDate();
  showTemp();
  window.setInterval(showDate, 1000); // update time every second
  window.setInterval(showTemp, 1000*60); // update temperature every minute

  showWelcomeScreen();
   
});


function KeyCheck(e) {
  var keyID = (window.event) ? event.keyCode : e.keyCode;
	// Global Events
  switch(keyID) {
    case 90: // z (sleep)
      sleep();
      break;
    case 79: // o (on)
      wake();
      break;  
  }

  if(isAsleep) return;
 
  if(isLoggedIn()) {
    // Main App Events
    switch(keyID) {
      case 37: // left
        var appFunctions = getAppFunctions();
        currentAppIndex = currentAppIndex - 1;
        if (currentAppIndex == -1) currentAppIndex = appFunctions.length-1;
        appFunctions[currentAppIndex]();
        break;
      case 39: // right
        var appFunctions = getAppFunctions();
        currentAppIndex = (currentAppIndex + 1) % appFunctions.length;
        appFunctions[currentAppIndex]();
        break;
      case 67: //c
        if(!isGuest()) showCalendar();
        break;
      case 77: //m
        showMiroslav();
        break;
      case 87: //w
        showWeather();
        break;
      case 89: // y
        showYoutube();
        break;
      case 72: // h
		print("playOrPause");
		playOrPauseVideo();
		break;
      case 76: // l (logout, show welcome page)
        logout();
        break;    
    }
  } else {
    // Welcome Events
    switch(keyID) {
      case 80: // p (password)
        addStar();
        break;
      case 82: // r (reset)
        clearStars();
        break;
      case 83: // s (success)
        showPasswordSuccess();
        break;
      case 70: // f (fail)
        clearStars();
        showPasswordFail();
        break;
      case 48: // 0 (guest)
        loginAsUser(GUEST_ID);
        break;
      case 49: // 1 (user 1)
        loginAsUser(USER1_ID);
        break;
      case 50: // 2 (user 2)
        loginAsUser(USER2_ID);
        break;
      case 51: // 3 (user 3)
        loginAsUser(USER3_ID);
        break;
    }
  }
}

///////// General (all working) /////////////
function switchDisplay(show, hide) {
  document.getElementById(hide).style.display = "none";
  document.getElementById(show).style.display = "block";
}


///////// Top Bar (all working) /////////////
function showDate() {
  var date = new Date(Date.now());
  $("#time").html(date.toLocaleTimeString());
  $("#date").html(date.toLocaleDateString());
}

function showTemp() {
  $.getJSON("http://api.openweathermap.org/data/2.5/weather?q=Durham,us&appid=e36c51e09e53c3817faab9a9d69ce41d",function(json){
    $("#temp").html(Math.round(json.main.temp * 9/5 - 459.67) + "&deg;F");
  });
}

function setTitle(title) {
  document.getElementById("title").innerHTML = title;
}


///////// Welcome (all working) /////////////
function showWelcomeScreen() {
  switchDisplay("main", "sleep");
  switchDisplay("welcome", "apps");
  setTitle("Smart Mirror");
  showGreeting();
  clearStars();
}

function showGreeting() {
  var date = new Date(Date.now());
  if (date.getHours() < 12) {
    $("#greeting").html("Good Morning");
  } else if (date.getHours() < 18) {
    $("#greeting").html("Good Afternoon");
  } else if (date.getHours() < 20) {
    $("#greeting").html("Good Evening");
  } else if (date.getHours() >= 20) {
    $("#greeting").html("Good Night");
  } 
}

function addStar() {
  if(numStars < 6) numStars++;
  console.log(numStars);
  showStars();
}

function clearStars() {
  numStars = 0;
  showStars();
}

function showStars() {
  var html = "";
  for(var i = 0; i < PASSWORD_LENGTH; i++) {
    if(i<numStars) {
      html+="* ";
    }else {
      html+= "_ ";
    }
  }
  document.getElementById("password").innerHTML = html;
  document.getElementById("password-status").style.visibility = "hidden";
}

function showPasswordSuccess() {
  var passwordStatus = document.getElementById("password-status");
  passwordStatus.style.visibility = "visible";
  passwordStatus.innerHTML = "Success! Logging in...";
}

function showPasswordFail() {
  var passwordStatus = document.getElementById("password-status");
  passwordStatus.style.visibility = "visible";
  passwordStatus.innerHTML = "Invalid password! Please try again.";
}

function sleep() {
  isAsleep = true;
  player.pauseVideo();
  switchDisplay("sleep", "main");
}

function wake() {
  isAsleep = false;
  switchDisplay("main", "sleep");
  if(currentAppIndex == YOUTUBE_INDEX) player.playVideo();
}

/////// Main Apps /////////
function showMainApps() {
  switchDisplay("main", "sleep");
  switchDisplay("apps", "welcome");
}

function loginAsUser(userId) {
  console.log("Log in as user " + userId);
  loadWeather();
  // TODO: load youtube?
  switchToUser(userId);
  showMainApps();
}

function switchToUser(userId) {
  if (!(userId in users) || userId == GUEST_ID) {
    currentUserId = GUEST_ID;
    clearEvents();
    showWeather();
  } else {
    currentUserId = userId;
    loadCalendar();
    showCalendar();
  }

  console.log(currentUserId);
  document.getElementById("title").innerHTML = getCurrentUser().name;
}

function logout() {
  clearEvents();
  player.pauseVideo();
  currentUserId = NO_USER;
  showWelcomeScreen();
}

function getAppFunctions() {
  if(isGuest()) {
    return GUEST_APP_FUNCTIONS;
  } else {
    return APP_FUNCTIONS;
  }
}

function isLoggedIn() {
  return currentUserId != NO_USER;
}

function isGuest() {
  return currentUserId == GUEST_ID;
}

function getCurrentUser() {
  return users[currentUserId];
}

function showDiv(id) {
  var div = document.getElementById(id);
  div.style.display = 'block';
  if(id=='youtube') {
    player.playVideo();
  }
}

function hideDiv(id) {
  var div = document.getElementById(id);
  div.style.display = 'none';
  if(id=='youtube') {
	  console.log("pause");
    player.pauseVideo();
  }
}

function showCalendar() {
  console.log("calendar");
  if(isGuest()) {
    showWeather();
  } else {
    currentAppIndex = CALENDAR_INDEX;
    showDiv('calendar');
    hideDiv('weather');
    hideDiv('youtube');
    hideDiv('miroslav');
  }
}

function showWeather() {
  console.log("weather");
  currentAppIndex = WEATHER_INDEX;
  hideDiv('calendar');
  showDiv('weather');
  hideDiv('youtube');
  hideDiv('miroslav');
}

function showYoutube() {
  console.log("youtube");
  currentAppIndex = YOUTUBE_INDEX;
  hideDiv('weather');
  hideDiv('calendar');
  showDiv('youtube');
  hideDiv('miroslav');
}

function showMiroslav() {
  console.log("miroslav");
  currentAppIndex = MIROSLAV_INDEX;
  hideDiv('weather');
  hideDiv('calendar');
  hideDiv('youtube');
  showDiv('miroslav');
}

function loadWeather() {
  $.getJSON("http://api.openweathermap.org/data/2.5/forecast?q=Durham,us&mode=json&appid=e36c51e09e53c3817faab9a9d69ce41d",function(json){
    console.log(json);
    for (var i = 0; i < 6; i++) {
      var date = new Date(json.list[i].dt*1000);
      console.log(date);
      var time = ((date.getUTCHours()-5 > 0) ? date.getUTCHours()-5 : date.getUTCHours()+7) % 12;
      $("#weather" + i).html("<td>" + time + ":00" + "</td><td>" + json.list[i].weather[0].main + "</td><td>" + Math.round(json.list[i].main.temp * 9/5 - 459.67) + "&deg;F</td><td>" + json.list[i].main.humidity + "%</td><td>" + json.list[i].wind.speed + "</td>");
    }
  });
}

function loadCalendar(){
  if(isGuest()) return;
  console.log("loading calendar for user: " + getCurrentUser().name + ".\n");
  // Retrieve today's events for current user

  var access_token = getCurrentUser().access_token;
  var calendarParams = getCalendarRequestParams(access_token);
  var calendarUrl = getCalendarURL(calendarParams);
  var calendarRequest = $.getJSON(calendarUrl);

  calendarRequest.done(function(json) {
    console.log("Successfully retrieved calendar with access token.\n");
    listUpcomingEvents(json);
  });

  calendarRequest.fail(function(json) {
    console.log("Failed to retreive calendar with access token.\nRequesting a new one using refresh token...\n");

    // Request new access token
    var refresh_token = getCurrentUser().refresh_token;
    var tokenURL = "https://www.googleapis.com/oauth2/v4/token";
    var payload = {
      client_id: CLIENT_ID,
      client_secret: CLIENT_SECRET,
      refresh_token: refresh_token,
      grant_type:"refresh_token"
    };
    var accessTokenRequest = $.post(tokenURL, payload);
    accessTokenRequest.done(function(json) {
      console.log("Successfully retrieved a new access token.\n")
      access_token = json.access_token;
      getCurrentUser().access_token = access_token;
      calendarParams = getCalendarRequestParams(access_token);
      calendarUrl = getCalendarURL(calendarParams);

      //note: could have done recursive call. decided not to so it doesn't get into an infinite loop on error
      // we could choose to repeat this a few times in case of a temporary error
      var calendarRequest2 = $.getJSON(calendarUrl);

      calendarRequest2.done(function(json) {
        console.log("Successfully retrieved calendar with new access token.\n");
        listUpcomingEvents(json);
      });

      calendarRequest2.fail(function(json) {
        console.log("Failed to retrieve calendar with new access token.\n");
      });
    });

    accessTokenRequest.fail(function(json) {
      console.log("Failed to retrieve new access token.\n")
    });
  });
}

function getCalendarRequestParams(access_token) {
  var today = new Date();
  var tomorrow = new Date();
  tomorrow.setDate(tomorrow.getDate() + 1);
  var params = {
    'access_token': access_token,
    'calendarId': 'primary',
    'timeMin': today.toISOString(),
    'timeMax' : tomorrow.toISOString(),
    'showDeleted': false,
    'singleEvents': true,
    'maxResults': 10,
    'orderBy': 'startTime'
  }
  return jQuery.param(params);;
}

function getCalendarURL(params) {
  return "https://www.googleapis.com/calendar/v3/calendars/primary/events?" + params;
};

/**
 * Print the summary and start datetime/date of the next 3 events in
 * the authorized user's calendar. If no events are found an
 * appropriate message is printed.
 */
function listUpcomingEvents(resp) {
  clearEvents();
  var events = resp.items;

  if (events.length > 0) {
    for (i = 0; i < Math.min(events.length,3); i++) { //for (i = 0; i < events.length; i++) {
      var event = events[i];
      var when = event.start.dateTime;
      if (!when) {
        when = event.start.date;
      }
      var startDate = new Date(when);
      appendEvent(event.summary, startDate.toLocaleString(), "");

    }
  } else {
    appendEvent('No events in next 24 hours. Have a chiller day!', "", "");
  }
}

/**
 * Append a event element to the body containing the given message
 * as its text node.
 */
function appendEvent(description, startDate, endDate) {
  console.log(description);
  console.log(startDate);
  var maxLength = 50;
  var shortDescription = description.substring(0, maxLength);
  var eventsDiv = document.getElementById('events');
  var eventInnerHTML = "";
  if(startDate) {
	    eventInnerHTML = "<div style='width:90%; margin: auto; margin-top: 10px; border-radius: 25px; border: 2px solid #FFFFFF; padding: 20px; font-size:25px'><div style='width:45%; height:100%; display:inline-block; padding-top:10px; padding-bottom:10px;'>"+shortDescription+"</div><div style='width:45%; display:inline-block; height:100%; float:right; text-align:right; padding-top:10px; padding-bottom:10px;'>"+startDate+"</div></div>";

  } else {
	    eventInnerHTML = "<div style='width:90%; margin: auto; margin-top: 10px; border-radius: 25px; border: 2px solid #FFFFFF; padding: 20px; font-size:25px'><div style='width:90%; height:100%; display:block; padding-top:10px; padding-bottom:10px;'>"+shortDescription+"</div></div>";

  }
  var eventDiv = document.createElement('div');
  eventDiv.innerHTML = eventInnerHTML;
  eventsDiv.appendChild(eventDiv);
}


function clearEvents() {
  var eventsDiv = document.getElementById('events');
  //eventDiv.innerHTML = ''; //slower
  while (eventsDiv.firstChild) {
    eventsDiv.removeChild(eventsDiv.firstChild);
  }
}

// create youtube player

function onYouTubePlayerAPIReady() {
  $.getJSON("https://www.googleapis.com/youtube/v3/search?key=AIzaSyDR_kYAugZpEqrkJshQt-9TIRdclo5PlNY&channelId=UCUHW94eEFW7hkUMVaZz4eDg&part=snippet,id&order=date&maxResults=40",function(json){
    var videoId = json.items[Math.floor(Math.random()*40)].id.videoId;
    player = new YT.Player('player', {
      height: '100%',
      width: '100%',
      videoId: videoId,
      events: {
        'onReady': onPlayerReady,
        'onStateChange': onPlayerStateChange
      }
    });
  });

}

function onPlayerReady(event) {
}


// when video ends
function onPlayerStateChange(event) {
    if(event.data === 0) {
        $.getJSON("https://www.googleapis.com/youtube/v3/search?key=AIzaSyDR_kYAugZpEqrkJshQt-9TIRdclo5PlNY&channelId=UCUHW94eEFW7hkUMVaZz4eDg&part=snippet,id&order=date&maxResults=40",function(json){
          var videoId = json.items[Math.floor(Math.random()*40)].id.videoId;
            player.loadVideoById(videoId);

        });
    }
}

function isPlaying(player) {
	return player.getPlayerState() == 1;
}

function playOrPauseVideo() {
	if(isPlaying(player)) {
		player.pauseVideo();
	} else {
		player.playVideo();
	}
}
