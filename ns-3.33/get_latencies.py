import csv
import sys

filepath = sys.argv[1]
nSta = sys.argv[2]
output = sys.argv[3]

with open(output, 'a', newline='') as file:
	writer = csv.writer(file)
	writer.writerow(["NSta="+nSta])

with open(filepath) as fp:
	 line1 = fp.readline() #Remove the two first lines (wrong values)
	 line2 = fp.readline()
	 line1 = fp.readline()
	 line2 = fp.readline()
	 while line2:
		 if "server received" in line2 and "client sent" in line1:
			 words1 = line1.split()
			 words2 = line2.split()
			 start = float(words1[0].replace('s', '').replace('+', ''))  #Time in logs is in Micro Seconds
			 end = float(words2[0].replace('s', '').replace('+', '')) 
			 with open(output, 'a', newline='') as file:
				 writer = csv.writer(file)
				 writer.writerow([(end-start)/1000])
			 line1 = fp.readline()
			 line2 = fp.readline()
		 else:
			 line1 = line2
			 line2 = fp.readline()