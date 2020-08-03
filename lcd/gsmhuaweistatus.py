#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Thanks to https://github.com/trick77/huawei-hilink-status
# Returns status of Huawei Hi-Link USB 3G GSM modem
# Tested with E3276, E3231,

from __future__ import print_function
import math
import xmltodict
import requests


def to_size(size):
    if size == 0:
        return '0 Bytes'
    size_name = ('Bytes', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB')
    i = int(math.floor(math.log(size, 1024)))
    p = math.pow(1024, i)
    s = round(size / p, 2)
    return '%s %s' % (s, size_name[i])


def is_hilink(device_ip):
    try:
        r = requests.get(url='http://' + device_ip + '/api/device/information', timeout=(2.0, 2.0))
    except requests.exceptions.RequestException as e:
        return False
    if r.status_code != 200:
        return False
    d = xmltodict.parse(r.text, xml_attribs=True)
    return 'error' not in d


def call_api(device_ip, resource, xml_attribs=True):
    try:
        r = requests.get(url='http://' + device_ip + resource, timeout=(2.0,2.0))
    except requests.exceptions.RequestException as e:
        return False
    if r.status_code == 200:
        d = xmltodict.parse(r.text, xml_attribs=xml_attribs)
        if 'error' in d:
            raise Exception('Received error code ' + d['error']['code'] + ' for URL ' + r.url)
        return d
    else:
        raise Exception('Received status code ' + str(r.status_code) + ' for URL ' + r.url)


def get_connection_status(status):
    result = 'n/a'
    if status == '2' or status == '3' or status == '5' or status == '8' or status == '20' or status == '21' or status == '23' or status == '27' or status == '28' or status == '29' or status == '30' or status == '31' or status == '32' or status == '33':
        result = 'Invalid Profile'
    elif status == '7' or status == '11' or status == '14' or status == '37':
        result = 'Not Allowed'
    elif status == '12' or status == '13':
        result = 'No Roaming'
    elif status == '201':
        result = 'Data Exceeded'
    elif status == '900':
        result = 'Connecting'
    elif status == '901':
        result = 'Connected'
    elif status == '902':
        result = 'Disconnected'
    elif status == '903':
        result = 'Disconnecting'
    return result


def get_network_type(type):
    result = 'n/a'
    if type == '0':
        result = 'N/A'
    elif type == '1':
        result = 'GSM'
    elif type == '2':
        result = '2G+' #2.5G' #GPRS
    elif type == '3':
        result = '2G++' #2.75G' #EDGE
    elif type == '4':
        result = '3G' #WCDMA
    elif type == '5':
        result = '3G' #HSDPA
    elif type == '6':
        result = '3G' #HSUPA
    elif type == '7':
        result = '3G' #HSPA
    elif type == '8':
        result = '3G' #TD-SCDMA
    elif type == '9':
        result = '4G' #HSPA+
    elif type == '10':
        result = 'EV' #-DO rev. 0'
    elif type == '11':
        result = 'EV' #-DO rev. A'
    elif type == '12':
        result = 'EV' #-DO rev. B'
    elif type == '13':
        result = '1x' #RTT'
    elif type == '14':
        result = 'UM' #B'
    elif type == '15':
        result = '1x' #EVDV'
    elif type == '16':
        result = '3x' #RTT'
    elif type == '17':
        result = '4G' # HSPA+ 64QAM'
    elif type == '18':
        result = '4G' #HSPA+ MIMO'
    elif type == '19':
        result = '4G' #LTE'
    elif type == '41':
        result = '3G'
    return result

gsm_connection_status = ['', '']
def return_gsm_connection_status(device_ip):
    connection_status = False
    d = call_api(device_ip, '/api/monitoring/status')
    if not d:
        connection_status = d['response']['ConnectionStatus']
        signal_level = d['response']['SignalIcon']
        network_type = d['response']['CurrentNetworkType']
        gsm_connection_status[0] = get_connection_status(connection_status)
    else:
        return ['No HiLink', 'Device']
    if connection_status == '901':
        gsm_connection_status[1] = get_network_type(network_type) + ' Signal: ' + signal_level + '/5'
    return gsm_connection_status

#device_ip = '192.168.1.1'
#if len(sys.argv) == 2:
#    device_ip = sys.argv[1]
  
#if not is_hilink(device_ip):
#    print("No HiLink device")
#    sys.exit(-1)


#print(return_gsm_connection_status('192.168.1.1'))
#print('')
#print_traffic_statistics(device_ip, connection_status)
