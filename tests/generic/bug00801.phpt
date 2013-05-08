--TEST--
Test for PHP-801: Deprecate boolean options to MongoCollection::insert().
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($dsn);
$d = $m->selectDB(dbname());
$c = $d->bug801;
$c->drop();

$c->insert(array('foo' => 1), true);
?>
--EXPECTF--
%s: Argument 2 passed to MongoCollection::insert() must be %s array, boolean given in %sbug00801.php on line %d
