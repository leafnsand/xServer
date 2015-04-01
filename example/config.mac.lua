config = {}
config.thread = "4"
config.daemon = "0"
config.root_path = "/Users/xavierjiang/Dropbox/xServer/"
config.module_path = config.root_path .. "build/mac/"
config.lua_path = config.root_path .. "lua-module/"
config.log_path = config.root_path .. "log/"
config.messages = [==[
	xLogger
	xLua socket_pong.lua
	xLua socket_ping.lua
]==]
return config