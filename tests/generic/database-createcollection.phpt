--TEST--
Database: Create collection
--SKIPIF--
<?php require_once "tests/utils/server.inc"; ?>
<?php $server = new MongoShellServer; $server->getStandaloneConfig(); $serverversion = $server->getServerVersion("STANDALONE"); if (version_compare($serverversion, "2.7.0", ">=") && version_compare($serverversion, "2.8.0-rc2", "<=")) { echo "skip only MongoDB < 2.7.0 or >= 2.8.0-rc3"; }; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/collection-info.inc";

$a = mongo_standalone();
$d = $a->selectDb(dbname());

// cleanup
$d->dropCollection('create-col1');
var_dump(findCollection($d, 'create-col1'));

// create
$d->createCollection('create-col1');
$retval = findCollection($d, 'create-col1');
var_dump($retval['name']);

?>
--EXPECTF--
NULL
string(%d) "%s.create-col1"
