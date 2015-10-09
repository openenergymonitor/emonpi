# Slim down Raspbian by removing GUI and education packages we don't use
# based on http://blog.samat.org/2015/02/05/slimming-an-existing-raspbian-install/

# Run by:
# $ sh ./raspbian_slimdown.sh | sudo sh

# GUI-related packages
pkgs="
xserver-xorg-video-fbdev
xserver-xorg xinit
gstreamer1.0-x gstreamer1.0-omx gstreamer1.0-plugins-base
gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-alsa
gstreamer1.0-libav
epiphany-browser
lxde lxtask menu-xdg gksu
xserver-xorg-video-fbturbo
xpdf gtk2-engines alsa-utils
netsurf-gtk zenity
desktop-base lxpolkit
weston
omxplayer
raspberrypi-artwork
lightdm gnome-themes-standard-data gnome-icon-theme
qt50-snapshot qt50-quick-particle-examples
"

# Edu-related packages
pkgs="$pkgs
scratch nuscratch
timidity
smartsim penguinspuzzle
pistore
sonic-pi
python3-pifacecommon python3-pifacedigitalio python3-pifacedigital-scratch-handler python-pifacecommon python-pifacedigitalio
minecraft-pi python-minecraftpi
wolfram-engine
"

# Remove packages
for i in $pkgs; do
	echo apt-get -y remove --purge $i
done

# Remove automatically installed dependency packages
echo apt-get -y autoremove
