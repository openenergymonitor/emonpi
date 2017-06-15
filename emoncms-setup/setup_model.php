<?php

// no direct access
defined('EMONCMS_EXEC') or die('Restricted access');

class Setup
{
    private $mysqli;
    
    public function __construct($mysqli)
    {
        $this->mysqli = $mysqli;
    }
    
    public function status() {
    
        $wifi = "unconfigured";
        $result = $this->mysqli->query("SELECT wifi FROM setup");
        if ($result && $row = $result->fetch_object()) {
            $wifi = $row->wifi;
        }

        $wpa_supplicant = file_get_contents("/home/pi/data/wpa_supplicant.conf");
        if ($wpa_supplicant=="ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\nupdate_config=1\n") {
            if ($wifi=="client") $wifi = "unconfigured";
        } else {
            if ($wifi=="unconfigured") $wifi = "client";
        }

        return $wifi;
    }
    
    public function set_status($val) {
        if ($val=="ethernet" || $val=="standalone" || $val=="client") {
            
            $result = $this->mysqli->query("SELECT wifi FROM setup");
            if ($result->num_rows==0) {
                $this->mysqli->query("INSERT INTO setup (wifi) VALUES ('$val')");
            } else {
                $this->mysqli->query("UPDATE setup SET `wifi`='$val'");
            }
            return "saved:$val";
        } else {
            return "Invalid value";
        }
    }
}
