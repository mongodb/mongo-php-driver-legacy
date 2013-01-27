--TEST--
MongoClient::setReadPreference errors [4]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db	= dbname();

$baseString = sprintf("mongodb://%s:%d/%s", $host, $port, $db);

$m = new mongo($baseString);
$m->setReadPreference(MongoClient::RP_SECONDARY, array( array( 'foo' => 'bar' ) ) );
$m->setReadPreference(MongoClient::RP_PRIMARY, array( array( 'foo' => 'bar' ) ) );
$rp = $m->getReadPreference();
var_dump($rp);
?>
--EXPECTF--
Warning: MongoClient::setReadPreference(): You can't use read preference tags with a read preference of PRIMARY in %smongoclient-setreadpreference_error-004.php on line %d
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(1) {
	[0]=>
	array(1) {
	["foo"]=>
	string(3) "bar"
	}
  }
}
