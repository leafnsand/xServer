xServer.timer_callback = function(now)
	print(now)
	xServer.timer_schedule(xServer.id, "1000")
end
xServer.timer_schedule(xServer.id, "1000")