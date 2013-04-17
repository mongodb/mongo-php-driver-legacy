--TEST--
Connection strings: Test unconnectable host names
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

/* Two random names */
$d = @new Mongo("mongodb://foofas:234,foofas:5345/demo?replicaSet=seta");
$c = $d->phpunit->test1;
var_dump( $c->findOne() );
?>
--EXPECTF--
Fatal error: Uncaught exception 'MongoConnectionException' with message 'No candidate servers found' in %sconnection-multiple_error-002.php:%d
Stack trace:
#0 %sconnection-multiple_error-002.php(%d): Mongo->__construct('mongodb://foofa...')
#1 {main}
  thrown in %sconnection-multiple_error-002.php on line %d
