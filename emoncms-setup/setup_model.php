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
        if ($row = $result->fetch_object()) {
            $wifi = $row->wifi;
        }
        
        // Double check that wpa_supplicant is set
        if ($wifi=="client") {
            $wpa_supplicant = file_get_contents("/home/pi/data/wpa_supplicant.conf");
            // for ($i=0; $i<strlen($wpa_supplicant); $i++) print $wpa_supplicant[$i]." ".ord($wpa_supplicant[$i])."<br>";
            if ($wpa_supplicant=="ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\nupdate_config=1\n") {
                $wifi = "unconfigured";
            }
        }
        
        return $wifi;
    }
}
