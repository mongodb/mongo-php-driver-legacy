--TEST--
Test for PHP-667: Off-by-one error in BSON deserialization of pre-epoch dates
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php if (PHP_INT_SIZE == 4) { die("skip strtotime() doesn't support that timestamp on 32bit"); } ?>
--INI--
date.timezone=UTC
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo_standalone();
$c = $m->selectCollection(dbname(), 'bug667');
$c->drop();

$mongoDate = new MongoDate(strtotime('1900-01-01 America/New_York'));
$c->insert(array('date' => $mongoDate));

$document = $c->findOne();

printf("%s\n", $mongoDate);
printf("%s\n", $document['date']);

--EXPECT--
0.00000000 -2208970800
0.00000000 -2208970800
