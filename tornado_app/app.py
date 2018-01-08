#!/usr/bin/python

import tornado.httpserver
import tornado.ioloop
import tornado.web
import tornado.websocket

import time
import multiprocessing
import sys
import json

sys.path.insert(0, '/usr/lib/python2.7/bridge/')
from bridgeclient import BridgeClient as bridgeclient

bc = bridgeclient()

connections = []
static_path = '/mnt/sda1/arduino/app/static'

class IndexHandler(tornado.web.RequestHandler):
	def get(self):
		self.render('index.html')

class WebSocketHandler(tornado.websocket.WebSocketHandler):
	def open(self):
		print 'new connection'
		if self not in connections:
			connections.append(self)
	
	def on_message(self, message):
		msg = json.loads(message)
		bc.put(msg['cmd'], msg['value'])
		print msg['cmd'] + '/' + msg['value']
	
	def on_close(self):
		print 'connection closed'
		connections.remove(self)
		
###### MAIN #####

def main():
	app = tornado.web.Application(
		handlers = [
			(r"/", IndexHandler),
			(r"/websocket", WebSocketHandler),
			(r"/static/(.*)", tornado.web.StaticFileHandler, {'path': static_path})
		]
	)
	
	httpServer = tornado.httpserver.HTTPServer(app)
	httpServer.listen(8080)
	
	def bridge_data():
		data = bc.get('data')
		for con in connections:
			con.write_message(data)
	
	mainLoop = tornado.ioloop.IOLoop.instance()
	scheduler = tornado.ioloop.PeriodicCallback(bridge_data, 10, io_loop = mainLoop)
	scheduler.start()
	mainLoop.start()

if __name__ == "__main__":
	main()