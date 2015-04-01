pong = nil
xServer.env_set("ping", xServer.id)
xServer.timer_callback = function(now)
		pong = xServer.env_get_value("pong");
		if pong == nil then 
			xServer.timer_schedule(xServer.id)
		else
			xServer.server_message(xServer.id, pong, "ping")
		end
	end
xServer.message_callback = function(from, message)
	print(xServer.timer_now(), message)
	if pong ~= nil then
		xServer.server_message(xServer.id, from, "ping")
	end
end
pong = xServer.env_get_value("pong");
if pong == nil then
	xServer.timer_schedule(xServer.id)
else
	xServer.server_message(xServer.id, pong, "ping")
end
