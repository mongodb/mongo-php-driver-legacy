--TEST--
Test for PHP-464: Re-implement ->connected (var_dump())
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
--FILE--
<?php

require_once "tests/utils/server.inc";

$m = new Mongo($STANDALONE_HOSTNAME, array("connect" => false));
var_dump($m, $m->connected);

$m = new Mongo("$STANDALONE_HOSTNAME:$STANDALONE_PORT");
var_dump($m, $m->connected);
?>
--EXPECTF--
object(Mongo)#%d (%d) {
  ["connected"]=>
  bool(false)
  ["status"]=>
  NULL
  ["server":protected]=>
  NULL
  ["persistent":protected]=>
  NULL
}
bool(false)
object(Mongo)#%d (%d) {
  ["connected"]=>
  bool(true)
  ["status"]=>
  NULL
  ["server":protected]=>
  NULL
  ["persistent":protected]=>
  NULL
}
bool(true)
