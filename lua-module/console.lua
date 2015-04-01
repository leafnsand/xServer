xServer.timer_callback = function(now)
	assert(load(io.read()))()
	xServer.timer_schedule(xServer.id)
end
xServer.timer_schedule(xServer.id)