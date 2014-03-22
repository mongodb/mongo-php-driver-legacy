--TEST--
MongoCommandCursor rewind
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

$document = $d->cursorcmd->aggregate(array(), array('cursor' => array('batchSize' => 0 )));
$c = new MongoCommandCursor(
	$m, $document["hash"], $document["cursor"]
);
$r = $c->rewind();
var_dump($r);

$c = new MongoCommandCursor(
	$m, $document["hash"], $document["cursor"]
);
$r = $c->rewind();
var_dump($r);
?>
--EXPECT--
bool(true)
bool(true)
