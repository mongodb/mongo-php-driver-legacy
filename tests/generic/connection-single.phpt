--TEST--
Connection strings: Test single host name with/without port
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$port = port();
if ($port != "27017") {
		die("skip this test attempts to connect to the standard port");
}
?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$hostname = hostname();
$port		 = port();

$a = new Mongo($hostname);
var_dump($a instanceof Mongo);
$b = new Mongo("$hostname:$port");
var_dump($b instanceof Mongo);
?>
--EXPECT--
bool(true)
bool(true)
