import requests
import json

data_labels = []
list = []
values = []
labels = []

collection = []
document = {}

keys = []
setoutput = 0
i = 0

# opens the interface file
file = open("data.csv", "r")

#reads the last line
lines = file.readlines()

for line in lines:
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
        collection.append(document)
        document = {}





print(collection)

file.close()

#url = "https://mndsphrstdntcntst11-cc3f.restdb.io/rest/wj-monitor"

# payload = json.dumps( {"temperature": tmp,"pressure": p} )
# headers = {
#     'content-type': "application/json",
#     'x-apikey': "40d3cebafe9b9df70cfa01170a3ad8e1189ab",
#     'cache-control': "no-cache"
#     }
#
# response = requests.request("POST", url, data=payload, headers=headers)
#
# print(response.text)
