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
Warning: MongoDB::setReadPreference() expects parameter 2 to be array, integer given in %smong%s-setreadpreference_error-002.php on line 23
array(2) {
  ["type"]=>
  int(0)
  ["type_string"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference() expects parameter 2 to be array, string given in %smong%s-setreadpreference_error-002.php on line 23
array(2) {
  ["type"]=>
  int(0)
  ["type_string"]=>
  string(7) "primary"
}
---

Warning: MongoDB::setReadPreference(): Tagset 1 needs to contain an array of 0 or more tags in %smong%s-setreadpreference_error-002.php on line 23
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %smong%s-setreadpreference_error-002.php on line 23
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 2 in tagset 1 needs to contain a string in %smong%s-setreadpreference_error-002.php on line 23
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 needs to contain a string in %smong%s-setreadpreference_error-002.php on line 23
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 2 needs to contain a string in %smong%s-setreadpreference_error-002.php on line 23
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

Warning: MongoDB::setReadPreference(): Tag 1 in tagset 1 has no string key in %smong%s-setreadpreference_error-002.php on line 23
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
---
