--TEST--
Test for PHP-802: Deprecate boolean options to MongoCollection::ensureIndex().
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());
$c = $d->bug802;

$c->ensureIndex(array('foo' => 1), true);
?>
--EXPECTF--
%s: Argument 2 passed to MongoCollection::ensureIndex() must be %s array, boolean given in %sbug00802.php on line %d
