# Change Log

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







