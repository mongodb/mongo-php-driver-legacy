--TEST--
MongoClient::setReadPreference() error setting invalid tag sets
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

function myerror($errno, $errstr) {
    var_dump($errno, $errstr);
}
set_error_handler("myerror", E_RECOVERABLE_ERROR);

$tagsets = array(
    42,
    "string",
    array( 42 ),
    array( array( 42 ) ),
    array( array( 'bar' => 'foo', 42 ) ),
    array( array( 42, 'bar' => 'foo' ) ),
    array( array( 'bar' => 'foo' ), array( 42 ) ),
    array( array( 'foo' ), array( 42 ) ),
);

foreach ($tagsets as $tagset) {
    $m = new_mongo_standalone();
    $m->setReadPreference(MongoClient::RP_SECONDARY, $tagset);
    $rp = $m->getReadPreference();
    var_dump($rp);

    echo "---\n";
}
?>
==DONE==
<?php exit(0); ?>
--EXPECTF--
int(4096)
string(%d) "Argument 2 passed to MongoClient::setReadPreference() must be %s array, integer given"

Warning: MongoClient::setReadPreference() expects parameter 2 to be array, integer given in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---
int(4096)
string(%d) "Argument 2 passed to MongoClient::setReadPreference() must be %s array, string given"

Warning: MongoClient::setReadPreference() expects parameter 2 to be array, string given in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoClient::setReadPreference(): Tagset 1 needs to contain an array of 0 or more tags in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoClient::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoClient::setReadPreference(): Tag 2 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoClient::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoClient::setReadPreference(): Tag 1 in tagset 2 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoClient::setReadPreference(): Tag 1 in tagset 1 has no string key in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---
==DONE==
