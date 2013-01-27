--TEST--
Connection strings: Incorrect connection string (2)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
<?php
require dirname(__FILE__) . "/../utils.inc";

$port = port();
if ($port != "27017") {
	die("skip this tests attempts to connect to the standard port");
}
?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";

$d = new Mongo("foofas:5345");
var_dump($b->connected);
?>
--EXPECTF--
Fatal error: Uncaught exception 'MongoConnectionException' with message 'Failed to connect to: foofas:5345: Couldn't get host info for foofas' in %sconnection-single_error-002.php:%d
Stack trace:
#0 %sconnection-single_error-002.php(%d): Mongo->__construct('foofas:5345')
#1 {main}
  thrown in %sconnection-single_error-002.php on line %d
