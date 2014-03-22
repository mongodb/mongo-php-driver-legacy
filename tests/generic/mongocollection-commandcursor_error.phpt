--TEST--
MongoCollection::commandCursor (broken cursor/batchsize structure)
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();
$dbname = dbname();

$m = new MongoClient($dsn);
$d = $m->selectDB($dbname);
$d->cursorcmd->drop();

for ($i = 0; $i < 10; $i++) {
	$d->cursorcmd->insert(array('article_id' => $i));
}

$c = $d->cursorcmd;

try {
$r = $c->aggregate(
    array( array( '$limit' => 2 ) ),
    array('cursor' => 42)
);
} catch (MongoResultException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}

?>
--EXPECTF--
16954
%s:%d: exception: cursor field must be missing or an object
