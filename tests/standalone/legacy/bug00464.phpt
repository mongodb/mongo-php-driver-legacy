--TEST--
Test for PHP-464: Re-implement ->connected (var_dump())
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
--FILE--
<?php

require_once "tests/utils/server.inc";
$cfg = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($cfg, array("connect" => false));
var_dump($m, $m->connected);

$m = new MongoClient($cfg);
var_dump($m, $m->connected);
?>
--EXPECTF--
%s: The 'connected' property is deprecated in %s on line %d
object(MongoClient)#%d (%d) {
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

%s: The 'connected' property is deprecated in %s on line %d
object(MongoClient)#%d (%d) {
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
