# -*- coding: utf-8 -*-

import codecs
import chardet
import os

source_dir = '../../src'

def main():
	
	ignore_files = ['makefile', 'makefile.in', 'cmakelists.txt']
	
	for dirpath, _dirnames, filenames in os.walk(source_dir):
		for filename in filenames:
			if filename.lower() in ignore_files:
				continue
			fullpath = os.path.normpath('%s/%s' % (dirpath, filename))
			origin_charset = get_charset(fullpath)
			convert(fullpath, origin_charset, 'UTF-8-SIG')

def get_charset(filepath):
	with open(filepath, 'rb') as f:
		return chardet.detect(f.read())['encoding']
	
def convert(filepath, from_code, to_code):
	from_code = from_code.upper()
	to_code = to_code.upper()
	
	try:
		print('Convert {filename} ... From {origin_charset} to {to_charset}'.format(
			filename=os.path.relpath(os.path.abspath(filepath), source_dir),
			origin_charset=from_code,
			to_charset=to_code
		))
		
		origin_file = codecs.open(filepath, 'r', from_code)
		content = origin_file.read()
		origin_file.close()
		
		target_file = codecs.open(filepath, 'w', to_code)
		target_file.write(content)
		target_file.close()
	except IOError as err:
		print("I/O error: {0}".format(err))

if __name__ == "__main__":
	main()
