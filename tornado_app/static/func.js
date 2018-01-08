//utilities

var ws;
var localhost = location.host; //includes also port #

function onLoad() {			
	ws = new WebSocket("ws://"+localhost+"/websocket");
	
	ws.onmessage = function(e) {
		obj = JSON.parse(e.data);
		document.getElementsByClassName('datetime')[0].innerHTML = obj.datetime;
		document.getElementsByClassName('datetime')[1].innerHTML = obj.datetime;
		document.getElementById('probe1').innerHTML = obj.EC1;
		document.getElementById('probe2').innerHTML = obj.EC2;
		document.getElementById('probe3').innerHTML = obj.EC3;
		document.getElementById('probe4').innerHTML = obj.EC4;
		
		
		document.getElementsByClassName('logger')[0].innerHTML = obj.logging_flag + ' @ ' + obj.log_int + ' sec';
		document.getElementsByClassName('logger')[1].innerHTML = obj.logging_flag + ' @ ' + obj.log_int + ' sec';
	};
}

function syncClock(){
	var d = new Date();
	var millis = d.getTime();
	var json = '{"cmd":"sync_clock", "value":"' + millis/1000 + '"}';
	ws.send(json);
}

function toggleLogging(i){
	var json = '{"cmd":"log_flag", "value":"' + i + '"}';
	ws.send(json);
}

function sendLogInt(){
	var logInt = document.getElementById('log_interval').value;
	var json = '{"cmd":"log_int", "value":"' + logInt + '"}';
	ws.send(json);
}