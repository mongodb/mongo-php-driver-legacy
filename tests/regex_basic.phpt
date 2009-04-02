--TEST--
MongoRegex class - basic regex functionality
--FILE--
<?php

include "Mongo.php";

$r1 = new MongoRegex();
var_dump($r1);

$r2 = new MongoRegex("/foo/bar");
var_dump($r2);

$rstupid = new MongoRegex('/zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz'.
                          'zzzzzzzzzz/'.
                          'flagflagflagflagflag');
var_dump($rstupid);

echo "$r1\n$r2\n$rstupid\n";

$m=new Mongo();
$c = $m->selectCollection('phpt', 'regex');
$c->drop();
$c->insert(array('x' => 0, 'r1' => $r1));
$c->insert(array('x' => 1, 'r2' => $r2));
$c->insert(array('x' => 2, 'stupid' => $rstupid));

var_dump($c->findOne(array('x' => 0)));
var_dump($c->findOne(array('x' => 1)));
var_dump($c->findOne(array('x' => 2)));

?>
--EXPECTF--
object(MongoRegex)#1 (2) {
  ["regex"]=>
  string(0) ""
  ["flags"]=>
  string(0) ""
}
object(MongoRegex)#2 (2) {
  ["regex"]=>
  string(3) "foo"
  ["flags"]=>
  string(3) "bar"
}
object(MongoRegex)#3 (2) {
  ["regex"]=>
  string(150) "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
  ["flags"]=>
  string(20) "flagflagflagflagflag"
}
//
/foo/bar
/zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz/flagflagflagflagflag
array(3) {
  ["_id"]=>
  object(MongoId)#8 (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["x"]=>
  int(0)
  ["r1"]=>
  object(MongoRegex)#9 (2) {
    ["regex"]=>
    string(1) ""
    ["flags"]=>
    string(1) ""
  }
}
array(3) {
  ["_id"]=>
  object(MongoId)#8 (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["x"]=>
  int(1)
  ["r2"]=>
  object(MongoRegex)#7 (2) {
    ["regex"]=>
    string(4) "foo"
    ["flags"]=>
    string(4) "bar"
  }
}
array(3) {
  ["_id"]=>
  object(MongoId)#8 (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["x"]=>
  int(2)
  ["stupid"]=>
  object(MongoRegex)#9 (2) {
    ["regex"]=>
    string(151) "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
    ["flags"]=>
    string(21) "flagflagflagflagflag"
  }
}
