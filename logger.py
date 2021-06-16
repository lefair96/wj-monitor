import serial
import serial.tools.list_ports
import requests
import json

document = {}

keys = []
setoutput = 0
i = 0


ports = list(serial.tools.list_ports.comports())
for p in ports:
    print(p)
    if "Dispositivo seriale USB" in p.description:
        arduino_port = p.name

#arduino_port = "COM3" #serial port of Arduino
baud = 115200 #arduino uno runs at 9600 baud


ser = serial.Serial(arduino_port, baud)
print("Connected to Arduino port:" + arduino_port)

samples = 720
lines = 0
ser.flushInput()

while lines < samples*2:

    ser_bytes = ser.readline()

    #changes comma decimal separator format
    if len(ser_bytes) > 0:
        decoded_bytes = ser_bytes[0:len(ser_bytes)-2].decode("utf-8")

        line = decoded_bytes
        seq = line.replace(",",".").replace("\n","").split(";")
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
            url = "https://mndsphrstdntcntst11-cc3f.restdb.io/rest/wj-monitor"

            payload = json.dumps(document)
            headers = {
                'content-type': "application/json",
                'x-apikey': "40d3cebafe9b9df70cfa01170a3ad8e1189ab",
                'cache-control': "no-cache"
                }

            response = requests.request("POST", url, data=payload, headers=headers)

            print(response.text)
            document = {}

        lines = lines + 1
