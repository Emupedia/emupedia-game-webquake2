var LibraryQ2Websocket = {
	  $Q2WS__deps: ['$Browser']
	, $Q2WS: {
		  sockets: []
		, newSocketId: 1
	}


	, q2wsInit: function() {
		console.log("q2wsInit");
	}


	, q2wsConnect: function(url_) {
		var url = Pointer_stringify(url_);
		console.log("q2wsConnect " + url);

		try {
			var socket = new WebSocket(url, "quake2");
		} catch (err) {
			console.log("Error connecting to " + url + " : " + err);
			return -1;
		}

		var socketId = Q2WS.newSocketId;
		Q2WS.newSocketId++;
		Q2WS.sockets[socketId] = socket;

		socket.socketId = socketId;
		socket.onopen = function () { console.log("websocket onopen");  };
		socket.onmessage = function () { console.log("websocket onmessage");  };
		socket.onclose = function () { console.log("websocket onclose");  };

		return socketId;
	}
};


autoAddDeps(LibraryQ2Websocket, '$Q2WS');
mergeInto(LibraryManager.library, LibraryQ2Websocket);
