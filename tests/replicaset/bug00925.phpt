--TEST--
Test for PHP-925: GridFS read methods issue a write through ensureIndex().
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$primary = MongoShellServer::getPrimaryNode();
$secondary = MongoShellServer::getASecondaryNode();

/* We use the primary to setup the test */
$p = new MongoClient($primary);
$d = $p->bug925;
$d->dropCollection("fs.chunks");
$d->dropCollection("fs.files");
$g = $d->getGridFS();
$id = $g->storeBytes("foo");

/* Sleep to fight replication lag */
sleep(1);

/* And secondary for read test */
$s = new MongoClient($secondary);
$d = $s->bug925;
$d->setReadPreference(MongoClient::RP_SECONDARY);
$g = $d->getGridFS();
$file = $g->get($id);
/* Test for getBytes */
echo $file->getBytes(), "\n";

/* Test for write */
$file->write("/tmp/foo");
echo file_get_contents("/tmp/foo"), "\n";
unlink("/tmp/foo");
?>
--EXPECT--
foo
foo
