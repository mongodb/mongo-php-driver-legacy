--TEST--
Test for PHP-667: Off-by-one error in BSON deserialization of pre-epoch dates
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = new_mongo();
$c = $m->selectCollection('phpunit', 'php_999');
$c->drop();

$mongoDate = new MongoDate(strtotime('1900-01-01 America/New_York'));
$c->insert(array('date' => $mongoDate));

$document = $c->findOne();

printf("%s\n", $mongoDate);
printf("%s\n", $document['date']);

--EXPECT--
0.00000000 -2208970800
0.00000000 -2208970800
