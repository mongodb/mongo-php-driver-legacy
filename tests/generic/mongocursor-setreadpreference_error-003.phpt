--TEST--
MongoCursor::setReadPreference errors [3]
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
$c = $m->$db->readpref->find();
$c->setReadPreference(MongoClient::RP_PRIMARY, array( array( 'foo' => 'bar' ) ) );
$rp = $c->getReadPreference();
var_dump($rp);
?>
--EXPECTF--
Warning: MongoCursor::setReadPreference(): You can't use read preference tags with a read preference of PRIMARY in %smongocursor-setreadpreference_error-003.php on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
