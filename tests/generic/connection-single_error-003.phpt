--TEST--
Connection strings: Unconnectable host/port (3)
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

$d = new Mongo("mongodb://foofas:5345");
var_dump($b->connected);
?>
--EXPECTF--
Fatal error: Uncaught exception 'MongoConnectionException' with message 'Couldn't connect to 'foofas:5345': Timed out after 1000 ms' in %sconnection-single_error-003.php:%d
Stack trace:
#0 %sconnection-single_error-003.php(%d): Mongo->__construct('mongodb://foofa...')
#1 {main}
  thrown in %sconnection-single_error-003.php on line %d
