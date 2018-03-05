#!/usr/bin/env python3
"""
Utility to format configuration for emon shield
"""
import struct
import sys

c = struct.Struct('<bbfffff')

values = (1, 0, 225.5, 240, 60.6, 13.3, 1.7)

if len(sys.argv) == 8:
    values = (
        int(sys.argv[1]),
        int(sys.argv[2]),
        float(sys.argv[3]),
        float(sys.argv[4]),
        float(sys.argv[5]),
        float(sys.argv[6]),
        float(sys.argv[7]),
    )
elif len(sys.argv) == c.size + 1:
    d = bytes([int(s) for s in sys.argv[1:]])
    values = c.unpack(d)

# version, rf_enable, Vcal,Vrms; Ical1 Ical2 phase_shift;
s = c.pack(*values)

print('Config version={}, rf_enable={}, Vcal={:3.2f}, Vrms={:3.2f},'
      ' Ical1={:3.2f}, Ical2={:3.2f} phase_shift={:3.2f}'.format(*values))
for b in s:
    print(int(b), ',', end='')
print('c')
