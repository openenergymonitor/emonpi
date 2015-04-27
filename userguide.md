# Using the EmonPi

The guide details how to use the EmonPi, walking through setting up the software, accessing the emonpi measurement data, recording the data locally on the emonpi and forwarding the data to a remove server such as emoncms.org

Connect up power and ethernet to the emonpi, the emonpi LCD display will start by cycling through information about what is connected to the emonpi shield, how many CT's and temperature sensors. This information is being provided by the ATMega328 on board. Once the raspberrypi has booted up it will take over control of the LCD and show the status of ethernet connectivity. With Ethernet connected it will show the IP address of the emonpi on your local network.

Enter the IP address shown in your browser address bar.

This will bring up the emonpi login. Select register to create a user, enter a username, email and password twice to create the administrator account.

Development: create custom emonpi login without email address requirement, emonpi graphic and title, and automatic single account creation (automatically disable ability to create further accounts)

![Create account](files/guide-createaccount.png)

Once logged in you will see the user profile page on which you can change your username, password and other user settings. 

In the top navigation menu click on *Nodes* to bring up a live view of the emonpi measurement data *node 15* by default and any other nodes on the default rfm69 network (433Mhz, group 210). The emonpi data should refresh every 5 seconds.

![EmonPi nodes](files/guide-nodes.png)

Under the hood here we have data being read from the emonpi shield serial connection with a piece of software called emonhub and then forwarded using MQTT to the local installation of emoncms which provides the GUI, data storage and visualisation. The important thing to note is that the information for decoding, scaling and naming the node data is stored in the emonhub.conf configuration file.

The emonhub.conf configuration file can be accessed from within emoncms by clicking on the EmonHub tab in the top menu. Scrolling down to the bottom of the file you will see the node definition:

    [[15]]
        nodename = EmonPi
        firmware = emonPi_RFM69CW_RF12Demo_DiscreteSampling.ino
        hardware = emonpi
        [[[rx]]]
            names = power1,power2,pulseCount,Vrms,T1,T2,T3,T4,T5,T6
            datacode = h
            scales = 1,1,1,0.01,0.01,0.01,0.01,0.01,0.01,0.01
            units = W,W,pulses,V,C,C,C,C,C,C
            
If you wish to change the variable and node names, units or any of the other properties, change them here and click 'save'. The new configuration will then appear on the Nodes page within 5 seconds.

**EmonPi RFM69 Radio settings**

Near the top of emonhub.conf there is a section labeled RFM2Pi, The first part of the settings here specify the serial port to which the emonpi shield is connect and its baud rate. The second part contains the radio group, frequency and baseid settings which can be changed if your radio module is an 868Mhz module or that you wish to run your radio network on a different group.

    ### This interfacer manages the RFM2Pi module
    [[RFM2Pi]]
        Type = EmonHubJeeInterfacer
        [[[init_settings]]]
            com_port = /dev/ttyAMA0
            com_baud = 38400
        [[[runtimesettings]]]
            pub_channels = ToEmonCMS,
            sub_channels = ToRFM12,
            
            # datacode = B #(default:h)
            # scale = 100 #(default:1)
            group = 210 #(default:210)
            frequency = 433 #(default:433)
            baseid = 15 #(default:15)
            quiet = false #(default:true)
            # interval = 300 #(default:0)
            # nodeoffset = 32 #(default:0)

### Sending data to emoncms.org, or other remote server

To send data to emoncms.org or other remote server add the following lines in the interfacers section of emonhub.conf

    [[emoncmsorg]]
        Type = EmonHubEmoncmsHTTPInterfacer
        [[[init_settings]]]
        [[[runtimesettings]]]
            pub_channels = ToRFM12,
            sub_channels = ToEmonCMS,
            url = http://emoncms.org
            apikey = YOUR EMONCMS WRITE APIKEY
            
Click save and check using the emonhub log viewer if data is being send and the response is an 'ok' from the remote emoncms server:

    2015-04-27 16:28:08,890 INFO     emoncmsorg sending: http://emoncms.org/input/bulk.json?apikey=E-M-O-N-C-M-S-A-P-I-K-E-Y&data=[[1430152081,15,77,0,0,0,0,0,0,0,0,0],[1430152086,15,80,0,0,0,0,0,0,0,0,0]]&sentat=1430152088

### Logging data locally on the emonpi

The emonpi measurement data and data recieved from wireless nodes can be logged locally to the emonpi SD card. To record a particular node:variable click on the spanner icon next to the variable you wish to log and select the log to feed option, if your unsure which engine type to use use the default option. The interval determines the how often the data is recorded, a large interval will mean less disk space is used up but at the expense of detail.

The power to kwh option creates a feed that records the accumulating kwh used or generated calculated from the power value in this case. Accumulating kwh feeds can be used to create a bar graph of daily use or generation values.

![logging data locally: nodes config](files/guide-local.png)

The feeds page shows the feeds created, in the example below CT1 was house power and CT2 the solar power generation recorded with 'log to feed' to feeds 'house power' and 'solar power'. The 'power to kwh' option was used to generated 'house kwh' and 'solar kwh':

![logging data locally: feeds](files/guide-feeds.png)

Clicking on the eye icon will show the data recorded using the data viewer. The data can be exported as CSV data both from the feeds page and from the data viewer.
