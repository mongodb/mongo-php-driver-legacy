--TEST--
Connection strings: Test multiple host names with/without port
--SKIPIF--
<?php
require_once "tests/utils/standalone.inc";

$port = standalone_port();

if ($port != "27017") {
    die("skip this tests attempts to connect to the standard port");
}
?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$hostname = hostname();
$port = standalone_port();
$ip = gethostbyname($hostname);

$a = new Mongo("$hostname,$ip");
var_dump($a->connected);
$b = new Mongo("$hostname:$port,$ip:$port");
var_dump($b->connected);
?>
--EXPECT--
bool(true)
bool(true)
