<?php

$old = "America/New_York";

if (!function_exists('memory_get_usage')) {
  function memory_get_usage($arg=0) {
    return 0;
  }
}

if (!function_exists('json_encode')) {
  function json_encode($str) {
    return "$str";
  }
}

function setTimezone() {
    date_default_timezone_set("America/New_York");
}

function unsetTimezone() {
}

?>
