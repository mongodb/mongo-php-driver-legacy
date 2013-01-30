--TEST--
MongoDB::setReadPreference() error setting tag sets for primary read preference mode
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) . "/../utils.inc"; ?>
<?php

$m = new_mongo();
$db = $m->phpunit;
$db->setReadPreference(MongoClient::RP_PRIMARY, array( array( 'foo' => 'bar' ) ) );
$rp = $db->getReadPreference();
var_dump($rp);
?>
--EXPECTF--
Warning: MongoDB::setReadPreference(): You can't use read preference tags with a read preference of PRIMARY in %s on line %d
array(1) {
  ["type"]=>
  string(7) "primary"
}
