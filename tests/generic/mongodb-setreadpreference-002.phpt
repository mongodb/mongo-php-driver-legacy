--TEST--
Mongo::setReadPreference/MongoDB::getReadPreference [2]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db   = dbname();

$baseString = sprintf("mongodb://%s:%d/%s?readPreference=secondaryPreferred", $host, $port, $db);

// set before db reference
$c = new mongo($baseString);
$c->setReadPreference(Mongo::RP_NEAREST);
$d = $c->phpunit;
var_dump($c->getReadPreference());

// set after db reference
$c = new mongo($baseString);
$d = $c->phpunit;
$c->setReadPreference(Mongo::RP_PRIMARY);
var_dump($c->getReadPreference());

?>
--EXPECT--
array(1) {
  ["type"]=>
  string(7) "nearest"
}
array(1) {
  ["type"]=>
  string(7) "primary"
}
