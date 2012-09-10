--TEST--
MongoDB::setReadPreference errors [2]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
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
	$d->setReadPreference(Mongo::RP_SECONDARY, $value);
	$rp = $d->getReadPreference();
	var_dump($rp);

	echo "---\n";
}
?>
--EXPECTF--
int(4096)
string(81) "Argument 2 passed to MongoDB::setReadPreference() must be %s array, integer given"

Warning: MongoDB::setReadPreference() expects parameter 2 to be array, integer given in %s on line %d
array(2) {
  ["type"]=>
  int(0)
  ["type_string"]=>
  string(7) "primary"
}
---
int(4096)
string(80) "Argument 2 passed to MongoDB::setReadPreference() must be %s array, string given"

Warning: MongoDB::setReadPreference() expects parameter 2 to be array, string given in %s on line %d
array(2) {
  ["type"]=>
  int(0)
  ["type_string"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tagset 1 needs to contain an array of 0 or more tags in %s on line %d
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 2 in tagset 1 needs to contain a string in %s on line %d
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %s on line %d
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 2 needs to contain a string in %s on line %d
array(3) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(1) {
      [0]=>
      string(7) "bar:foo"
    }
  }
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 has no string key in %s on line %d
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---
