#!/usr/bin/env python

# Return md5sum of file
# Pass file path as bash argument

import hashlib
import sys

if (len(sys.argv)>1):
    file_name = str(sys.argv[1])
else:
    file_name = 0

# Open,close, read file and calculate MD5 on its contents
if (file_name != 0):
    with open(file_name) as file_to_check:
        # read contents of the file
        data = file_to_check.read()
        # pipe contents of the file through
        md5_returned = hashlib.md5(data).hexdigest()
else:
    md5_returned = 0

print(md5_returned)
