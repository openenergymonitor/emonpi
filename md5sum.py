#!/usr/bin/env python

# Return md5sum of file
# Pass file path as bash argument

from __future__ import print_function
import hashlib
import sys

def main():
    if len(sys.argv) > 1:
    # Open,close, read file and calculate MD5 on its contents
        with open(sys.argv[1]) as file_to_check:
            # read contents of the file
            data = file_to_check.read()
            # pipe contents of the file through
            md5_returned = hashlib.md5(data).hexdigest()
    else:
        md5_returned = 0

    print(md5_returned)

if __name__ == '__main__':
    main()
