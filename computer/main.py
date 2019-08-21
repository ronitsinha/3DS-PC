#!/usr/bin/python
import sys
import pyautogui
from http.server import BaseHTTPRequestHandler, HTTPServer

SPEED = 100

# TODO: handle touch screen input

def normalize (val, min1, max1, min2, max2):
	span1 = max1 - min1
	span2 = max2 - min2

	scaled = float(val-min1) / float(span1)

	return min2 + (scaled * span2)

def handle_movement (x, y):

	xscale = normalize(x, -156, 156, -1, 1)
	yscale = normalize(y, -156, 156, -1, 1)

	print(xscale, yscale)

	xcomp = 0
	ycomp = 0

	if xscale > 0.1 or xscale < -0.25:
		xcomp = int(xscale*SPEED)

	if yscale > 0.1 or yscale < -0.25:
		ycomp = int(yscale*SPEED)


	pyautogui.moveRel(xcomp, -ycomp)

def handle_click (key, state):
	if key == 'KEY_A':
		if state == 'UP':
			pyautogui.mouseUp()
		elif state == 'HELD':
			pyautogui.mouseDown()
		elif state == 'DOWN':
			pyautogui.click()

# https://gist.github.com/bradmontgomery/2219997
class Handler(BaseHTTPRequestHandler):
	def _set_headers(self):
		self.send_response(200)
		self.send_header('Content-type', 'text/html')
		self.end_headers()

	def do_GET(self):
		self._set_headers()
		self.wfile.write(b'GET!')

	def do_HEAD(self):
		self._set_headers()

	def do_POST(self):
		content_length = int(self.headers['Content-Length']) # Gets the size of data
		post_data = self.rfile.read(content_length) # Gets the data itself
		
		try:
			posx, posy = [int(n) for n in post_data.decode('utf-8').split(' ')]

			# print('Got a POST: ', posx, posy)

			handle_movement(posx, posy)
		except ValueError:
			try:
				key, state = post_data.decode('utf-8').split(' ')

				print('Got a POST: ', key, state)

				handle_click(key, state)
			except UnicodeDecodeError:
				pass

		self._set_headers()
		self.wfile.write(b'POST!')


def run(server_class=HTTPServer, handler_class=Handler, port=8080):
	server_address = ('', port)
	httpd = server_class(server_address, handler_class)
	print (f'Starting server on port {port}...')
	httpd.serve_forever()


if __name__ == '__main__':
	run()
