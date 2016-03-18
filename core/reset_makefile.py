#!/usr/bin/python
from os import listdir
from os.path import isdir, join
from subprocess import call
import sys
import uuid
def process(dirpath):
	print(dirpath)
	if dirpath.endswith(".dir"):
		tmpfile = ""
		with open("%s/link.txt" % dirpath) as link_file:
			fields = link_file.read()[:-1].split(" ")
			not_link_flags = filter(lambda f: not f.startswith("-l"), fields)
			link_flags = filter(lambda f: f.startswith("-l"), fields)
			new_link_txt = " ".join(not_link_flags) + " " + " ".join(link_flags)
			tmpfile  = dirpath + "/" + str(uuid.uuid4())
			f = open(tmpfile, "w")
			f.write("%s\n" % new_link_txt)
			f.close()
		if not tmpfile == "":
			call(["mv", tmpfile, "%s/link.txt" % dirpath])
	else:
		for d in listdir(dirpath):
			subpath = join(dirpath, d)
			if isdir(subpath):
				process(subpath)

def main():
	if len(sys.argv) == 2:
		process(sys.argv[1])

if __name__ == "__main__":
	main()
