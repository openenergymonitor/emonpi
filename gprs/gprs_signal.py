import time
import serial
import subprocess
starttime=time.time()
#previous_signal=0



def make_percentage(value_raw):
        multiplier = 100/32
        value_perc = value_raw * multiplier
        return value_perc



def get_gsm_signal_strength(baud=115200,port='/dev/ttyO4',bytesize=8,parity='N',stopbits=1,timeout=5,perc=True):

        ser = serial.Serial(port=port, baudrate=baud, bytesize=bytesize,parity=parity, stopbits=stopbits, timeout=timeout)
        #print "DISABLING GPRS"
        subprocess.call(['/home/debian/emonpi/gprs/gprs_off.sh'])
        time.sleep(1)
        cmd="AT+CSQ\r"
        ser.write(cmd.encode())
        time.sleep(1)
        #response =ser.readline()
        result = 0
        for line in ser.readlines():
                if line.find(":") > 0:
                        line = line.strip()
                        split_line = line.split(" ")
                        if split_line:
                                int_val_arr = split_line[1].split(',')
                                # Turn on GPRS
                                time.sleep(1)
                                result =  int(int_val_arr[0])
                                if perc:
                                        val_int = int(int_val_arr[0])
                                        result = make_percentage(val_int)
                         # ser.write('AT+CGDATA="ppp",1')

        #print "ENNALBING GPRS"
        time.sleep(1)
        subprocess.call(['/home/debian/emonpi/gprs/gprs_on.sh'])

        return result
        #time.sleep(1)

def switch_off_gsm():
   ser = serial.Serial(port='/dev/ttyO4', baudrate=115200, bytesize=8,parity='N', stopbits=1, timeout=5)

   subprocess.call(['/home/debian/emonpi/gprs/gprs_off.sh'])
   time.sleep(1)

   cmd="AT+CPOWD=1\r"
   ser.write(cmd.encode())

if __name__ == '__main__':
        gprs=get_gsm_signal_strength()
        previous_signal=gprs
        #print gprs
        #print "previous signal:",previous_signal
        print "current signal: ",gprs, "%"

