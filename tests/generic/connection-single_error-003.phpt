--TEST--
Connection strings: Unconnectable host/port (3)
--SKIPIF--
<?php
require_once "tests/utils/standalone.inc";
require_once "tests/utils/server.inc";

$port = standalone_port();
if ($port != "27017") {
    die("skip this tests attempts to connect to the standard port");
}
?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$d = @new Mongo("mongodb://foofas:5345");
var_dump($b->connected);
?>
--EXPECTF--
Fatal error: Uncaught exception 'MongoConnectionException' with message '%s' in %sconnection-single_error-003.php:%d
Stack trace:
#0 %sconnection-single_error-003.php(%d): Mongo->__construct('mongodb://foofa...')
#1 {main}
  thrown in %sconnection-single_error-003.php on line %d
