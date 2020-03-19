#!/usr/bin/python3

import os
import sys
import struct
import logging
import argparse
from enum import Enum
from recordtype import recordtype
from collections import Iterable


class ConfigPart(Enum):
    IGNORE = 1
    REMOTE = 2
    CODES = 3


RemoteInfo = recordtype("RemoteInfo", [
    "name",
    "bits",
    "freq",  # Hz
    "header",
    "one",
    "zero",
    "ptrail",
    "gap",
    "codes",
])


def __read_single_config(fname) -> RemoteInfo:
    remote = RemoteInfo(
        name=None,
        bits=32,
        freq=38000,
        header=None,
        one=None,
        zero=None,
        ptrail=None,
        gap=None,
        codes={})
    part = ConfigPart.IGNORE
    with open(fname) as f:
        for line in f:
            line = line.split('#', 1)[0]
            line = line.strip()

            if len(line) == 0:
                continue

            if line == "begin remote":
                part = ConfigPart.REMOTE
            elif line == "begin codes":
                part = ConfigPart.CODES
            elif line == "end codes":
                part = ConfigPart.REMOTE
            elif line == "end remote":
                part = ConfigPart.IGNORE
            else:
                if part == ConfigPart.REMOTE:
                    __read_remote(remote, line)
                elif part == ConfigPart.CODES:
                    __read_codes(remote, line)

    return remote


def __read_remote(remote, line):
    if line.startswith("name"):
        remote.name = line.replace("name", "").strip()
    elif line.startswith("bits"):
        remote.bits = int(line.replace("bits", "").strip())
    elif line.startswith("header"):
        remote.header = [int(x) for x in filter(
            None, line.replace("header", "").strip().split(" "))]
    elif line.startswith("one"):
        remote.one = [int(x) for x in filter(
            None, line.replace("one", "").strip().split(" "))]
    elif line.startswith("zero"):
        remote.zero = [int(x) for x in filter(
            None, line.replace("zero", "").strip().split(" "))]
    elif line.startswith("ptrail"):
        remote.ptrail = [int(x) for x in filter(
            None, line.replace("ptrail", "").strip().split(" "))]
    elif line.startswith("gap"):
        remote.gap = int(line.replace("gap", "").strip())


def __read_codes(remote, line):
    (key, code) = tuple(filter(None, line.split(" ")))
    bytecode = bytearray.fromhex(code.replace("0x", ""))
    if len(bytecode) * 8 != remote.bits:
        logging.info(
            "Warning, incorrect number of bits for key '{}'".format(key))
    remote.codes[key] = bytecode


def __make_length(array, size):
    result = array[:5]
    if len(result) < size:
        result = result + [0] * (size - len(result))
    return result

def __write_remote_bin(filename, remote):

    outf = open(filename, "wb")

    # 16 bytes          name
    # 1 byte            number of bits
    # 4 bytes           frequency
    # 5x 2 bytes        header
    # 5x 2 bytes        one
    # 5x 2 bytes        zero
    # 5x 2 bytes        ptrail
    # 4 bytes           gap
    # 1 byte            number of codes
    # 16 bytes          code name
    # nb bits as bytes  encoded signal, 0 = header, 1 = one, 2 = zero, 3 = ptrail, 4 = gap?

    outf.write(struct.pack('15s', remote.name.encode('ascii')))
    outf.write(b'\0')

    outf.write(struct.pack('B', remote.bits))
    outf.write(struct.pack('I', remote.freq))
    
    for h in __make_length(remote.header, 5):
        outf.write(struct.pack('H', h))
    for h in __make_length(remote.one, 5):
        outf.write(struct.pack('H', h))
    for h in __make_length(remote.zero, 5):
        outf.write(struct.pack('H', h))
    for h in __make_length(remote.ptrail, 5):
        outf.write(struct.pack('H', h))
    outf.write(struct.pack('I', remote.gap))
    
    outf.write(struct.pack('B', len(remote.codes)))
    for name, code in remote.codes.items():
        outf.write(struct.pack('15s', name.encode('ascii')))
        outf.write(b'\0')
        bytes_as_bits = ''.join(format(byte, '08b') for byte in code)

        for b in bytes_as_bits:
            outf.write(struct.pack('?', b == '1'))
        
    outf.close()



def read_config(path):
    remotes = {}
    files = []
    # r=root, d=directories, f = files
    for r, d, f in os.walk(path):
        for file in f:
            if '.conf' in file:
                files.append(os.path.join(r, file))

    for f in files:
        print("Parsing: {}".format(f))
        remote = __read_single_config(f)
        __write_remote_bin(os.path.splitext(f)[0] + '.bin', remote)

    return remotes


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Convert .conf to .bin",
        fromfile_prefix_chars='@')

    parser.add_argument(
        "path",
        help="path to read for .conf files to convert",
        default=".",
        nargs="*",
        metavar="DIR")
    args = parser.parse_args()

    read_config(args.path)
