--TEST--
MongoDB::setReadPreference() error setting invalid tag sets
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) . "/../utils.inc"; ?>
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
    $m = new_mongo();
    $db = $m->phpunit;
    $db->setReadPreference(MongoClient::RP_SECONDARY, $tagset);
    $rp = $db->getReadPreference();
    var_dump($rp);

    echo "---\n";
}
?>
==DONE==
<?php exit(0); ?>
--EXPECTF--
int(4096)
string(%d) "Argument 2 passed to MongoDB::setReadPreference() must be %s array, integer given"

Warning: MongoDB::setReadPreference() expects parameter 2 to be array, integer given in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---
int(4096)
string(%d) "Argument 2 passed to MongoDB::setReadPreference() must be %s array, string given"

Warning: MongoDB::setReadPreference() expects parameter 2 to be array, string given in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tagset 1 needs to contain an array of 0 or more tags in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tag 2 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 2 needs to contain a string in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 has no string key in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
---
==DONE==
