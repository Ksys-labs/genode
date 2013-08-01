#!/usr/bin/python2

import sys
import xml.etree.ElementTree as ET
import commands

def usage():
	print sys.argv[0], " binary_name run_dir"
	sys.exit(1)


def main():
	print sys.argv[0], sys.argv[1], sys.argv[2]

	if len(sys.argv) != 3:
		usage()
	
	binary_name = sys.argv[1]
	run_dir = sys.argv[2]
	config = run_dir + "/genode/config"
	
	tree = ET.parse(config)
	root = tree.getroot()
	
	name = ""
	
	for start in root.findall('start'):
		binary = start.find('binary')
		if binary is not None:
			bname = binary.get('name')
		else:
			bname = start.get('name')
		
		if (binary_name == bname):
			name = bname
			break
			
	print "name: ", name
	
	if name != "":
		print "binary found:", name
		
		sign = start.find('signature')
		
		print "signature: ", sign.get('value'), " file:", run_dir + "/genode/" + binary_name
		ret = commands.getstatusoutput('bin/signfile '+run_dir+'/genode/'+name+' bin/private.key')
		print ret
		if ret[0] == 0:
			if sign is None:
				sign = ET.Element("signature", value=ret[1])
				start.append(sign)
			else:
				sign.set('value', ret[1])
			tree.write(config)
		else:
			print "signing error"
			sys.exit(1)
		
if __name__ == "__main__":
	main()
