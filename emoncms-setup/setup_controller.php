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
        $val = get("mode");
        if ($val=="ethernet" || $val=="standalone" || $val=="client") {
            if ($setup->status()=="unconfigured") {
                $mysqli->query("INSERT INTO setup (wifi) VALUES ('$val')");
            } else {
                $mysqli->query("UPDATE setup SET `wifi`='$val'");
            }
            $result = "saved:$val";
        } else {
            $result = "Invalid value";
        }
        $route->format = "text"; 
    }

    else if ($route->action=="" && $setup_access) {
        if ($setup_access) $result = view("Modules/setup/hello.php",array());
    }
    
    $fullwidth = false;
    return array('content'=>$result, 'fullwidth'=>true);
}
