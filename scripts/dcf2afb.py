#!/usr/bin/python3
#
# Copyright (C) 2015-2024 IoT.bzh Company
# SPDX-License-Identifier: 0BSD
#
# This program extract from a DCF file the declarations needed
# by the repesk binding canopen-binding.
#

import sys
import configparser
import argparse
import json
import collections
import copy

########################################################################
## routines for playing with names
########################################################################

# remove any text in parenthesis and replace it with space
def rmparen(txt):
	r = ''
	s = 0
	for i in range(len(txt)):
		c = txt[i]
		if c == '(':
			if s == 0:
				r = r + ' '
			s = s + 1
		elif c == ')':
			if s > 0:
				s = s - 1
		elif s == 0:
			r = r + c
	return r

# removes any character that is not alpha-numeric
def keep_alnum(txt):
	return ''.join(map(lambda x: x if x.isalnum() else ' ', txt))

# translate camel case to splitted words
# numbers are treated as plain words
def decamel(txt):
	r = ''
	l = False
	n = False
	for i in range(len(txt)):
		c = txt[i]
		if c.isdigit():
			if not n and i > 0:
				r = r + ' '
			n = True
			l = False
		else:
			if n or (l and c.isupper()):
				r = r + ' '
			l = c.islower()
			n = False
		r = r + c
	return r

# compute a transformed text from the given txt
# transformation is:
#   - replace any text in parenthesis by spaces
#   - replace not alphanumeric symbols by spaces
#   - replace camelized names by them words separated by spaces
#   - when 'nrw' is a positive number, keep at most nrw first words
#   - when 'capît' is True, capitalize each word
#   - when 'trunc' is a positive number truncates each word if length greater than trig
#   - join each resulting word with the given separator 'sep'
def trftxt(txt, trunc = None, trig = None, capit = False, sep = '_', nrw = None):
	if trunc == None:
		trunc = trig
	norm = decamel(keep_alnum(rmparen(txt))).strip().lower()
	part = norm.split()
	if type(nrw) == int and nrw > 0 and nrw < len(part):
		part = part[0:nrw]
	if type(trunc) == int and trunc > 0:
		trig = trig if type(trig) == int and trunc < trig else trunc
		part = map(lambda x: x if x.isdigit() or len(x) <= trig else x[0:trunc], part)
	if capit:
		part = map(lambda x: x.capitalize(), part)
	return (sep if type(sep) == str else '').join(part)

# translate the text 'txt' to its camelized equivalent
def camel(txt, trunc = None, nrw = None):
	return trftxt(txt, trunc = trunc, nrw = nrw, capit = True, sep ='')

# translate the text 'txt' to its snake equivalent
def snake(txt, trunc = None, capit = False, nrw = None):
	return trftxt(txt, trunc = trunc, capit = capit, nrw = nrw, sep = '_')

# translate the text 'txt' to its kebab equivalent
def kebab(txt, trunc = None, capit = False, nrw = None):
	return trftxt(txt, trunc = trunc, capit = capit, nrw = nrw, sep = '-')

def abbrev(val):
	"makes an abreviated version of val"
	return kebab(val, 4)

########################################################################
## for using symbolic names instead of CANOpen numbers #################
########################################################################

# tdef defines symbolic names for numeric datatype of DCF
tdef = {
	'0x0001': 'u1',    # a boolean
	'0x0002': 's8',    # a 8-bit signed integer
	'0x0003': 's16',   # a 16-bit signed integer
	'0x0004': 's32',   # a 32-bit signed integer
	'0x0005': 'u8',    # a 8-bit unsigned integer
	'0x0006': 'u16',   # a 16-bit unsigned integer
	'0x0007': 'u32',   # a 32-bit unsigned integer
	'0x0008': 'f32',   # a 32-bit IEEE-754 floating-point number
	'0x0009': 'vc',    # a vector of characters
	'0x000a': 'vb',    # a vector of bytes
	'0x000b': 'vw',    # a vector of (16-bit) Unicode characters
	'0x000c': 'd48',   # a 48-bit structure representing the absolute time
	'0x000d': 'd48',   # a 48-bit structure representing a time difference
	'0x000f': 'vx',    # an arbitrary large block of data
	'0x0010': 's24',   # a 24-bit signed integer
	'0x0011': 'f64',   # a 64-bit IEEE-754 floating-point number
	'0x0012': 's40',   # a 40-bit signed integer
	'0x0013': 's48',   # a 48-bit signed integer
	'0x0014': 's56',   # a 56-bit signed integer
	'0x0015': 's64',   # a 64-bit signed integer
	'0x0016': 'u24',   # a 24-bit unsigned integer
	'0x0018': 'u40',   # a 40-bit unsigned integer
	'0x0019': 'u48',   # a 48-bit unsigned integer
	'0x001a': 'u56',   # a 56-bit unsigned integer
	'0x001b': 'u64'    # a 64-bit unsigned integer
}

# odef defines symbolic names for numeric objecttype of DCF
odef = {
	'0x00': 'null',      # no data
	'0x02': 'domain',    # large amount of data
	'0x05': 'deftype',   # type definitions
	'0x06': 'defstruct', # record definition
	'0x07': 'var',       # single value
	'0x08': 'array',     # multiple similar fields
	'0x09': 'record'     # multiple fields
}

# get the symbolic data type after normalization of key
def get_tdef(key):
	# convert the key to its numeric value
	if type(key) == str:
		if key[0:2] == '0x':
			key = int(key[2:], 16)
		else:
			key = int(key)
	# convert the numeric value to a normal key
	if type(key) == int:
		key = '0x%04x' % key
	# get the symbolic type
	return tdef.get(key)

# get the symbolic object type after normalization of key
def get_odef(key):
	# convert the key to its numeric value
	if type(key) == str:
		if key[0:2] == '0x':
			key = int(key[2:], 16)
		else:
			key = int(key)
	# convert the numeric value to a normal key
	if type(key) == int:
		key = '0x%02x' % key
	# get the symbolic type
	return odef.get(key)


########################################################################
## prepare entries for canopen binding
########################################################################

# map of symbolic types to binding size
szs = {
	's8':  1,
	's16': 2,
	's32': 4,
	's64': 8,
	'u1':  1,
	'u8':  1,
	'u16': 2,
	'u32': 4,
	'u64': 8,
	'vc':  5,
}

# map of symbolic types to binding format
fmts = {
	's8':  'int',
	's16': 'int',
	's32': 'int',
	's64': 'int',
	'u1':  'uint',
	'u8':  'uint',
	'u16': 'uint',
	'u32': 'uint',
	'u64': 'uint',
	'vc':  'string',
}

# inspect the entry ent possibly from the group
# for adding it the data for binding
def filltype(ent, group = None):
	# get a data from the entry or else from its group if any
	get = lambda key: ent.get(key) or (group and group.get(key))
	# inspect the object type
	ot = get('objecttype')
	if ot:
		t = get_odef(ot)
		if t:
			ent['odef'] = t
	# inspect the data type
	dt = get('datatype')
	if dt:
		t = get_tdef(dt)
		if t:
			ent['tdef'] = t
			if t in szs:
				ent['size'] = szs[t]
			if t in fmts:
				ent['format'] = fmts[t]
	# inspect data access
	pm = get('pdomapping')
	at = get('accesstype')
	wr = at and at.upper().find("W") >= 0
	if pm == None or int(pm) == 0:
		ent['type'] = 'SDO'
	elif wr:
		ent['type'] = 'RPDO'
	else:
		ent['type'] = 'TPDO'

########################################################################
## DCF class ###########################################################
########################################################################

## This class is used for scanning the DCF file
class DCF:

	## From https://stackoverflow.com/questions/49755480/case-insensitive-sections-in-configparser#49758364
	## with fixes
	## This class is used for having case insensitive sections when reading
	## the DCF file.
	class CaseInsensitiveDict(collections.abc.MutableMapping):
		""" Ordered case insensitive mutable mapping class. """
		def __init__(self, *args, **kwargs):
			self._d = collections.OrderedDict(*args, **kwargs)
			self._convert_keys()
		def _convert_keys(self):
			for k in list(self._d.keys()):
				v = self._d.pop(k)
				self._d.__setitem__(k, v)
		def __len__(self):
			return len(self._d)
		def __iter__(self):
			return iter(self._d)
		def __setitem__(self, k, v):
			self._d[k.lower()] = v
		def __getitem__(self, k):
			return self._d[k.lower()]
		def __delitem__(self, k):
			del self._d[k.lower()]
		def copy(self):
			result = self.__class__()
			for k, v in self.items():
				result[k] = v
			return result


	# read the DCF file using the case insensitive dictionnary
	def read(self, filename):
		"read the DCF file"
		self.filename = filename
		self.config = configparser.ConfigParser(
					comment_prefixes = (';'),
					interpolation = None,
					dict_type = self.CaseInsensitiveDict,
					strict = False)
		self.config.read(filename)
		self.dico = None

	# export the dcf file as a dictionnary
	def todict(self):
		"return a dict of dict representing the DCF"
		if self.dico is None:
			self.dico = r = dict()
			for s in self.config.sections():
				sc = dict()
				r[s] = sc
				for k, v in self.config.items(s):
					sc[k] = v
		return self.dico

	# list of objects of a section listing objects
	def lisobj(self, name):
		"get the index list of the given section"
		r = []
		if name == "MandatoryObjects" or name == "OptionalObjects" or name == "ManufacturerObjects":
			for k, v in self.config.items(name):
				if k != "supportedobjects":
					r.append(v)
		return r

	# get object describing the subsection idx of the index base
	def getsub(self, base, idx):
		""
		key = base + 'sub' + str(idx)
		r = {}
		if self.config.has_section(key):
			r['objecttype'] = '0x07'
			r['id'] = r['index'] = key
			for k, v in self.config.items(key):
				r[k] = v
		return r

	# get the object of the given index
	def describe(self, index):
		"get the description of the object of given index"
		r = { 'index': index, 'objecttype': '0x07' }
		# removes 0x0* from index for getting is section base name
		base = index if index[0:2] != '0x' else index[2:]
		while base[0] == '0':
			base = base[1:]
		# get the description
		if self.config.has_section(base):
			r['id'] = base
			for k, v in self.config.items(base):
				n = k if k != 'parametername' else 'name'
				r[n] = v
		# ok if it contains at name
		ok = 'name' in r
		if ok:
			# fill the description
			filltype(r)
			snum = r.get('subnumber')
			csub = r.get('compactsubobj')
			if snum and int(snum) > 0:
				# describe the sub indexes
				ns = int(snum)
				a = []
				i = 0
				while ns > 0 and i < 1000:
					v = self.getsub(base, i)
					if v:
						v['group'] = index
						v['subindex'] = i
						v['reg'] = index + ("%02x" % i)
						filltype(v, r)
						a.append(v)
						ns = ns - 1
					i = i + 1
				r['subs'] = a
			elif csub and int(csub) > 0:
				# describe the sub indexes
				ns = int(csub)
				snams = base + 'name'
				svals = base + 'value'
				a = []
				for i in range(ns):
					ias = str(i)
					vnam = self.config.get(snams, ias, fallback=None) or r['name'] + ias
					v = {
						'name': vnam,
						'datatype': r.get('datatype') if i > 0 else '0x0005',
						'pdomapping': r.get('pdomapping') or '0' if i > 0 else '0',
						'objecttype': '0x07',
						'group': index,
						'id': base,
						'subindex': i,
						'reg': index + ("%02x" % i)
					}
					filltype(v, r)
					a.append(v)
				r['subs'] = a
			else:
				r['reg'] = index + "00"
		# done
		return r if ok else None

########################################################################
########################################################################
## generator #######################################

class Gener:
	def __init__(self, trunc = None, trig = None, capit = False, sep = '-', nrw = None, wsub = False, isep = None, info = False):
		self.d_ = {}
		self.a_ = []
		self.e_ = []
		self.wsub_ = wsub
		self.trig_ = trig
		self.trunc_ = trunc
		self.capit_ = capit
		self.sep_ = sep
		self.isep_ = sep if isep is None else isep
		self.nrw_ = nrw
		self.info_ = info

	# add an error for the given object
	def err_(self, obj, txt):
		self.e_.append({'item': obj, 'text': txt})
	
	# tell if error were reported
	def has_error(self):
		return len(self.e_) > 0
	
	# get the errors as a long text
	def error_full_text(self, full = False):
		if full:
			fun = lambda x: x['text'] + ": " + json.dumps(x['item'])
		else:
			fun = lambda x: x['text']
		return "\n".join(map(fun, self.e_))

	# get the entries	
	def entries(self):
		return self.a_

	# add an entry described by obj
	def put(self, obj):
		self.put_(obj, None)

	# add an entry described by obj of container cont (internal)
	def put_(self, obj, cont):
		odef = obj['odef'] if 'odef' in obj else '?'
		if odef == 'var':
			if 'type' in obj and 'format' in obj and 'size' in obj and 'reg' in obj:
				self.addvar_(obj, cont)
			else:
				self.err_(obj, 'unexpected variable ' + obj['id'])
		elif odef == 'record' or odef ==  'array':
			if 'subs' not in obj:
				self.err_(obj, 'unexpected empty record for ' + obj['id'])
			else:
				for s in obj['subs']:
					self.put_(s, obj)
		else:
			self.err_(obj, 'unexpected object type ' + odef + ' for ' + obj['id'])

	# add a variable described by obj of container cont (internal)
	def addvar_(self, obj, cont):
		# build the name
		name = obj.get('parametername') or obj.get('name')
		name = trftxt(name, trig = self.trig_, trunc = self.trunc_, capit = self.capit_, sep = self.sep_, nrw = self.nrw_)
		# get the index
		id = obj['id'] if self.wsub_ or not cont else cont['id']
		# build the uid
		uid = id + self.isep_ + name
		# make the descriptor for the binding
		v = {
			'uid': uid,
			'type': obj['type'],
			'format': obj['format'],
			'size' : obj['size'],
			'register' : obj['reg'] }
		if self.info_:
			ifo = obj.get('name') or obj.get('parametername')
			v['info'] = ifo
		# record the descriptor
		self.d_[obj['reg']] = v
		self.a_.append(v)

########################################################################
## main #######################################

def main():
	# options
	parser = argparse.ArgumentParser(
		description="Generation of AFB binding description from DCF"
	)
	parser.add_argument(
		"-t",
		"--truncate",
		metavar = "COUNT",
		type = int,
		default = None,
		help="truncate words of names to COUNT characters"
	)
	parser.add_argument(
		"-l",
		"--limit",
		metavar = "COUNT",
		type = int,
		default = None,
		help="truncate words of names only if having more than COUNT characters"
	)
	parser.add_argument(
		"-n",
		"--count",
		metavar = "COUNT",
		type = int,
		default = None,
		help="restrict the count of words in names to COUNT"
	)
	parser.add_argument(
		"-s",
		"--separator",
		metavar = "SEPAR",
		type = str,
		default = '-',
		help="separator of words of names"
	)
	parser.add_argument(
		"-S",
		"--separidx",
		metavar = "SEPAR",
		type = str,
		default = None,
		help="separator of index and names"
	)
	parser.add_argument(
		"-c",
		"--capitalize",
		action = 'store_true',
		help = "capitalize words of names"
	)
	parser.add_argument(
		"-m",
		"--minimal",
		action = 'store_true',
		help = "remove 'info' entry"
	)
	parser.add_argument(
		"-C",
		"--compact",
		action = 'store_true',
		help = "compact JSON output"
	)
	parser.add_argument(
		"-i",
		"--index",
		action = 'store_true',
		help = "use index for names, not sub-index"
	)
	parser.add_argument(
		"-r",
		"--range",
		action = 'append',
		default = [],
		help = "set a range of index to add"
	)
	parser.add_argument(
		"-D",
		"--debug",
		action = 'store_true',
		help = "show debugging data"
	)
	parser.add_argument(
		"-q",
		"--quiet",
		action = 'store_true',
		help = "don't show progression"
	)
	parser.add_argument(
		"-o",
		"--output",
		metavar = "FILE",
		type = str,
		default = None,
		help = "write the output to FILE instead of stdout"
	)
	parser.add_argument(
		"filename",
		help = "the name of the EDS/DCF file"
	)
	args = parser.parse_args()

	# read the DCF/EDS file
	if args.debug or not args.quiet:
		print("reading " + args.filename + " ...", file = sys.stderr)
	dcf = DCF()
	dcf.read(args.filename)

	# debugging output
	if args.debug:
		print("======== READEN FILE ===============", file = sys.stderr)
		json.dump(dcf.todict(), sys.stderr, indent=3)

		for s in ["MandatoryObjects", "OptionalObjects", "ManufacturerObjects"]:
			print("======== " + s + " ===============", file = sys.stderr)
			print(dcf.lisobj(s), file = sys.stderr)
			print("\n", file = sys.stderr)

	# list of required objects
	l = []
	for n in [ "MandatoryObjects", "OptionalObjects", "ManufacturerObjects" ]:
		l = l + dcf.lisobj(n)
	l.sort()

	# filter objects when ranges are reuired
	if len(args.range) > 0:
		def r2s(x):
			a = x.split('-')
			n = min(2, len(a))
			for i in range(n):
				b = a[i].strip().upper()
				if b == '':
					b = '0XFFFF' if i else '0X0000'
				elif b[0:2] != '0X':
					b = '0X' + b
				a[i] = b
			return a[0:n]
		r = list(map(r2s, args.range))
		o = []
		for i in l:
			v = i.upper()
			for a in r:
				lena = len(a)
				if lena == 1:
					f = v == a[0]
				elif lena == 2:
					f = a[0] <= v and v <= a[1]
				else:
					f = False
				if f:
					o.append(i)
					break
		l = o
		# debugging output
		if args.debug:
			print("======== FILTERED OBJECT ===============", file = sys.stderr)
			print(l, file = sys.stderr)
			print(file = sys.stderr)

	# debugging output
	if args.debug:
		for i in l:
			print("======== ENTRY " + i + " ===============", file = sys.stderr)
			json.dump(dcf.describe(i), sys.stderr, indent=3)
			print(file = sys.stderr)

	# iterate generation of descriptors
	if args.debug or not args.quiet:
		print("processing ...", file = sys.stderr)
	g = Gener(
		info = not args.minimal,
		trig = args.limit,
		trunc = args.truncate,
	   	capit = args.capitalize,
		sep = args.separator,
		isep = args.separidx,
		nrw = args.count,
		wsub = not args.index)
	for i in l:
		g.put(dcf.describe(i))

	# output the result
	if args.debug or not args.quiet:
		print("writing to " + (args.output or "stdout"), file = sys.stderr)
	if args.output == None:
		outf = sys.stdout
	else:
		outf = open(args.output, mode = 'wt', encoding = 'utf_8')
	json.dump(g.entries(), outf, indent = None if args.compact else 3)
	print(file = outf)

	# shows errors if any
	if g.has_error():
		print("Found errors:")
		print(g.error_full_text(args.debug))


if __name__ == "__main__":
	main()

# vim: ts=8 sw=8 sts=8 noet
