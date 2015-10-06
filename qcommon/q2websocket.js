var LibraryQ2Websocket = {
	  $Q2WS__deps: ['$Browser']
	, $Q2WS: {
		  sockets: []
		, newSocketId: 1

		, onOpenFunc: function() {
			console.log("websocket onOpen");
		}

		, onCloseFunc: function() {
			console.log("websocket onClose");
		}

		, onMessageFunc: function(msg) {
			var buf = new Uint8Array(msg.data);
			// TODO: this doesn't change, store it during init
			var q2wsMessageCallback = Module.cwrap('q2wsMessageCallback', null, ['number', 'array', 'number']);
			q2wsMessageCallback(this.socketId, buf, buf.length);
		}
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
		socket.onopen = Q2WS.onOpenFunc;
		socket.onmessage = Q2WS.onMessageFunc;
		socket.onclose = Q2WS.onCloseFunc;
		socket.binaryType = 'arraybuffer';

		return socketId;
	}


	, q2wsSend: function(socketId, buf, length) {
		var socket = Q2WS.sockets[socketId];

		var binary = new Uint8Array(length);
		for (var i = 0; i < length; i++) {
			binary[i] = {{{ makeGetValue('buf', 'i', 'i8') }}};
		}

		try {
			socket.send(binary.buffer);
		} catch (err) {
			console.log("Error in webSocket.send: " + err);
		}
	}
};


autoAddDeps(LibraryQ2Websocket, '$Q2WS');
mergeInto(LibraryManager.library, LibraryQ2Websocket);
