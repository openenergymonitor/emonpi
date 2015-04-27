# Using the EmonPi

The guide details how to use the EmonPi, walking through setting up the software, accessing the emonpi measurement data, recording the data locally on the emonpi and forwarding the data to a remove server such as emoncms.org

Connect up power and ethernet to the emonpi, the emonpi LCD display will start by cycling through information about what is connected to the emonpi shield, how many CT's and temperature sensors. This information is being provided by the ATMega328 on board. Once the raspberrypi has booted up it will take over control of the LCD and show the status of ethernet connectivity. With Ethernet connected it will show the IP address of the emonpi on your local network.

Enter the IP address shown in your browser address bar.

This will bring up the emonpi login. Select register to create a user, enter a username, email and password twice to create the administrator account.

Development: create custom emonpi login without email address requirement, emonpi graphic and title, and automatic single account creation (automatically disable ability to create further accounts)

![Create account](files/guide-createaccount.png)

Once logged in you will see the user profile page on which you can change your username, password and other user settings. 

In the top navigation menu click on *Nodes* to bring up a live view of the emonpi measurement data *node 15* by default and any other nodes on the default rfm69 network (433Mhz, group 210). The emonpi data should refresh every 5 seconds.

![EmonPi nodes](files/emonpi-nodes.png)


