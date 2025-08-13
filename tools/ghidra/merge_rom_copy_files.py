#!/usr/bin/env python3

import os
import argparse


def main():
    args = parse_args()
    
    # we assume the input file names are of the form:
    # 99D.0x1000.bin where the dotted parts are model,
    # memory offset of contents, and "bin".  This is
    # the name format used by qemu-eos "-d romcpy".
    #
    # Other files are ignored.
    input_files = find_input_files(args.input_dir)

    merged_files = merge_files(input_files)

    for f in merged_files:
        with open(os.path.join(args.output_dir, f.name), "wb") as f_:
            f_.write(f.content)


class FileRegion:
    """
    Represents a region in DryOS memory;
    a starting offset and associated content.

    The name is generated from the object attributes,
    to fit the qemu-eos naming pattern from "-d romcpy".
    
    This means modifying the start offset will update the name.

    This is also why the constructor takes a model string,
    romcpy names are like this: 99D.0x1000.bin

    Simlar for end; update content and end adjusts.
    """
    def __init__(self, model, start, content):
        self.model = str(model)
        self.start = int(start, 16)
        self.content = bytes(content)

    @property
    def name(self):
        self._name = self.model + "." + hex(self.start) + ".bin"
        return self._name

    @property
    def end(self):
        return self.start + len(self.content)

    def __eq__(self, other):
        return self.start == other.start

    def __ne__(self, other):
        return self.start != other.start

    def __lt__(self, other):
        return self.start < other.start

    def __le__(self, other):
        return self.start <= other.start

    def __gt__(self, other):
        return self.start > other.start

    def __ge__(self, other):
        return self.start >= other.start

    def __repr__(self):
        s = f"{self.name}, len: {hex(len(self.content))}"
        return s


class MergeRomFilesError(Exception):
    pass


def merge_files(files):
    """
    Given a list of FileRegions, return a list of FileRegions
    representing a minimised set of files.  This set is produced
    by concatenating adjacent regions, and merging overlapping regions
    if the overlapping bytes are identical.

    If two files have overlapping bytes, some rules apply:
     - if the overlapping bytes are identical, the regions are merged
     - if one of the sets of overlapping bytes is all zeros, the regions are merged,
       with the region containing non-zero bytes being used
     - if both regions that overlap contain any non-zero bytes, whichever region
       starts at the earlier address will be used
    """
    if not files:
        return []

    sorted_files = sorted(files)
    # Sorting allows a one-pass algorithm to find all overlaps;
    # consider each item, merge successive items that overlap.
    # If a non-overlapping item is found, save the current merged region
    # and repeat the process from the new item.

    merged_files = []
    i = 0
    current = sorted_files[0]
    while i < len(sorted_files):
        i += 1
        try:
            next_ = sorted_files[i]
        except IndexError: # last item reached, we're finished
            merged_files.append(current)
            break

        if next_.start > current.end: # no overlap
            merged_files.append(current)
            current = next_

        elif next_.start == current.end:
            print("Concatenating adjacent regions: %s, %s" %
                        (current.name, next_.name))
            current.content += next_.content

        elif next_.start < current.end:
            offset = next_.start - current.start
            c_overlap = current.content[offset:offset + len(next_.content)]
            n_overlap = next_.content[:len(c_overlap)]
            if c_overlap == n_overlap:
                print("Overlap of identical bytes, merging regions: %s, %s" %
                        (current.name, next_.name))
            elif c_overlap.count(b'\x00') == len(c_overlap):
                print("Overlap where first region is all-zero, merging in second: %s, %s" %
                        (current.name, next_.name))
                start = current.content[0:offset]
                mid = n_overlap
                end = current.content[offset + len(c_overlap):]
                current.content = start + mid + end
            elif n_overlap.count(b'\x00') == len(n_overlap):
                print("Overlap where second region is all-zero, keeping first: %s, %s" %
                        (current.name, next_.name))
            else:
                print("Overlapping regions but bytes differ, keeping first region: %s, %s" %
                        (current.name, next_.name))
            current.content += next_.content[len(c_overlap):]

        else:
            raise(MergeRomFilesError("Shouldn't happen: unhandled case in merge algorithm"))
    return merged_files


def find_input_files(input_dir):
    """
    Returns a list of FileRegion objects where the name of each file
    looked like a romcpy output file.  List is restricted to one model,
    whatever file happens to match first is used - other models in the
    same dir will be ignored.
    """
    all_files = [f for f in os.listdir(input_dir)
                    if os.path.isfile(os.path.join(input_dir, f))]
    good_files = []
    for f in all_files:
        try:
            model, start, suffix = f.split(".")
        except ValueError:
            continue
        if suffix != "bin":
            continue
        if good_files and good_files[0].model != model:
            # merging regions from different models makes no sense,
            # only collect regions from one model
            continue
        if start.startswith("0x"):
            try:
                int(start, 16)
            except ValueError:
                continue
        else:
            continue
        # we passed all checks, add the file
        with open(os.path.join(input_dir, f), "rb") as f_:
            good_files.append(FileRegion(model, start, f_.read()))
    return good_files


def parse_args():
    description = '''
    Given a set of files output from qemu using "-d romcpy",
    merge the files into a smaller set for easier understanding
    and usage.  Adjacent and overlapping files are merged
    where content doesn't conflict.
    '''

    parser = argparse.ArgumentParser(description=description)

    parser.add_argument("input_dir",
                        help="directory containing files from romcpy, "
                             "names of format 99D.0x1000.bin")
    parser.add_argument("output_dir",
                        help="location to place merged output files")

    args = parser.parse_args()

    abs_input_dir = os.path.abspath(args.input_dir)
    if not os.path.isdir(abs_input_dir):
        print("input_dir didn't exist: '%s'" % abs_input_dir)
        exit()
    args.input_dir = abs_input_dir

    abs_output_dir = os.path.abspath(args.output_dir)
    if not os.path.isdir(abs_output_dir):
        print("output_dir didn't exist: '%s'" % abs_output_dir)
        exit()
    args.output_dir = abs_output_dir

    return args


if __name__ == "__main__":
    main()
