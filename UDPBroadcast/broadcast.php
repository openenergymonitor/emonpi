<?php
sleep(1);
$port = 5005;
$broadcast_string = "emonpi.local";

$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
socket_set_option($sock, SOL_SOCKET, SO_BROADCAST, 1);
socket_sendto($sock, $broadcast_string, strlen($broadcast_string), 0, '255.255.255.255', $port);
