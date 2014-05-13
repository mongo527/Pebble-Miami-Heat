/**********************************
    *** Created by: Mongo527 ***
**********************************/

var BASE_URL = "https://erikberg.com/";

Pebble.addEventListener("ready", function(e) {
    console.log(e.type);
});

/*Pebble.addEventListener("showConfiguration", function() {
    console.log("Showing Configuration");
    config = localStorage.getItem("pebble-miami-heat-config");
    Pebble.openURL('http://shawnconroyd.com/pebble/mh-config-v10.html?' + encodeURIComponent(config));
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("Configuration Closed");
    // webview closed
    options = JSON.parse(decodeURIComponent(e.response));
    console.log("Options = " + JSON.stringify(options));
    localStorage.setItem("pebble-miami-heat-config", JSON.stringify(options));
});*/

function sendMessage(json) {
    
    Pebble.sendAppMessage(json, function(e) {
                                    console.log("Successfully delivered message with transactionId=" + e.data.transactionId);
                                },
                                function(e) {
                                    console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message);
                                }
    );
    
}

function fetchEvents(eventDate) {
    var response;
    var req = new XMLHttpRequest();
    
    var eventID = "";
    var eventStatus;
    var homeTeam;
    var awayTeam;
    var gameTime;
    var gameHour;
    var gameMin;
    
    req.open('GET', BASE_URL + "events.json?sport=nba&date=" + eventDate, true);
    req.setRequestHeader("Authorization", "Bearer 08f07b28-0865-415d-bdfe-5d8f95f3f989");
    req.setRequestHeader("Accept-Encoding", "gzip");
    
    req.onload = function(e) {
        if (req.readyState == 4 && req.status == 200) {
            response = JSON.parse(req.responseText);
            
            for(var index in response.event) {
                if(response.event[index].home_team.full_name == "Miami Heat" || response.event[index].away_team.full_name == "Miami Heat") {
                    eventID = response.event[index].event_id;
                    homeTeam = response.event[index].home_team.abbreviation;
                    awayTeam = response.event[index].away_team.abbreviation;
                    eventStatus = response.event[index].event_status;
                    gameHour = (response.event[index].start_date_time).match(/^(\d+\-\d+\-\d+T)(\d\d)(:\d\d)/)[2];
                    gameMin = (response.event[index].start_date_time).match(/^(\d+\-\d+\-\d+T)(\d\d:)(\d\d)/)[3];
                    //gameTime = (response.event[index].start_date_time).match(/^(\d+\-\d+\-\d+T)(\d\d:\d\d)/)[2];
                    break;
                }
            }
            gameHour = parseInt(gameHour) % 12;
            gameTime = gameHour + ":" + gameMin;
            
            if(eventID === "") {
                sendMessage({"GAME_TIME": "No Game Today"});
            }
            else if(eventStatus != "completed") {
                sendMessage({"HOME_TEAM": homeTeam, "AWAY_TEAM": awayTeam, "GAME_TIME": gameTime});   
            }
            else if(eventStatus == "completed") {
                fetchScores(eventID, homeTeam, awayTeam);
            }
        }
        else {
            console.log("Error: " + req.status.toString());
        }
    };
    req.send(null);
}

function fetchScores(eventID, homeTeam, awayTeam) {
    var response;
    var req = new XMLHttpRequest();
    
    var homeScore;
    var awayScore;
    
    req.open('GET', BASE_URL + "nba/boxscore/" + eventID + ".json", true);
    req.setRequestHeader("Authorization", "Bearer 08f07b28-0865-415d-bdfe-5d8f95f3f989");
    req.setRequestHeader("Accept-Encoding", "gzip");
    
    req.onload = function(e) {
        if (req.readyState == 4 && req.status == 200) {
            response = JSON.parse(req.responseText);
            
            homeScore = response.home_totals.points;
            awayScore = response.away_totals.points;
            
            sendMessage({"HOME_TEAM": homeTeam, "AWAY_TEAM": awayTeam, "HOME_SCORE": homeScore, "AWAY_SCORE": awayScore});
        }
        else {
            console.log("Error: " + req.status.toString());
        }
    };
    req.send(null);
}

Pebble.addEventListener("appmessage", function(e) {
    
    if(e.payload.GET_EVENTS) {
        fetchEvents(e.payload.GET_EVENTS);
    }
});