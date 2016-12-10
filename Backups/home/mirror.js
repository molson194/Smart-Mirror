$(document).ready(function() {

    window.setInterval(showDate(), 1000); // update time every second
    window.setInterval(showTemp(), 1000*60); // update temperature every minute
    document.onkeyup = KeyCheck;

    initCalendar();
    initWeather();

    //TODO get params
    var app = getParameterByName("app", window.location.href);
    //var app = 1;
    if (!app) app = 1; //default to calendar
    currentAppIndex = app-1;
    appFunctions[currentAppIndex]();

    var user = getParameterByName("user", window.location.href);
    //if(!user) switchToUser(anonymous);
    switchToUser(user);
    console.log('test');

});



// Your Client ID can be retrieved from your project in the Google Developer Console, https://console.developers.google.com
var CLIENT_ID = '593428568477-kfk8jggviu6h69l3c19gmn6pcl5e8792.apps.googleusercontent.com';
var CLIENT_SECRET = 'ivf-51KXZ108Zmo7AOhISxRb';
var player;

var jeremy = "1";
var saeed = "2";
var matthew = "3";
var dukeball = "4";
var refresh_tokens = {
  "1": "1/AENYqu4C-mfy7IcHBMqdj1ZuOyV4p4V9j986HQEb5lE",
  "4": "1/0oU9XNdTpQNGJpiHm7GXddHR9yCP_7Yp7FzSjqc6O54",
  "3": "1/FUHoBAYNyK0zkOXxG2q8hV-o5TCCx5keY-Ez0ztWs9k",
  "2": "1/YlfEGl6iHnJ3mqa84vaO3ij8ugux8yiYR0MlwPSn2P27QbjYypn-VYPPkkIGDqjM"
}

//todo maybe store access_tokens in local storage somewhere. doesn't really matter tho unless there's a limit on refreshing (keeping it as is will just result in an extra failed call using an old access token everytime the browser is reopened)
var access_tokens = {
  "1" : "ya29.GlugA0_DnCr4HHFNXMkG_s2t47IpQvF5YLwNpD3adV6bGM8lfnRP7fTDTHo6bSTsaDQkWk4aDysRtXIVIWoZp7mIgHkp0IQYiTkVyef-upoMxiFluuBjw_Qc9gBR",
  "4" : "ya29.Ci-gAyJ60pWzoMVvh56V1TMezs4UKflwznoKNjCVvccW4piEF_85c7Ga6d94IvczUA",
  "3" : "ya29.Ci-lA3RFvcahBluRI3mgWC3IbayDjh7bM9a-M_QD9BT2eWrJEhqQ53aKfGC-_6Xf2A",
  "2" : "ya29.Ci-lA97hxELYARY-Xb6ogSpoPYkEYn-8cVQUtdtXMJBhL4-69sDFnrtZvijTf_Kjcg"
};

var users = [jeremy, saeed, matthew, dukeball];
var appFunctions = [showCalendar, showWeather, showYoutube];
var currentAppIndex = 0;


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

function initCalendar() {

}

function initWeather() {
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

function changeYoutube() {

}

function showDiv(id) {
  var div = document.getElementById(id);
  div.style.display = 'block';
  if(id=='youtube') {
    //player.playVideo();
  }
}

function hideDiv(id) {
  var div = document.getElementById(id);
  div.style.display = 'none';
  if(id=='youtube') {
    //player.pauseVideo();
  }
}

function showCalendar() {
  console.log("calendar");
  hideDiv('weather');
  hideDiv('youtube');
  showDiv('calendar');

}

function showWeather() {
  console.log("weather");
  hideDiv('calendar');
  hideDiv('youtube');
  showDiv('weather');

}

function showYoutube() {
  console.log("youtube");
  hideDiv('weather');
  hideDiv('calendar');
  showDiv('youtube');
}


function KeyCheck(e) {
  var KeyID = (window.event) ? event.keyCode : e.keyCode;
	switch(KeyID) {
    case 37:
      currentAppIndex = currentAppIndex - 1;
      if (currentAppIndex == -1) currentAppIndex = 2;
      appFunctions[currentAppIndex]();
      break;
    case 39:
      currentAppIndex = (currentAppIndex + 1) % appFunctions.length;
      appFunctions[currentAppIndex]();
		  break;
    case 38: // up
      console.log('Up was pressed. Chaning to next user.\n');
      changeToNextUser();
      break;
    case 40: // down
      console.log('Down was pressed. Changing to previous user.\n');
      changeToPreviousUser();
      break;
	case 67: //c
	  currentAppIndex = 0;
	  appFunctions[currentAppIndex]();
	  console.log('calendar');
	  break;
	case 77: //m
	  currentAppIndex = 3;
	  //appFunctions[currentAppIndex]();
	  break;
	case 87: //w
	  currentAppIndex = 1;
	  appFunctions[currentAppIndex]();
	  break;
	case 89: // y
	  currentAppIndex = 2;
	  appFunctions[currentAppIndex]();
	  break;
	}
	
	
}

function getParameterByName(name, url) {
  if (!url) {
    url = window.location.href;
  }
  name = name.replace(/[\[\]]/g, "\\$&");
  var regex = new RegExp("[?&]" + name + "(=([^&#]*)|&|#|$)"),
      results = regex.exec(url);
  if (!results) return null;
  if (!results[2]) return '';
  return decodeURIComponent(results[2].replace(/\+/g, " "));
}

function switchToUser(user) {
  //if(user in users) { //todo figure out syntax
    current_user = user;
    var userDiv = document.getElementById('user');
    userDiv.innerHTML = current_user + "'s calendar";
    loadCalendar();

  //}
}

function switchToUserWithIndex(index) {
  switchToUser(users[index]);
}

function switchToUserWithPassword(password) {
  if(password in passwords) {
    switchToUser(passwords[password]);
  }
}

function getCurrentUserIndex() {
  for (var i = 0; i < users.length; i++) {
    if(users[i]==current_user){
      return i;
    }
  }
}
function changeToPreviousUser() {
  var currentUserIndex = getCurrentUserIndex();
  var newUserIndex = currentUserIndex == 0 ? users.length - 1 : currentUserIndex - 1;
  switchToUserWithIndex(newUserIndex);
}

function changeToNextUser() {
  var currentUserIndex = getCurrentUserIndex();
  var newUserIndex = currentUserIndex == users.length - 1  ? 0 : currentUserIndex + 1;
  switchToUserWithIndex(newUserIndex);
}
function loadCalendar(){
  console.log("loading calendar for user: " + current_user + ".\n");
  // Retrieve today's events for current user
  var access_token = access_tokens[current_user];
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
    var refresh_token = refresh_tokens[current_user];
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
      access_tokens[current_user] = access_token;
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
  var eventInnerHTML = "<div style='width:60%; height: 15%; margin: auto; margin-top: 10px; border-radius: 25px; border: 2px solid #FFFFFF; padding: 20px; font-size:25px'><div style='width:45%; height:100%; float:left; padding-top:10px; padding-bottom:10px;'>"+shortDescription+"</div><div style='width:45%; height:100%; float:right; text-align:right; padding-top:10px; padding-bottom:10px;'>"+startDate+"</div></div>";
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

// autoplay video
function onPlayerReady(event) {
    //event.target.playVideo();
    //event.target.pauseVideo();
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
