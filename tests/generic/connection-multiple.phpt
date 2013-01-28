--TEST--
Connection strings: Test multiple host names with/without port
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$port = port();

if ($port != "27017") {
		die("skip this tests attempts to connect to the standard port");
}
?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$hostname = hostname();
$port = port();
$ip = gethostbyname($hostname);

$a = new Mongo("$hostname,$ip");
var_dump($a->connected);
$b = new Mongo("$hostname:$port,$ip:$port");
var_dump($b->connected);
?>
--EXPECT--
bool(true)
bool(true)
