--TEST--
Connection strings: Incorrect connection string (2)
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
<?php
require "tests/utils/server.inc";

$port = standalone_port();
if ($port != "27017") {
    die("skip this tests attempts to connect to the standard port");
}
?>
--FILE--
<?php
require "tests/utils/server.inc";

$d = new Mongo("foofas:5345");
var_dump($b->connected);
?>
--EXPECTF--
Fatal error: Uncaught exception 'MongoConnectionException' with message 'Failed to connect to: foofas:5345: Couldn't get host info for foofas' in %sconnection-single_error-002.php:%d
Stack trace:
#0 %sconnection-single_error-002.php(%d): Mongo->__construct('foofas:5345')
#1 {main}
  thrown in %sconnection-single_error-002.php on line %d
