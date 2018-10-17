#!/bin/bash

# Disable SSH
sudo update-rc.d ssh disable
sudo invoke-rc.d ssh stop

exit
