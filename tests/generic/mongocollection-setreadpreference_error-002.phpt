--TEST--
MongoCollection::setReadPreference errors [2]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/../utils.inc"; ?>
<?php
$host = hostname();
$port = port();
$db   = dbname();

$baseString = sprintf("mongodb://%s:%d/%s", $host, $port, $db);
function myerror($errno, $errstr) {
    var_dump($errno, $errstr);
}
set_error_handler("myerror", E_RECOVERABLE_ERROR);

$a = array(
    42,
    "string",
    array( 42 ),
    array( array( 42 ) ),
    array( array( 'bar' => 'foo', 42 ) ),
    array( array( 42, 'bar' => 'foo' ) ),
    array( array( 'bar' => 'foo' ), array( 42 ) ),
    array( array( 'foo' ), array( 42 ) ),
);

foreach ($a as $value) {
    $m = new mongo($baseString);
    $d = $m->$db;
    $c = $d->test;
    $c->setReadPreference(Mongo::RP_SECONDARY, $value);
    $rp = $c->getReadPreference();
    var_dump($rp);

    echo "---\n";
}
?>
--EXPECTF--
int(4096)
string(%d) "Argument 2 passed to MongoCollection::setReadPreference() must be %s array, integer given"

Warning: MongoCollection::setReadPreference() expects parameter 2 to be array, integer given in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---
int(4096)
string(%d) "Argument 2 passed to MongoCollection::setReadPreference() must be %s array, string given"

Warning: MongoCollection::setReadPreference() expects parameter 2 to be array, string given in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoCollection::setReadPreference(): Tagset 1 needs to contain an array of 0 or more tags in %s on line %d
array(1) {
  ["type"]=>
  string(9) "secondary"
}
---

Warning: MongoCollection::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(9) "secondary"
}
---

Warning: MongoCollection::setReadPreference(): Tag 2 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(9) "secondary"
}
---

Warning: MongoCollection::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(9) "secondary"
}
---

Warning: MongoCollection::setReadPreference(): Tag 1 in tagset 2 needs to contain a string in %s on line %d
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(1) {
      ["bar"]=>
      string(3) "foo"
    }
  }
}
---

Warning: MongoCollection::setReadPreference(): Tag 1 in tagset 1 has no string key in %s on line %d
array(1) {
  ["type"]=>
  string(9) "secondary"
}
---
