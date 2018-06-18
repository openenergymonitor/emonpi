<?php
// no direct access
defined('EMONCMS_EXEC') or die('Restricted access');
function setup_controller()
{
    global $path,$session,$route,$mysqli,$fullwidth;

    $result = false;

    // Special setup access to WIFI function scan and setconfig
    $setup_access = false;
    if (isset($_SESSION['setup_access']) && $_SESSION['setup_access']) $setup_access = true;
    
    require_once "Modules/setup/setup_model.php";
    $setup = new Setup($mysqli);
    
    if ($route->action=="status") {
        $route->format = "text";
        $result = $setup->status();
    }
    
    else if ($route->action=="setwifi" && $setup_access) {
        $route->format = "text"; 
        $result = $setup->set_status(get("mode"));
    }
    
    else if ($route->action=="ethernet-status" && $setup_access) {
        $route->format = "text";
        $result = exec("cat /sys/class/net/eth0/operstate");
        if ($result=="down") $result = "false";
    }
    
    else if ($route->action=="wlan0-status" && $setup_access) {
        $route->format = "text";
        $result = exec("/sys/class/net/wlan0/operstate");
        if ($result=="down") $result = "false";
    }

    else if ($route->action=="" && $setup_access) {
        $result = view("Modules/setup/hello.php",array());
    }
    
    $fullwidth = false;
    return array('content'=>$result, 'fullwidth'=>true);
}
