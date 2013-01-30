--TEST--
Test for PHP-687: MongoDate usec not decoded correctly on 32-bit platform
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo_standalone();
$c = $m->selectCollection(dbname(), 'bug687');
$c->drop();

$date = new MongoDate();
$c->insert(array('d' => $date));
$doc = $c->findOne();

var_dump($date == $doc['d']);

?>
--EXPECT--
bool(true)
