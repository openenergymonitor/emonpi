<?php
// no direct access
defined('EMONCMS_EXEC') or die('Restricted access');
function setup_controller()
{
    global $path,$session,$route,$mysqli,$fullwidth;

    $result = false;
    
    require_once "Modules/setup/setup_model.php";
    $setup = new Setup($mysqli);
    
    if ($route->action=="status") {
        $route->format = "text";
        $result = $setup->status();
    }
    
    else if ($route->action=="setwifi") {
        $val = get("mode");
        if ($val=="standalone" || $val=="client") {
            if ($wifi=="unconfigured") {
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

    else if ($route->action=="") {
        $result = view("Modules/setup/hello.php",array("wifi"=>$wifi));
    }
    
    $fullwidth = false;
    return array('content'=>$result, 'fullwidth'=>true);
}
