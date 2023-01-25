This guide details how to setup the emonPi to broadcast a WIFI Hotspot using the Edimax EW-7811UN (rtl871xdrv chipset) USB WIFI adapter to make it possible to connect directly to an EmonPi from a tablet or computer without the need for a router or internet connection. 

Bridging Ethernet (eth0) and 3G GSM modems (eth1) using NAT is also covered. The allows the emonPi to function as a 3G router while also being able to access local emoncms. Perfect for remote installations.

This guide was put together using a Pi2 running emonPi SD card `emonSD-20Feb16` based on `RASPBIAN JESSIE LITE (2015-11-21). Kernel 4.1.18`.

There are 4 pieces of software that need to be installed to get this to work:

1. hostapd – WIFI Hotspot
2. edimax version of hostapd
3. isc-dhcp-server – DHCP Server
4. iptables - NAT (link network interfaces) 

# 1) Install dhcp server 

	sudo apt-get install isc-dhcp-server 

# 2) Install Hostapd

If your using **RasPi3 or official RasPi Wifi dongle - nl80211 chipset** with the brcmfmac driver simply 

	sudo apt-get install hostapd
	
If using **Edimax EW-7811UN (rtl871xdrv chipset)** need to use latest version of hostapd (which requires compile) which supports rtl871xdrv in AP mode.

```
sudo apt-get install libnl-genl-3-dev libnl-3-dev
git clone https://github.com/lostincynicism/hostapd-rtl8188
cd hostapd-rtl8188/hostapd
sudo make
sudo make install
``` 

check vesion :

	pi@emonpi:~ $ hostapd -v
	hostapd v2.4 for Realtek rtl871xdrv

# 3) Configure /etc/network/interfaces

Open the network interfaces file to edit:

	sudo nano /etc/network/interfaces

Replace content with:

```
auto lo
iface lo inet loopback

auto eth0
allow-hotplug eth0
iface eth0 inet manual

#allow-hotplug wlan0
#iface wlan0 inet manual
#    wpa-conf /etc/wpa_supplicant/wpa_supplicant.conf


iface wlan0 inet static
address 192.168.42.1
netmask 255.255.255.0

allow-hotplug eth1
iface eth1 inet dhcp
```

# 4) Configure /etc/hostapd/hostapd.conf

Open hostapd config file with:

	sudo nano /etc/hostapd/hostapd.conf

Add the following lines to hostapd.conf (set SSID and password of your choice)


WITHOUT AUTHENTICATION  USE - for **Edimax EW-7811UN (rtl871xdrv chipset**
```
interface=wlan0
ssid=emonPi
driver=nl80211
hw_mode=g
channel=6
wmm_enabled=0
```

WITHOUT AUTHENTICATION  -if using **RasPi3 or official RasPi Wifi dongle - nl80211 chipset with the brcmfmac driver**
```
interface=wlan0
ssid=emonPi
driver=nl80211
hw_mode=g
channel=6
wmm_enabled=0
```

WITH AUTENTICATION USE - for **Edimax EW-7811UN (rtl871xdrv chipset)**

```
interface=wlan0
driver=rtl871xdrv
ssid=emonPi
hw_mode=g
channel=6
macaddr_acl=0
wmm_enabled=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=raspberry
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
```
WITH AUTENTICATION  - if using **RasPi3 or official RasPi Wifi dongle - nl80211 chipset with the brcmfmac driver**

```
interface=wlan0
driver=nl80211
ssid=emonPi
hw_mode=g
channel=6
macaddr_acl=0
wmm_enabled=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_passphrase=raspberry
rsn_pairwise=CCMP
```

Manual start to test everything is working, you should see a Wifi network called `emonPi` and password `raspberry`

	sudo hostapd -dd /etc/hostapd/hostapd.conf


#5) Configure /etc/default/hostapd

	sudo nano /etc/default/hostapd

Set

	DAEMON_CONF="/etc/hostapd/hostapd.conf"


	6) Configure /etc/dhcp/dhcpd.conf

Open dhcpd config file:

	sudo nano /etc/dhcp/dhcpd.conf

Add the following to the end of the file:

```
lease-file-name "/home/pi/data/dhcpd.leases";
subnet 192.168.42.0 netmask 255.255.255.0 {
        range 192.168.42.2 192.168.42.20;
        option broadcast-address 192.168.42.255;
        option routers 192.168.42.1;
        default-lease-time 600;
        max-lease-time 7200;
        option domain-name "local";
        option domain-name-servers 8.8.8.8, 8.8.4.4;
}
```

comment out: 

	#option domain-name "example.org";

	#option domain-name-servers ns1.example.org, ns2.example.org;

uncomment

	authoritative;

Save and exit. 

## Create dhcpd.leases in ~/data 

	sudo touch /home/pi/data/dhcpd.leases

# 7) Configure /etc/default/isc-dhcp-server

	sudo nano /etc/default/isc-dhcp-server 

Remove the comment from DHCPD_CONF, DHCPD_PID and then set the INTERFACES to "wlan0" (including ""), see example below:

```
# Defaults for isc-dhcp-server initscript 
# sourced by /etc/init.d/isc-dhcp-server 
# installed at /etc/default/isc-dhcp-server by the maintainer scripts 
# 
# This is a POSIX shell fragment 
# 

# Path to dhcpd's config file (default: /etc/dhcp/dhcpd.conf). 
DHCPD_CONF=/etc/dhcp/dhcpd.conf 

# Path to dhcpd's PID file (default: /var/run/dhcpd.pid). 
DHCPD_PID=/var/run/dhcpd.pid 

# Additional options to start dhcpd with. 
#       Don't use options -cf or -pf here; use DHCPD_CONF/ DHCPD_PID instead 
#OPTIONS="" 

# On what interfaces should the DHCP server (dhcpd) serve DHCP requests? 
#       Separate multiple interfaces with spaces, e.g. "eth0 eth1". 
INTERFACES="wlan0"
```


# 8.) Setup NAT forwarding 

To bridge eth0 the wlan0 while still allowing us to access the Pi via SSH (pure network bridge does not)

To set this up automatically on boot, edit the file /etc/sysctl.conf and 

	sudo nano /etc/sysctl.conf

Add the following line to the bottom of the file:

	net.ipv4.ip_forward=1

Second, to enable NAT in the kernel, run the following commands to bridge eth0 the wlan0:
```
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
sudo iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT
```

If bridge is also required to eth1 (for 3G dongle) also add:
```
sudo iptables -t nat -A POSTROUTING -o eth1 -j MASQUERADE
sudo iptables -A FORWARD -i eth1 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo iptables -A FORWARD -i wlan0 -o eth1 -j ACCEPT
```
Save the iptables rules to /etc/iptables.ipv4.nat

	sudo sh -c "iptables-save > /etc/iptables.ipv4.nat"

Now edit the file /etc/network/interfaces and add the following line to the bottom of the file:

	up iptables-restore < /etc/iptables.ipv4.nat

This did not work for me I had to also add entry to rc.local to load iptables (see below)

# 8) Edit /etc/rc.local

	sudo nano /etc/rc.local

Add before exit 0:

```
sudo iptables-restore < /etc/iptables.ipv4.nat
sudo ifconfig wlan0 192.168.42.1
sudo service isc-dhcp-server restart
```



# 9) Start services at boot

	sudo update-rc.d isc-dhcp-server defaults
	sudo update-rc.d hostapd defaults


Thats it restart your emonpi/emonbase to finish and then connect to network emonPi with password: raspberry

# 11) Add a Real Time Clock (RTC)
When running an emonPi in offline mode we recommend adding a hardware RTC to ensure the system time is always correct. See [emonPi modifications](https://docs.openenergymonitor.org/emonpi/modifications.html)

# Operation 

Once your up and running and connected to emonPi hotspot you should be able to browse to [http://192.168.42.1/emoncms](http://192.168.42.1/emoncms) to acess local Emoncms

# Automated Operation

I've made a script to start and stop WiFi AP mode. 

It works starting WiFi AP when emonPi is in normal Wifi mode without any rebooting and then when AP is stopped normal client mode is resumed.

The script also bridges eth1 (USB 3G dongle) to the WiFi AP. This will be useful when using emonPi with 3G dongle, this will allow a local user to connect to the emponPi via Wifi AP, to connect via SSH, view data locally and even get internet via the 3G dongle...an emonPi 3G router! 

	sudo ln -s /home/pi/emonpi/wifiAP/wifiAP.sh /usr/local/sbin/wifiAP
	
then run 

	sudo wifiAP start

Then to stop

	sudo wifiAP stop


# Forum discussion 

https://openenergymonitor.org/emon/node/12305

# Resources 
Useful blogs, guides and forum threads that I used to work out how to set up the above:

http://www.daveconroy.com/turn-your-raspberry-pi-into-a-wifi-hotspot-with-edimax-nano-usb-ew-7811un-rtl8188cus-chipset

http://askubuntu.com/questions/140126/how-do-i-configure-a-dhcp-server
http://ubuntuforums.org/showthread.php?t=2068111

http://raspberrypi.stackexchange.com/questions/9678/static-ip-failing-for-wlan0

https://forums.opensuse.org/showthread.php/438756-DHCP-can-t-write-to-dhcpd-leases

http://elinux.org/RPI-Wireless-Hotspot

http://askubuntu.com/questions/140126/how-do-i-configure-a-dhcp-server

http://www.instructables.com/id/Raspberry-Pi-as-a-3g-Huawei-E303-wireless-Edima/?ALLSTEPS
