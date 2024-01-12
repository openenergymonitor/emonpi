#!/bin/bash
# -------------------------------------------------------------
# emonpilcd install script
# -------------------------------------------------------------
# Assumes emonpi repository installed via git:
# git clone https://github.com/openenergymonitor/emonpi.git

usrdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# sudo apt update

sudo apt-get install -y python3-smbus i2c-tools python3-rpi.gpio python3-gpiozero
sudo apt-get install -y libopenjp2-7
pip3 install redis paho-mqtt xmltodict requests adafruit-circuitpython-ssd1306 Pillow

# Uncomment dtparam=i2c_arm=on
sudo sed -i "s/^#dtparam=i2c_arm=on/dtparam=i2c_arm=on/" /boot/config.txt
# Append line i2c-dev to /etc/modules
sudo sed -i -n '/i2c-dev/!p;$a i2c-dev' /etc/modules


# ---------------------------------------------------------
# Install service
# ---------------------------------------------------------
service=emonPiLCD

if [ -f /lib/systemd/system/$service.service ]; then
    echo "- reinstalling $service.service"
    sudo systemctl stop $service.service
    sudo systemctl disable $service.service
    sudo rm /lib/systemd/system/$service.service
else
    echo "- installing $service.service"
fi

# ---------------------------------------------------------
# Install service
# ---------------------------------------------------------
echo "- installing emonPiLCD.service"
sudo ln -sf $usrdir/$service.service /lib/systemd/system
sudo systemctl enable $service.service
sudo systemctl restart $service.service

state=$(systemctl show $service | grep ActiveState)
echo "- Service $state"
# ---------------------------------------------------------
