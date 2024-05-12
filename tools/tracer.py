#!/bin/python3

import matplotlib.animation as animation
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.widgets as widgets
import serial
import threading
import time

class Traced:
	def __init__(self, name: str):
		self.name = name
		self.executing = False
		self.data = []

class Acquirer(threading.Thread):
	def __init__(self, traceds: list[Traced], anim: animation.FuncAnimation) -> None:
		threading.Thread.__init__(self)
		self.traceds = traceds
		self.anim = anim
		self.serial_port = serial.Serial(
			port='/dev/ttyUSB0',
			baudrate=115200,
			parity=serial.PARITY_NONE,
			stopbits=serial.STOPBITS_ONE,
			bytesize=serial.EIGHTBITS,
			timeout=1
		)
		self.stopped = True
		self.done = False
		self.start_time = 0

	def run(self) -> None:
		while not self.done:
			data = self.serial_port.read(2)
			if self.stopped or len(data) < 2:
				continue
			event = int(data[0])
			if event > 1:
				self.serial_port.read(1)
				continue
			index = int(data[1])
			if index > len(traceds):
				self.serial_port.read(1)
				continue
			traced = traceds[index]
			if event == 0:
				traced.executing = True
				if self.start_time == 0:
					self.start_time = time.time()
				traced.data.append((time.time() - self.start_time, 0))
			elif event == 1:
				traced.executing = False

	def pause(self, event=None) -> None:
		self.anim.pause()
		self.stopped = True

	def resume(self, event=None) -> None:
		for traced in self.traceds:
			traced.executing = False
			traced.data.clear()
		self.start_time = 0
		self.anim.resume()
		self.stopped = False


traceds = [Traced(name) for name in ["Idle", "Correr", "Água", "Descansar"]]

fig, ax = plt.subplots()
cmap = plt.get_cmap('tab10').colors
def animate(_):
	ax.clear()
	for (index, traced) in enumerate(traceds):
		if len(traced.data) == 0:
			continue
		if traced.executing:
			enter_time = traced.data[-1][0]
			traced.data[-1] = (enter_time, time.time() - acquirer.start_time - enter_time)
		ax.broken_barh(traced.data, (-index, -1), color=cmap[index])
	ax.set_yticks(range(-(len(traceds) + 1), 0), labels=[""] * (len(traceds) + 1), minor=False)
	ax.set_yticks([-(x + 0.5) for x in range(len(traceds))], labels=[traced.name for traced in traceds], minor=True)
	ax.xaxis.set_major_locator(ticker.MultipleLocator(base=1.0))
	ax.set_ylim([-len(traceds), 0])
	ax.grid(True)
anim = animation.FuncAnimation(fig, animate, interval=16, cache_frame_data=False)

acquirer = Acquirer(traceds, anim)

button_start = widgets.Button(fig.add_axes([0.38, 0.9, 0.1, 0.06]), "Start")
button_start.on_clicked(acquirer.resume)
button_pause = widgets.Button(fig.add_axes([0.52, 0.9, 0.1, 0.06]), "Stop")
button_pause.on_clicked(acquirer.pause)

acquirer.start()
plt.show()

acquirer.done = True
acquirer.join()
