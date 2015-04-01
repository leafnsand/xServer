listen_socket = xServer.socket_create(xServer.id)
client_socket = {}
xServer.socket_listen(xServer.id, listen_socket, "127.0.0.1", 12306, 16)
xServer.socket_callback = function(from, message)
	if message == nil then
		if listen_socket == from then
			print("server closed")
			listen_socket = nil
			return
		end
		if client_socket[from] == nil then
			print("client connected")
		else
			client_socket[from] = nil
		end
	else
		print(message)
		xServer.socket_send(xServer.id, from, "pong at " .. xServer.timer_now())
	end
end