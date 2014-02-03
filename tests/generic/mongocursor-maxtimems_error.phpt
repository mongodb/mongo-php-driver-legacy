--TEST--
MongoCursor::maxTimeMS() (error)
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());
$c = $d->maxtimems;
$c->drop();

for ( $i = 0; $i < 2; $i++ )
{
	$c->insert(array('foo' => $i));
}

$cursor = $c->find()->maxTimeMS(-1);
try {
	foreach ($cursor as $foo) {
	}
} catch (MongoCursorException $e) {
	echo get_class($e), "\n";
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
MongoCursorException
17287
%s:%d:%sBadValue%smaxTimeMS%s
