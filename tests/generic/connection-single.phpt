--TEST--
Connection strings: Test single host name with/without port
--SKIPIF--
<?php
require_once "tests/utils/standalone.inc";

$port = standalone_port();
if ($port != "27017") {
    die("skip this test attempts to connect to the standard port");
}
?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$hostname = hostname();
$port     = standalone_port();

$a = new Mongo($hostname);
var_dump($a instanceof Mongo);
$b = new Mongo("$hostname:$port");
var_dump($b instanceof Mongo);
?>
--EXPECT--
bool(true)
bool(true)
