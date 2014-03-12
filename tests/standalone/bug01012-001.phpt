--TEST--
Test for PHP-1012: MongoCollection is_gle_op() logic
--SKIPIF--
<?php $needs = "2.5.5"; $needsOp = 'lt'; ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

MongoLog::setModule(MongoLog::IO);
MongoLog::setLevel(MongoLog::FINE);
printLogs(MongoLog::IO, MongoLog::FINE, '/is_gle_op/');

$host = MongoShellServer::getStandaloneInfo();

echo "Testing default MongoClient and MongoCollection write method options\n";

$mc = new MongoClient($host);
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->remove();
$c->remove(array(), array('w' => 0));
$c->remove(array(), array('w' => 1));
// We don't actually care about the exception here, just MongoLog's output
try { $c->remove(array(), array('w' => 'majority')); } catch (MongoWriteConcernException $e) {}
$c->remove(array(), array('w' => 0, 'fsync' => true));
$c->remove(array(), array('w' => 0, 'j' => true));

echo "\nTesting MongoClient with no options requiring GLE\n";

$mc = new MongoClient($host, array('w' => 0, 'fsync' => false, 'journal' => false));
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->remove();
$c->remove(array(), array('w' => 1));
// We don't actually care about the exception here, just MongoLog's output
try { $c->remove(array(), array('w' => 'majority')); } catch (MongoWriteConcernException $e) {}
$c->remove(array(), array('fsync' => true));
$c->remove(array(), array('j' => true));
$c->setWriteConcern(1);
$c->remove();

echo "\nTesting MongoClient with w option requiring GLE\n";

$mc = new MongoClient($host, array('w' => 1, 'fsync' => false, 'journal' => false));
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->remove();
$c->remove(array(), array('w' => 0));
$c->setWriteConcern(0);
$c->remove();

echo "\nTesting MongoClient with fsync option requiring GLE\n";

$mc = new MongoClient($host, array('w' => 0, 'fsync' => true, 'journal' => false));
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->remove();
$c->remove(array(), array('w' => 0));
$c->remove(array(), array('fsync' => false));
$c->setWriteConcern(0);
$c->remove();

echo "\nTesting MongoClient with journal option requiring GLE\n";

$mc = new MongoClient($host, array('w' => 0, 'fsync' => false, 'journal' => true));
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->remove();
$c->remove(array(), array('w' => 0));
$c->remove(array(), array('j' => false));
$c->setWriteConcern(0);
$c->remove();

echo "\nTesting MongoClient with all options requiring GLE\n";

$mc = new MongoClient($host, array('w' => 1, 'fsync' => true, 'journal' => true));
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->remove();
$c->remove(array(), array('w' => 0));
$c->remove(array(), array('fsync' => false));
$c->remove(array(), array('j' => false));
$c->remove(array(), array('w' => 0, 'fsync' => false, 'j' => false));
$c->setWriteConcern(0);
$c->remove();
$c->remove(array(), array('fsync' => false, 'j' => false));

echo "\nTesting MongoCollection with write concern\n";

$mc = new MongoClient($host, array());
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->setWriteConcern(0);
$c->remove();
$c->remove(array(), array('w' => 1));
$c->remove(array(), array('fsync' => true));
$c->remove(array(), array('j' => true));
$c->setWriteConcern(1);
$c->remove();
$c->remove(array(), array('w' => 0));
$c->remove(array(), array('fsync' => false));
$c->remove(array(), array('j' => false));

?>
===DONE===
--EXPECTF--
Testing default MongoClient and MongoCollection write method options
is_gle_op: yes
is_gle_op: no
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes

Testing MongoClient with no options requiring GLE
is_gle_op: no
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes

Testing MongoClient with w option requiring GLE
is_gle_op: yes
is_gle_op: no
is_gle_op: no

Testing MongoClient with fsync option requiring GLE
is_gle_op: yes
is_gle_op: yes
is_gle_op: no
is_gle_op: yes

Testing MongoClient with journal option requiring GLE
is_gle_op: yes
is_gle_op: yes
is_gle_op: no
is_gle_op: yes

Testing MongoClient with all options requiring GLE
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes
is_gle_op: no
is_gle_op: yes
is_gle_op: no

Testing MongoCollection with write concern
is_gle_op: no
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes
is_gle_op: yes
is_gle_op: no
is_gle_op: yes
is_gle_op: yes
===DONE===
