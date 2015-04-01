socket = xServer.socket_create(xServer.id)
xServer.socket_connect(xServer.id, socket, "127.0.0.1", 12306)
xServer.socket_send(xServer.id, socket, "ping")
xServer.socket_callback = function(from, message)
	if message == nil then
		print("client closed")
		socket = nil
	else
		print(message)
		xServer.socket_send(xServer.id, socket, "ping at " .. xServer.timer_now())
	end
end