# Change Log - emonPi Discrete Sampling Firmware

**See continuous-sampling branch for continuous sampling**

# V2.9.0 (16/01/18)

- Allow CT's to be plugged in after unit has powered up. CT input will read zero if no CT is connected.

```
Program:   17886 bytes (54.6% Full)
(.text + .data + .bootloader)

Data:       1048 bytes (51.2% Full)
(.data + .bss + .noinit)
```

# V2.8.4 (21/12/17)

- Correct USA voltage for apparent power sampling to 120V. Only affects battery / DC USB 5V operation. Does not effect when powering unit vi AC-AC adaptor.

```
Program:   17942 bytes (54.8% Full)
(.text + .data + .bootloader)

Data:       1034 bytes (50.5% Full)
(.data + .bss + .noinit)
```

# V2.8.3 (07/04/17)

- Reduce min pulse width to 60ms


# V2.8.2 (07/04/17)

* Reduce min pulse width to 60ms

```
Program:   17892 bytes (54.6% Full)
(.text + .data + .bootloader)
Data:       1022 bytes (49.9% Full)
(.data + .bss + .noinit)
```

# V2.8.2 (10/12/16)

* Autodetect LCD on I2C address `0x27` or `0x3f`

```
Program:   17900 bytes (54.6% Full)
(.text + .data + .bootloader)
Data:       1026 bytes (50.1% Full)
(.data + .bss + .noinit)
```

# V2.8.1 (21/11/16)

* Reintroduces "quiet mode" feature as seen in the RFM2Pi firmwares to allow improved RF debugging.

Serial `1q` enables quiet mode (default) and `0q` removes quiet mode for a more verbose output including bad packets that have failed crc checks. In emonHub it can be set with `quiet = true` or `quiet = false` in the interfacer settings. [Pull request discussion](https://github.com/openenergymonitor/emonpi/pull/34).

```
Program:   19206 bytes (58.6% Full)
(.text + .data + .bootloader)
Data:        953 bytes (46.5% Full)
(.data + .bss + .noinit)
```

# V2.8 (18/11/16)

* Compiled using platformIO with [JeeLib 10th Nov 2015 (f097c0039c)](https://github.com/jcw/jeelib/tree/f097c0039c926881d80a74bec7a7aa020de610ee)
* Fixes emonTx drop off [forum discussion](https://community.openenergymonitor.org/t/emon-txs-not-updating-after-emonpi-update/2233/10)


```
Program:   19074 bytes (58.2% Full)
(.text + .data + .bootloader)
Data:        951 bytes (46.4% Full)
(.data + .bss + .noinit)
```

# V2.7 (16/11/16)

* Compiled using platformIO with latest JeeLib (7fc95a72ec)
* Consistency with github releases
* Smaller compiled size
* Ensure soft-reset introduced in V2.5 is included
  * Fix RF droping issue caused by V2.6 being mistakenly compiled

```
Program:   19100 bytes (58.3% Full)
(.text + .data + .bootloader)
Data:        951 bytes (46.4% Full)
(.data + .bss + .noinit)
```

# V2.6 (15/04/16)
* [Update to latest JeeLib with RF Fixes ](https://github.com/jcw/jeelib/issues/92) [68f6e42]~~
* Remove RF keep alive introduced in V2.5~
Compiled with Arduino 1.6.8 | Hex size: 54.8kB | Sketch uses 19,400 bytes (60%) | Global variables: 947 bytes (46%)~~

**(10/05/16) - Temporarily restored to V2.5 to fix emonTx RF dropping issue [thread [[1]](https://community.openenergymonitor.org/t/emonsd-03may16-release/145/40) [[2]](https://openenergymonitor.org/emon/node/12593)**

# V2.5 (18/03/16)
 * Soft reset RFM69CW ever 60s to ensure its kept alive

Compiled with Arduino 1.6.8 | Hex size: 55kB | Sketch uses 19,554 bytes | Global variables: 946 bytes

# V2.4 (8/03/16)
 * Adjust Vcal_EU to improve VRMS voltage reading accuracy when using UK/EU AC-AC
 * Compile with Arduino 1.6.7 with JeeLib 10th Sep 2015 f097c0039c926881d80a74bec7a7aa020de610ee
 * Updated OneWire library to V2.3.2 & updated sensors.getAddress function

Compiled with Arduino 1.6.8 | Hex size: 54.8kB | Sketch uses 19,460 bytes | Global variables: 941 bytes

# V2.3 (20/02/16)
 * Fix VRMS reading bug
 * Compile with Arduino 1.6.7

# V2.2 (30/01/16)
 * Reduction in SRAM memory usage thanks to F-macro serial string implementation
 * Display "Raspberry Pi Booting" message on LCD at startup
 * Only sample from CT channels when CT is connected to that channel (fix noise readings when no CT connected)

# V2.1 (20/1/16)
  * Allow use of group 0

# V2.0 (17/11/15)
  * Don't print ACK's to serial

# V1.9 (16/11/15)
  * Fix counting pulses faster than 110ms, strobed meter LED http://openenergymonitor.org/emon/node/11490

# V1.8 (30/10/15)
  * Enable pulse count INT1 internal pull-up to fix spurious pulse count readings

# V1.7 (12/06/15)
  * Fix bug which stopped USA AC-AC Vcal from being applied at runtime

# V1.6 (12/06/15)
  * Fix interrupt pulse debouce issue by limiting pulse width to 52ms min (50ms is default for LED pulse output meters)
  * Compile with latest JeeLib with fix for RF hang, remove RF reset added in V1.3 (bf179057ee1abc2cfa2965b454421203ac52c6c3)
  * Enable simultaneous pulse counting and temperature monitoring on single RJ45 using breakout

# V1.5 (27/05/15)
  * Change pulse count to be stored as unsigned long datatype
  * Add new variable power 1 + power 2 as third item in the node struct. This will be useful for US installs and Solar PV type 2 installations to provide a feed for consumption (solar PV + grid import / export)

**Important Note:**
After this update a manual change will be required to emonhub.conf, copy and paste the following to override the node decoder for the emonPi node [[5]] in the Nodes section of emonhub.conf:
```
[[5]]
    nodename = emonPi
    firmware = emonPi_RFM69CW_RF12Demo_DiscreteSampling.ino
    hardware = emonpi
    [[[rx]]]
        names = power1,power2,power1_plus_power2,Vrms,T1,T2,T3,T4,T5,T6,pulseCount
        datacodes = h, h, h, h, h, h, h, h, h, h, L
        scales = 1,1,1,0.01,0.1,0.1,0.1,0.1,0.1,0.1,1
        units = W,W,W,V,C,C,C,C,C,C,p
```
https://github.com/openenergymonitor/emonpi/commit/c427cd2bdfce7b3c535388d1ee53cc45183edf9c

# V1.4 (27/05/15)
Make firmware version return command 'v' return firmware version and RFM setting all on one line
https://github.com/openenergymonitor/emonpi/commit/fa4474571cffa45d03483ce9da32f021dd2df91a

# V1.3 (21/05/15)
1.3 firmware dev, add RF init ever 10min to keep RF alive
https://github.com/openenergymonitor/emonpi/commit/96b59c38c045a44e6dea3c1a5e048d038380e88a

# V1.2 (13/05/15)
Fix AC wave detect and improve AC calibration calibration
https://github.com/openenergymonitor/emonpi/commit/41987f1e21b3687b8b0ac6b9f5f5160cdfb808a4

# V1.1 (11/05/15)
Add sample from both CT's as default and set 110V VRMS for apparent power when US calibration is set e.g '2p'
https://github.com/openenergymonitor/emonpi/commit/2565eb3663847c22bfb62b0d1258f8cbc5fea224

# V1.0
First release
