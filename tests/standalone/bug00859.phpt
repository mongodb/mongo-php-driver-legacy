--TEST--
Test for PHP-859: MongoCollection::save() crashes when giving options.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectDb(dbname())->bug859;
$c->drop();

$document = array('data' => 'test');
$writeConcern = array('w' => 1, 'j' => true);
$c->save($document, $writeConcern);
echo "DONE\n";
?>
--EXPECTF--
DONE
