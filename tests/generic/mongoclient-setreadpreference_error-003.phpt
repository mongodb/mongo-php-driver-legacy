--TEST--
MongoClient::setReadPreference errors [3]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db   = dbname();

$baseString = sprintf("mongodb://%s:%d/%s", $host, $port, $db);

$m = new mongo($baseString);
$m->setReadPreference(MongoClient::RP_PRIMARY, array( array( 'foo' => 'bar' ) ) );
$rp = $m->getReadPreference();
var_dump($rp);
?>
--EXPECTF--
Warning: MongoClient::setReadPreference(): You can't use read preference tags with a read preference of PRIMARY in %smongoclient-setreadpreference_error-003.php on line %d
array(2) {
  ["type"]=>
  int(0)
  ["type_string"]=>
  string(7) "primary"
}
