#!/usr/bin/env python3

import os, sys
import argparse
import subprocess

def findinasm(addr, asm):
	t = ""
	with open(asm,"r", encoding = "utf8") as fasm:
		ls = fasm.readlines()
		i = -1
		j = -1
		for l in ls:
			i += 1
			if len(l) < 9:
				continue
			if l[9] == '<':
				j = i
			if l[8] != ':':
				continue
			addr_str = "{:0>8x}".format(addr)
			val = l[0:8]
			
			if val == addr_str:
				t += "----------------------------\n"
				t += " " + ls[j]
				t += "        ...\n"
				t += " " + ls[i-5]
				t += " " + ls[i-4]
				t += " " + ls[i-3]
				t += " " + ls[i-2]
				t += " " + ls[i-1]
				t += ">" + ls[i-0]
				t += " " + ls[i+1]
				t += " " + ls[i+2]
				t += " " + ls[i+3]
				t += " " + ls[i+4]
				t += " " + ls[i+5]
				t += "        ...\n"
				t += "----------------------------\n"
				return t
			
	return t

def main():
	parser = argparse.ArgumentParser(description='stack scan')
	parser.add_argument("-o", "--out", action="store", dest="out",
		type=str, help="input out file")
	parser.add_argument("-s", "--stack", action="store", dest="stack",
		type=str, help="stack binary file")
	parser.add_argument("-S", "--source", action="store_true",
		help="generate asm with source, it is slow")
	parser.add_argument("-d", "--detail", action="store_true",
		help="generate asm with source, it is slow")

	args = parser.parse_args()
	out = ""
	nm = ""
	asm = ""
	stack = ""
	source = ""
	bottom = 0
	endaddr = ""
	startaddr = ""

	if args.out:
		out = args.out

	if args.stack:
		stack = args.stack

	if stack == "" or out == "":
		parser.print_help()
		sys.exit(1)

	asm = out + ".dis"
	nm = out + ".nm"
	if not os.path.isfile(asm):
		print("Generating %s ..." % asm)
		if args.source:
			print("/opt/mips32-mti-elf/2019.09-03-2/bin/mips-mti-elf-objdump -D -S " + out + " > " + asm)
			os.system("/opt/mips32-mti-elf/2019.09-03-2/bin/mips-mti-elf-objdump -D -S " + out + " > " + asm)
		else:
			print("/opt/mips32-mti-elf/2019.09-03-2/bin/mips-mti-elf-objdump -D " + out + " > " + asm)
			os.system("/opt/mips32-mti-elf/2019.09-03-2/bin/mips-mti-elf-objdump -D " + out + " > " + asm)

	if not os.path.isfile(nm):
		print("Generating %s ..." % nm)
		os.system("nm " + out + " > " + nm)

	p = subprocess.Popen(["awk", "/__text_start/{print $1}", nm], stdout=subprocess.PIPE)
	startaddr = int(p.stdout.read(), 16)
	p = subprocess.Popen(["awk", "/__text_end/{print $1}", nm], stdout=subprocess.PIPE)
	endaddr = int(p.stdout.read(), 16)

	with open(stack, "rb") as f:
		while True:
			strb = f.read(4)
			if strb == b"":
			    break
			val = int.from_bytes(strb, byteorder='little', signed=False)
			if (val >= startaddr) and (val <= endaddr):
				print("0x{:0>8x} : 0x{:0>8x} ".format(bottom, val))
				print(findinasm(val, asm), end='')
			elif args.detail:
				print("0x{:0>8x} : 0x{:0>8x} ".format(bottom, val))

if __name__ == '__main__':
	main()
