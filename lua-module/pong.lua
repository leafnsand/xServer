ping = nil
xServer.env_set("pong", xServer.id)
xServer.message_callback = function(from, message)
	print(xServer.timer_now(), message)
	if ping == nil then
		ping = xServer.env_get_value("ping");
	end
	if ping ~= nil then
		xServer.server_message(xServer.id, from, "pong")
	end
end