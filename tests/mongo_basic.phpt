--TEST--
Mongo class - basic connection functionality
--FILE--
<?php

include "Mongo.php";

$m = new Mongo();
echo "$m\n";
var_dump($m->connected);

$p = new Mongo("127.0.0.1:27017");
echo "$p\n";

$m->resetError();
var_dump($m->prevError());
var_dump($m->lastError());

$m->forceError();
var_dump($m->prevError());
var_dump($m->lastError());

$m->close();
var_dump($m->connected);

?>
--EXPECT--
localhost:27017
bool(true)
127.0.0.1:27017
array(2) {
  ["err"]=>
  NULL
  ["nPrev"]=>
  int(1)
}
NULL
array(2) {
  ["err"]=>
  string(12) "forced error"
  ["nPrev"]=>
  int(1)
}
string(12) "forced error"
bool(false)
