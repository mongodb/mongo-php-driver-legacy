--TEST--
Mongo class - basic connection functionality
--FILE--
<?php

include "Mongo.php";

$m = new Mongo();
echo "$m\n";

$p = new Mongo("127.0.0.1:27017");
echo "$p\n";

$m->resetError();
var_dump($m->prevError());
var_dump($m->lastError());

$m->forceError();
var_dump($m->prevError());
var_dump($m->lastError());

$m->close();

?>
--EXPECT--
localhost:27017
127.0.0.1:27017
array(4) {
  ["err"]=>
  NULL
  ["n"]=>
  int(0)
  ["nPrev"]=>
  int(-1)
  ["ok"]=>
  float(1)
}
array(3) {
  ["err"]=>
  NULL
  ["n"]=>
  int(0)
  ["ok"]=>
  float(1)
}
array(4) {
  ["err"]=>
  string(12) "forced error"
  ["n"]=>
  int(0)
  ["nPrev"]=>
  int(1)
  ["ok"]=>
  float(1)
}
array(3) {
  ["err"]=>
  string(12) "forced error"
  ["n"]=>
  int(0)
  ["ok"]=>
  float(1)
}
