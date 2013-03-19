--TEST--
Connection strings: Test unconnectable host names without replicaset name
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

/* We forget to specify the replicaset name */
$d = new Mongo("mongodb://foofas:234,foofas:5345");
$c = $d->phpunit->test1;
var_dump( $c->findOne() );
?>
--EXPECTF--
Fatal error: Uncaught exception 'MongoConnectionException' with message 'Failed to connect to: foofas:234: Couldn't get host info for foofas; Failed to connect to: foofas:5345: Couldn't get host info for foofas' in %sconnection-multiple_error-001.php:%d
Stack trace:
#0 %sconnection-multiple_error-001.php(%d): Mongo->__construct('mongodb://foofa...')
#1 {main}
  thrown in %sconnection-multiple_error-001.php on line %d
