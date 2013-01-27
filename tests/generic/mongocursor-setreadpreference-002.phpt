--TEST--
MongoDB::setReadPreference/MongoCursor::getReadPreference [2]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db	= dbname();

$baseString = sprintf("mongodb://%s:%d/%s?readPreference=secondaryPreferred", $host, $port, $db);

// set before db reference
$m = new mongo($baseString);
$m->setReadPreference(Mongo::RP_PRIMARY_PREFERRED);
$c = $m->phpunit->test->find();
var_dump($c->getReadPreference());

// set after db reference
$m = new mongo($baseString);
$c = $m->phpunit->test->find();
$m->setReadPreference(Mongo::RP_SECONDARY);
var_dump($c->getReadPreference());

?>
--EXPECT--
array(1) {
  ["type"]=>
  string(16) "primaryPreferred"
}
array(1) {
  ["type"]=>
  string(18) "secondaryPreferred"
}
