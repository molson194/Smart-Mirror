$(document).ready(function() {

    window.setInterval(showDate(), 1000); // update time every second
    window.setInterval(showTemp(), 1000*60); // update temperature every minute
    document.onkeyup = KeyCheck;

    handleUserParam();
    handleAppParam();

    window.setInterval(loadCalendar(), 1000*60*60); // update calendar every hour
    window.setInterval(loadWeather(), 1000*60*60); // update weather every hour
    //initYoutube(); ?

});



// Your Client ID can be retrieved from your project in the Google Developer Console, https://console.developers.google.com
var CLIENT_ID = '593428568477-kfk8jggviu6h69l3c19gmn6pcl5e8792.apps.googleusercontent.com';
var CLIENT_SECRET = 'ivf-51KXZ108Zmo7AOhISxRb';

// Users
var guestId = "0";
var guest = "Guest";
var jeremy = "Jeremy";
var saeed = "Saeed";
var matthew = "Matthew";
var dukeball = "Duke Club Ball";
var users = {
  "0" : {
    "name" : guest
  },
  "1" : {
    "name" : jeremy,
    "access_token" : "ya29.GlugA0_DnCr4HHFNXMkG_s2t47IpQvF5YLwNpD3adV6bGM8lfnRP7fTDTHo6bSTsaDQkWk4aDysRtXIVIWoZp7mIgHkp0IQYiTkVyef-upoMxiFluuBjw_Qc9gBR",
    "refresh_token" :  "1/AENYqu4C-mfy7IcHBMqdj1ZuOyV4p4V9j986HQEb5lE",
  },
  "2" : {
    "name" : saeed,
    "access_token" : "ya29.Ci-lA97hxELYARY-Xb6ogSpoPYkEYn-8cVQUtdtXMJBhL4-69sDFnrtZvijTf_Kjcg",
    "refresh_token" : "1/YlfEGl6iHnJ3mqa84vaO3ij8ugux8yiYR0MlwPSn2P27QbjYypn-VYPPkkIGDqjM",
  },
  "3" : {
    "name" : matthew,
    "access_token" : "ya29.Ci-lA3RFvcahBluRI3mgWC3IbayDjh7bM9a-M_QD9BT2eWrJEhqQ53aKfGC-_6Xf2A",
    "refresh_token" : "1/FUHoBAYNyK0zkOXxG2q8hV-o5TCCx5keY-Ez0ztWs9k",
  },
  "4" : {
    "name" : dukeball,
    "access_token" : "ya29.Ci-gAyJ60pWzoMVvh56V1TMezs4UKflwznoKNjCVvccW4piEF_85c7Ga6d94IvczUA",
    "refresh_token" : "1/0oU9XNdTpQNGJpiHm7GXddHR9yCP_7Yp7FzSjqc6O54",
  }
}

var current_user = guestId;

// Apps
var currentAppIndex = 0;
var player;


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

function KeyCheck(e) {
  var KeyID = (window.event) ? event.keyCode : e.keyCode;
  switch(KeyID) {
    case 37:
      var appFunctions = getAppFunctions();
      currentAppIndex = currentAppIndex - 1;
      if (currentAppIndex == -1) currentAppIndex = appFunctions.length-1;
      appFunctions[currentAppIndex]();
      break;
    case 39:
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
  }
}

function handleAppParam() {
  var app = getParameterByName("app", window.location.href);
  if(isGuest()) {
    currentAppIndex = 0; // default guest to weather
  } else if (app == "calendar") {
    currentAppIndex = 0;
  } else if (app == "weather") {
    currentAppIndex = 1;
  } else if (app == "youtube") {
    currentAppIndex = 2;
  } else {
    currentAppIndex = 0; // default to calendar
  }

  getAppFunctions()[currentAppIndex]();
}

function handleUserParam() {
  var user = getParameterByName("user", window.location.href);
  if(!user) switchToUser(guestId);
  switchToUser(user);
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

function getAppFunctions() {
  if(isGuest()) {
    return [showWeather, showYoutube];
  } else {
    return [showCalendar, showWeather, showYoutube];
  }
}

function isGuest() {
  return current_user == guestId;
}

function switchToUser(user) {
  console.log("switch to user " + user);
  if (!(user in users) || user == guestId) {
    current_user = guestId;
    clearEvents();
    showWeather();
  } else {
    current_user = user;
    showCalendar();
  }

  document.getElementById('user').innerHTML = users[current_user].name;
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
  if(isGuest()) {
    showWeather();
  } else {
    currentAppIndex = 0;
    showDiv('calendar');
    hideDiv('weather');
    hideDiv('youtube');
    hideDiv('miroslav');
  }
}

function showWeather() {
  console.log("weather");
  currentAppIndex = isGuest() ? 0 : 1;
  hideDiv('calendar');
  showDiv('weather');
  hideDiv('youtube');
  hideDiv('miroslav');


}

function showYoutube() {
  console.log("youtube");
  currentAppIndex = isGuest() ? 1 : 2;
  hideDiv('weather');
  hideDiv('calendar');
  showDiv('youtube');
  hideDiv('miroslav');

}

function showMiroslav() {
  console.log("miroslav");
  currentAppIndex = isGuest() ? 2 : 3;
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
  if(current_user == guestId) return;
  console.log("loading calendar for user: " + users[current_user].name + ".\n");
  // Retrieve today's events for current user

  var access_token = users[current_user].access_token;
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
    var refresh_token = users[current_user].refresh_token;
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
      users[current_user].access_token = access_token;
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

function changeYoutube() {

}
