var LibraryQ2Websocket = {
	  $Q2WS__deps: ['$Browser']
	, $Q2WS: {
		  sockets: []
		, newSocketId: 1
		, preparedSocket: null
		, q2wsMessageCallback: null
		, q2wsSocketStatusCallback: null

		, onOpenFunc: function() {
			console.log("websocket onOpen");
			if (this.socketId != null) {
				Q2WS.q2wsSocketStatusCallback(this.socketId, this.readyState);
			}
		}

		, onCloseFunc: function() {
			console.log("websocket onClose");
			if (this.socketId != null) {
				Q2WS.q2wsSocketStatusCallback(this.socketId, this.readyState);
			}
		}

		, onMessageFunc: function(msg) {
			var buf = new Uint8Array(msg.data);
			Q2WS.q2wsMessageCallback(this.socketId, buf, buf.length);
		}

		, createSocket: function(url) {
			var socket = new WebSocket(url, "quake2");

			socket.onopen = Q2WS.onOpenFunc;
			socket.onmessage = Q2WS.onMessageFunc;
			socket.onclose = Q2WS.onCloseFunc;
			socket.binaryType = 'arraybuffer';
			socket.url = url;

			return socket;
		}
	}


	, q2wsInit: function() {
		console.log("q2wsInit");
		Q2WS.q2wsMessageCallback = Module.cwrap('q2wsMessageCallback', null, ['number', 'array', 'number']);
		Q2WS.q2wsSocketStatusCallback = Module.cwrap('q2wsSocketStatusCallback', null, ['number', 'number']);
	}


	, q2wsConnect: function(url_) {
		var url = Pointer_stringify(url_);
		console.log("q2wsConnect " + url);

		// are we connecting to the pre-established server?
		if (Q2WS.preparedSocket != null && Q2WS.preparedSocket.url == url) {
			console.log("Using pre-established socket");
			var socket = Q2WS.preparedSocket;
			Q2WS.preparedSocket = null;
		} else {
			try {
				var socket = Q2WS.createSocket(url);
			} catch (err) {
				console.log("Error connecting to " + url + " : " + err);
				return -1;
			}
		}

		var socketId = Q2WS.newSocketId;
		Q2WS.newSocketId++;
		Q2WS.sockets[socketId] = socket;
		socket.socketId = socketId;

		return socketId;
	}


	, q2wsGetReadyState: function(socketId) {
		assert(socketId > 0);
		assert(socketId < Q2WS.sockets.length);
		var socket = Q2WS.sockets[socketId];
		assert(socket != null);
		return socket.readyState;
	}


	, q2wsClose: function(socketId) {
		assert(socketId > 0);
		assert(socketId < Q2WS.sockets.length);
		var socket = Q2WS.sockets[socketId];
		assert(socket != null);
		socket.close();
		Q2WS.sockets[socketId] = null;
	}


	, q2wsPrepSocket: function(url_) {
		var url = Pointer_stringify(url_);
		console.log("q2wsPrepSocket " + url);

		try {
			var socket = Q2WS.createSocket(url);
			Q2WS.preparedSocket = socket;
		} catch (err) {
			console.log("Error connecting to " + url + " : " + err);
		}
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
			return -1;
		}

		return 0;
	}
};


autoAddDeps(LibraryQ2Websocket, '$Q2WS');
mergeInto(LibraryManager.library, LibraryQ2Websocket);
