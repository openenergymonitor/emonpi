# Change Log

## V1.0 
First release 

## V1.1 (11/05/15) 
Add sample from both CT's as default and set 110V VRMS for apparent power when US calibration is set e.g '2p'
https://github.com/openenergymonitor/emonpi/commit/2565eb3663847c22bfb62b0d1258f8cbc5fea224

## V1.2 (13/05/15) 
Fix AC wave detect and improve AC calibration calibration 
https://github.com/openenergymonitor/emonpi/commit/41987f1e21b3687b8b0ac6b9f5f5160cdfb808a4

# V1.3 (21/05/15)
1.3 firmware dev, add RF init ever 10min to keep RF alive
https://github.com/openenergymonitor/emonpi/commit/96b59c38c045a44e6dea3c1a5e048d038380e88a

# V1.4 (27/05/15)
Make firmware version return command 'v' return firmware version and RFM setting all on one line
https://github.com/openenergymonitor/emonpi/commit/fa4474571cffa45d03483ce9da32f021dd2df91a

# V1.5 (27/05/15)
Change pulse count to be stored as unsigned long datatype and move the end of struct to avoid problems if older decoder is used with this new firmware. Add new variable power 1 + power 2 as third item in the struct. This will be useful for US installs and Solar PV type 2 installations to provide a feed for consumption (solar PV + grid import / export)
https://github.com/openenergymonitor/emonpi/commit/c427cd2bdfce7b3c535388d1ee53cc45183edf9c