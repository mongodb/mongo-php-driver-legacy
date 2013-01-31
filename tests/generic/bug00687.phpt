--TEST--
Test for PHP-687: MongoDate usec not decoded correctly on 32-bit platform
--SKIPIF--
<?php require_once dirname(__FILE__) . '/skipif.inc'; ?>
--FILE--
<?php require_once dirname(__FILE__) . '/../utils.inc'; ?>
<?php

$m = new_mongo();
$c = $m->selectCollection(dbname(), 'bug687');
$c->drop();

$date = new MongoDate();
$c->insert(array('d' => $date));
$doc = $c->findOne();

var_dump($date == $doc['d']);

?>
--EXPECT--
bool(true)
