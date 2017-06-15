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
        
        // Special case, if setup flag in database is wifi client but wpa_supplicant is blank then something went wrong
        // or user cleared wpa_supplicant deliberatly in order to move emonpi to another network.
        // If this is the case then we flag up as unconfigured
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
