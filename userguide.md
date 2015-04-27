# Using the EmonPi

The guide details how to use the EmonPi, walking through setting up the software, accessing the emonpi measurement data, recording the data locally on the emonpi and forwarding the data to a remove server such as emoncms.org

Connect up power and ethernet to the emonpi, the emonpi LCD display will start by cycling through information about what is connected to the emonpi shield, how many CT's and temperature sensors. This information is being provided by the ATMega328 on board. Once the raspberrypi has booted up it will take over control of the LCD and show the status of ethernet connectivity. With Ethernet connected it will show the IP address of the emonpi on your local network.

Enter the IP address shown in your browser address bar.

This will bring up the emonpi login. Select register to create a user, enter a username, email and password twice to create the administrator account.

Development: create custom emonpi login without email address requirement, emonpi graphic and title, and automatic single account creation (automatically disable ability to create further accounts)

