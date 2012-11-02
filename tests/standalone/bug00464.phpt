--TEST--
Test for PHP-464: Re-implement ->connected (var_dump())
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
<?php if (version_compare(PHP_VERSION, "5.3.0", "lt")) { exit("skip var_dump() overloading only supported in 5.3 and later"); } ?>
--FILE--
<?php

require_once dirname(__FILE__) . "/../mongo-test-cfg.inc";

$m = new Mongo($STANDALONE_HOSTNAME, array("connect" => false));
var_dump($m, $m->connected);

$m = new Mongo($STANDALONE_HOSTNAME);
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
