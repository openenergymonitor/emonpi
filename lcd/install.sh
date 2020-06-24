#!/bin/bash
# -------------------------------------------------------------
# emonpilcd install script
# -------------------------------------------------------------
# Assumes emonpi repository installed via git:
# git clone https://github.com/openenergymonitor/emonpi.git

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
usrdir=${DIR/\/emonpi/}

# interrogates the shebang of emonPiLCD.py to catch the python version
shebang="$(head -1 emonPiLCD.py)"
splitted=($(echo $shebang | tr "/" "\n"))
python="${splitted[-1]}"

sudo apt update

if [ "$python" = "python3" ]; then
    sudo apt-get install python3-smbus i2c-tools python3-rpi.gpio python3-pip redis-server  python3-gpiozero -y
    pip3 install redis paho-mqtt xmltodict requests
else
    sudo apt-get install -y python-smbus i2c-tools python-rpi.gpio python-gpiozero
    pip install xmltodict
fi


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
sudo ln -sf $usrdir/emonpi/lcd/$service.service /lib/systemd/system
sudo systemctl enable $service.service
sudo systemctl restart $service.service

state=$(systemctl show $service | grep ActiveState)
echo "- Service $state"
# ---------------------------------------------------------
