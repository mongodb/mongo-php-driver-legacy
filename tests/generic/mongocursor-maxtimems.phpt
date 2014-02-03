--TEST--
MongoCursor::maxTimeMS().
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());
$c = $d->maxtimems;
$c->drop();

for ( $i = 0; $i < 20000; $i++ )
{
	$c->insert(array('foo' => $i));
}

$cursor = $c->find()->maxTimeMS(1);
try {
	foreach ($cursor as $foo) {
	}
} catch (MongoExecutionTimeoutException $e) {
	echo get_class($e), "\n";
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
MongoExecutionTimeoutException
50
%s:%d: operation exceeded time limit
