--TEST--
Connection strings: Incorrect connection string (1)
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
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

$d = new Mongo("http://foofas:5345");
var_dump($b->connected);
?>
--EXPECTF--
Fatal error: Uncaught exception 'MongoConnectionException' with message 'Failed to connect to: http:0: No such file or directory' in %sconnection-single_error-001.php:%d
Stack trace:
#0 %sconnection-single_error-001.php(4): Mongo->__construct('http://foofas:5...')
#1 {main}
  thrown in %sconnection-single_error-001.php on line %d
