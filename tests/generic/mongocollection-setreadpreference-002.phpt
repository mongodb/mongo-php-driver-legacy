--TEST--
MongoCollection::getReadPreference [2]
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
$c->setReadPreference(Mongo::RP_PRIMARY_PREFERRED);
$d = $c->phpunit->test;
var_dump($c->getReadPreference());

// set after db reference
$c = new mongo($baseString);
$d = $c->phpunit->test;
$c->setReadPreference(Mongo::RP_SECONDARY);
var_dump($c->getReadPreference());

?>
--EXPECT--
array(2) {
  ["type"]=>
  int(1)
  ["type_string"]=>
  string(17) "primary preferred"
}
array(2) {
  ["type"]=>
  int(2)
  ["type_string"]=>
  string(9) "secondary"
}
