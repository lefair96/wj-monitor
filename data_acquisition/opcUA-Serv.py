import sys
sys.path.insert(0, "..")
import time
import math
import serial
import serial.tools.list_ports

document = {}

keys = []
setoutput = 0
i = 0

arduino_port = "/dev/ttyACM0" #serial port of USB devices
arduino_id = "usb-Arduino__www.arduino.cc__0043_75932313738351B080C0-if00"
arduino_port = "/dev/serial/by-id/"+arduino_id # the virtual path to Arduino
baud = 115200 #arduino uno runs at 9600 baud


ser = serial.Serial(arduino_port, baud)
print("Connected to Arduino port:" + arduino_port)

samples = 720
lines = 0
ser.flushInput()


from opcua import ua, Server
from opcua.server.history_sql import HistorySQLite


if __name__ == "__main__":

    # setup our server
    server = Server()
    server.set_endpoint("opc.tcp://192.168.8.204:4840/freeopcua/server/")

    # setup our own namespace, not really necessary but should as spec
    #
    uri = "http://examples.freeopcua.github.io"
    idx = server.register_namespace(uri)
    # get Objects node, this is where we should put our custom stuff
    #
    objects = server.get_objects_node()
    # populating our address space
    #
    wjMonitor = objects.add_object(idx, "wj-monitor")
    temperature = wjMonitor.add_variable(idx, "temperature", ua.Variant(0, ua.VariantType.Double))
    pressure = wjMonitor.add_variable(idx, "pressure", ua.Variant(0, ua.VariantType.Double))
    # starting!
    server.start()
    try:
		count = 0
		while True:
			ser_bytes = ser.readline()
			#changes comma decimal separator format
			if len(ser_bytes) > 0:
				line = ser_bytes.replace("\r","").replace("\n","").replace("\xc2\xb0","*").decode("utf-8")
				seq = line.split(";")
				if "<<" in line and setoutput == 0:
					setoutput = 1
					keys = seq[1:]
				elif ">>" in line and setoutput == 1:
					setoutput = 0
					i = 0
					for key in keys:
						i = i + 1
						document[key] = float(seq[i])
					print(document)
					temperature.set_value(document["T:*C"])
					pressure.set_value(document["P:kPa"])
					document = {}


    finally:
        # close connection, remove subscriptions, etc
        server.stop()
