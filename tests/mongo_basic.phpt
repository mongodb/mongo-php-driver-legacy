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
array(3) {
  ["err"]=>
  NULL
  ["nPrev"]=>
  int(1)
  ["ok"]=>
  float(1)
}
array(2) {
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
array(3) {
  ["err"]=>
  string(12) "forced error"
  ["nPrev"]=>
  int(1)
  ["ok"]=>
  float(1)
}
array(2) {
  ["err"]=>
  string(12) "forced error"
  ["ok"]=>
  float(1)
}
