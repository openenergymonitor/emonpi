# Use in North America

The emonPi / emonTx hardware units are designed for use on single-phase systems. However, the they can be used on a North American Split-Phase system with the following considerations:

## 1. Ensure clip-on CT sensor will fit your conductors

Typical residential Service Entrance Wires are AWG 2/0 Copper or AWG 4/0 Aluminium.  The opening in the [standard clip-on CT sensor sold in the OpenEnergyMonitor shop](http://shop.openenergymonitor.com/100a-max-clip-on-current-sensor-ct/) is **13mm**, and hence can accommodate a maximum wire size of **AWG 1/0**. Therefore, we recommend you measure the diameter of your feeders before purchasing a CT sensor.

If the standard CT sensor does not fit your conductors there are two options:

1. Use an [Optical Pulse Sensor](https://shop.openenergymonitor.com/optical-utility-meter-led-pulse-sensor/) instead of CT sensor to interface directly with a utility meter. This will give an accurate reading of energy consumed (kWh) but will not provide a real-time power reading (W).

2. Use an alternative larger CT sensor: It's possible to obtain larger CT sensors. However, using an alternative CT will usually require re-calibration and possibly, hardware modification (burden resistor may need replacement). See [Learn > emonTx in North America](../electricity-monitoring/ac-power-theory/use-in-north-america.md) for more info. Please search / post on the [community forums](https://community.openenergymonitor.org) for further assistance.

```{warning}
Installing clip-on CT sensors on USA electrical systems should only by undertaken by a professional electrician. In a typical residential installation, even if the main breaker is switched off, the load center Service Entrance Wires and bus bars are always live.
```

## 2. Use USA AC-AC Adapter

Use the [USA AC-AC voltage sensor adapter sold via the OpenEnergyMonitor Shop](http://shop.openenergymonitor.com/ac-ac-power-supply-adapter-ac-voltage-sensor-us-plug). The system has been calibrated to work with this adapter. Using any other adapter will usually require re-calibration.

## 3. Software Calibration

a) emonPi: Set EmonHub USA Calibration in the `[[RFM2Pi]]` interfacer section of `emonhub.conf` set: `calibration=110v`.

`emonhub.conf` can be edited directly in local Emoncms as described in [EmonHub > Overview](../emonhub/overview.md). See [[[RFM2Pi]] section of emonHub configuration guide](../emonhub/configuration.md) for advanced emonhub config info.

b) emonTx: Enable USA calibration using [on-board DIP switch](../emontx3/configuration.md)

## 4. Sum power values

If monitoring a split-phase system the power values from each leg can be summed in Emoncms to calculate the total power.

The emonPi by default reports `power1_plus_power2`, this input will need to be logged to a feed. See [Setup > Logging Locally](../emoncms/intro-rpi.md).

If using emonTx then the power values from multiple CT inputs can be summed in Emoncms using `+ feed` Input Processor.

See [Learn > emonTx in North America](../electricity-monitoring/ac-power-theory/use-in-north-america.md) for further technical info about the USA system.
