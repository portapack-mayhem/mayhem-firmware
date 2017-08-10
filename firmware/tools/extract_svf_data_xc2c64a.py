#!/usr/bin/env python

#
# Copyright (C) 2016 Jared Boone, ShareBrained Technology, Inc.
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# Very fragile code to extract data from Xilinx XC2C64A CPLD SVF

import sys
import re
import os.path
import argparse

def crack_variable_path(variable_path):
	tmp = args.variable_path.split('::')
	namespaces, variable_name = tmp[:-1], tmp[-1]
	return namespaces, variable_name

parser = argparse.ArgumentParser()
parser.add_argument('input_file_path', type=str)
parser.add_argument('variable_path', type=str)
# parser.add_argument('header_file_path', type=str)
parser.add_argument('data_file_path', type=str)
args = parser.parse_args()

f = open(args.input_file_path, 'r')
namespaces, variable_name = crack_variable_path(args.variable_path)

def to_hex(value, length_bits):
	return ('%x' % value).zfill((length_bits + 3) >> 2)

def long_int_to_bytes(n, bit_count):
	byte_count = (bit_count + 7) >> 3
	h = ('%x' % n).zfill(byte_count * 2)
	return [int(h[n:n+2], 16) for n in range(0, len(h), 2)]

re_sdr = re.compile(r'^(?P<length>\d+)\s*TDI\s*\((?P<tdi>[0-9A-F]+)\)(|\s*SMASK\s*\((?P<smask>[0-9A-F]+)\))(|\s*TDO\s*\((?P<tdo>[0-9A-F]+)\))(|\s*MASK\s*\((?P<mask>[0-9A-F]+)\))\s*;$')
re_sir = re_sdr

class SIR_or_SDR(object):	
	def __init__(self, name, s):
		self.name = name
		match = re_sdr.match(s)
		self.length = int(match.group('length'))
		self.tdi = self._bits(match.group('tdi'))
		self.smask = self._optional_bits(match.group('smask'))
		self.tdo = self._optional_bits(match.group('tdo'))
		self.mask = self._optional_bits(match.group('mask'))

	def __repr__(self):
		result = [self.name, str(self.length)]
		result.append('TDI (%s)' % to_hex(self.tdi, self.length))
		if self.smask:
			result.append('SMASK (%s)' % to_hex(self.smask, self.length))
		if self.tdo:
			result.append('TDO (%s)' % to_hex(self.tdo, self.length))
		if self.mask:
			result.append('MASK (%s)' % to_hex(self.mask, self.length))
		result.append(';')
		return ' '.join(result)

	def _bits(self, matched):
		return int(matched, 16)

	def _optional_bits(self, matched):
		return self._bits(matched) if matched else None

class SDR(SIR_or_SDR):
	def __init__(self, s):
		SIR_or_SDR.__init__(self, 'SDR', s)

class SIR(SIR_or_SDR):
	def __init__(self, s):
		SIR_or_SDR.__init__(self, 'SIR', s)

class SVFParser(object):
	instruction_parsers = {
		'SIR': SIR,
		'SDR': SDR,	
	}

	def parse(self, f, instruction_handler_map):
		complete_line = ''
		for line in f:
			line = line.strip().upper()
			if line.startswith('//'):
				continue

			complete_line += line
			if not line.endswith(';'):
				continue

			instruction_name, args_string = complete_line.split(None, 1)
			instruction = self.instruction_parser(instruction_name, args_string)
			if instruction:
				instruction_type = type(instruction)
				if instruction_type in instruction_handler_map:
					instruction_handler_map[instruction_type](instruction)

			if complete_line.endswith(';'):
				complete_line = ''

	def instruction_parser(self, instruction_name, args_string):
		if instruction_name in self.instruction_parsers:
			parser = self.instruction_parsers[instruction_name]
			return parser(args_string)
		else:
			return None

class ProgramExtractor(object):
	idcode = int('0bXXXX0110111001011XXX000010010011'.replace('X', '0'), 2)
	idcode_mask = None
	id_bits = 7
	block_bits = 274
	block_ordinals = frozenset(list(range(64)) + list(range(80, 82)) + list(range(96, 128)))
	block_count = len(block_ordinals)

	def __init__(self):
		self.tap_instruction = None
		self.program_data = []
		self.verify_data = []
		self.sdr_smask = None
		self.sdr_mask = None

	def map(self):
		return {
			SIR: self.on_sir,
			SDR: self.on_sdr,
		}

	def on_sir(self, o):
		assert(o.length == 8)

		if o.tdi == 0x01:
			self.tap_instruction = 'idcode'
		elif o.tdi == 0xea:
			self.tap_instruction = 'program'
			self.program_data.append([])
		elif o.tdi == 0xee:
			self.tap_instruction = 'verify'
			self.verify_data.append([])
			self.verify_block_id = None
		else:
			self.tap_instruction = None

	def on_sdr(self, o):
		if o.smask:
			self.sdr_smask = o.smask
		if o.mask:
			self.sdr_mask = o.mask

		if self.tap_instruction == 'idcode':
			assert(o.length == 32)
			assert(self.sdr_smask == 0xffffffff)
			assert((o.tdo & self.sdr_mask) == self.idcode)
			if self.idcode_mask is None:
				self.idcode_mask = self.sdr_mask
			else:
				assert(self.idcode_mask == self.sdr_mask)

		elif self.tap_instruction == 'program':
			assert(o.length == (self.id_bits + self.block_bits))
			assert(self.sdr_smask == ((1 << o.length) - 1))
			assert(o.tdo is None)
			assert(o.mask is None)
			block_id = o.tdi >> (o.length - 7)
			mask = (1 << (o.length - 7)) - 1
			self.program_data[-1].append({
				'id': block_id,
				'tdi': o.tdi & mask,
				'length': o.length - 7,
			})
		elif self.tap_instruction == 'verify':
			assert(o.length in (self.id_bits, self.block_bits))

			if o.length == self.id_bits:
				assert(o.smask == ((1 << self.id_bits) - 1))
				assert(o.tdo is None)
				assert(o.mask is None)
				self.verify_block_id = o.tdi

			elif o.length == self.block_bits:
				assert(o.tdi == (1 << o.length) - 1)
				assert(o.smask == (1 << o.length) - 1)
				self.verify_data[-1].append({
					'id': self.verify_block_id,
					'tdo': o.tdo,
					'mask': o.mask,
					'length': o.length,
				})
				self.verify_block_id = None

program_extractor = ProgramExtractor()

parser = SVFParser()
parser.parse(f, program_extractor.map())

def has_all_blocks(blocks):
	ordinals = set()

	for block in blocks:
		ordinal = int(bin(block['id'])[2:].zfill(7)[::-1], 2)
		ordinals.add(ordinal)

	return ordinals == program_extractor.block_ordinals

def is_verify_blank(blocks):
	for block in blocks:
		length = block['length']
		mask = (1 << length) - 1
		if block['tdo'] != mask:
			return False
		if block['mask'] != mask:
			return False
	return True

def deduplicate(passes):
	result = [passes[0]]
	for this_pass in passes[1:]:
		if this_pass != result[0]:
			result.append(this_pass)
	return result

def program_and_verify_match(program, verify):
	for program_block, verify_block in zip(program, verify):
		if program_block['tdi'] != verify_block['tdo']:
			return False
	return True

program_passes = [blocks for blocks in program_extractor.program_data if has_all_blocks(blocks)]
program_done = [blocks for blocks in program_extractor.program_data if len(blocks) == 1][0]
if len(program_passes) == 0:
	raise RuntimeError('no complete program passes')
if len(program_passes) > 1:
	raise RuntimeError('too many program passes')
program = program_passes[0]

verify_passes = [blocks for blocks in program_extractor.verify_data if has_all_blocks(blocks) and not is_verify_blank(blocks)]
verify_passes = deduplicate(verify_passes)
if len(verify_passes) == 0:
	raise RuntimeError('no complete verify passes')
if len(verify_passes) > 1:
	raise RuntimeError('too many verify passes')
verify = verify_passes[0]

if not program_and_verify_match(program, verify):
	raise RuntimeError('program and verify data do not match')

class FileGen(object):
	def comment_header(self):
		return ['/*' , ' * WARNING: Auto-generated file. Do not edit.', '*/']

	def includes(self, filenames):
		return ['#include "%s"' % filename for filename in filenames]

	def _namespaces(self, ns_list, line_format):
		return [line_format % ns for ns in ns_list]

	def namespaces_start(self, ns_list):
		return self._namespaces(ns_list, 'namespace %s {')
		
	def namespaces_end(self, ns_list):
		return self._namespaces(ns_list, '} /* namespace %s */')

	def verify_block(self, block):
		tdo_bytes = long_int_to_bytes(block['tdo'], block['length'])
		mask_bytes = long_int_to_bytes(block['mask'], block['length'])
		tdo_cpp_s = ', '.join(['0x%02x' % b for b in tdo_bytes]);
		mask_cpp_s = ', '.join(['0x%02x' % b for b in mask_bytes]);
		return '\t{ 0x%02x, { { %s } }, { { %s } } },' % (block['id'], tdo_cpp_s, mask_cpp_s)

	def verify_blocks_declaration(self, type_name, variable_name):
		return ['extern const %s %s;' % (type_name, variable_name)]

	def verify_blocks_definition(self, type_name, variable_name, blocks):
		return ['const %s %s { {' % (type_name, variable_name)] \
			+ [self.verify_block(block) for block in blocks] \
			+ ['} };']

	# def cpp_program_t(block):
	# 	tdi_bytes = long_int_to_bytes(block['tdi'], block['length'])
	# 	tdi_cpp_s = ', '.join(['0x%02x' % b for b in tdi_bytes]);
	# 	return '0x%02x, { { %s } }' % (block['id'], tdi_cpp_s)

	def __repr__(self):
		return '\n'.join(self.lines)

	def to_file(self, file_path):
		f = open(file_path, 'w')
		f.write(str(self))
		f.close()

class HeaderGen(FileGen):
	def __init__(self, includes, namespaces, type_name, variable_name):
		self.lines = self.comment_header() \
			+ self.includes(includes) \
			+ self.namespaces_start(namespaces) \
			+ self.verify_blocks_declaration(type_name, variable_name) \
			+ self.namespaces_end(namespaces) \
			+ ['']

class DataGen(FileGen):
	def __init__(self, includes, namespaces, type_name, variable_name, verify_blocks):
		self.lines = self.comment_header() \
			+ self.includes(includes) \
			+ self.namespaces_start(namespaces) \
			+ self.verify_blocks_definition(type_name, variable_name, verify_blocks) \
			+ self.namespaces_end(namespaces) \
			+ ['']

# Tricky (a.k.a. "brittle") code to set DONE bit
for block in verify:
	if block['id'] == 0x05:
		block['tdo'] = program_done[0]['tdi']

# header_file_name = os.path.split(args.header_file_path)[1]
header_file_name = 'hackrf_cpld_data.hpp'

# header_includes = ('cpld_xilinx.hpp',)
data_includes = (header_file_name,)

type_name = '::cpld::xilinx::XC2C64A::verify_blocks_t'

# HeaderGen(header_includes, namespaces, type_name, variable_name).to_file(args.header_file_path)
DataGen(data_includes, namespaces, type_name, variable_name, verify).to_file(args.data_file_path)

