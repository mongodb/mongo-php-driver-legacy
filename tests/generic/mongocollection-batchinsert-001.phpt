--TEST--
MongoCollection::batchInsert() sets continueOnError flag
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function print_flags($server, $docs, $options, $info) {
    var_dump($info['flags']);
}

$dsn = MongoShellServer::getStandaloneInfo();
$ctx = stream_context_create(array('mongodb' => array('log_batchinsert' => 'print_flags')));

$m = new MongoClient($dsn, array(), array('context' => $ctx));

$c = $m->selectCollection(dbname(), 'batchinsert');
$c->drop();

$documents = array(array('_id' => 1), array('_id' => 2));
$c->batchInsert($documents, array('continueOnError' => 1));

$documents = array(array('_id' => 3), array('_id' => 4));
$c->batchInsert($documents, array('continueOnError' => 0));

var_dump($c->count());
?>
--EXPECT--
int(1)
int(0)
int(4)
