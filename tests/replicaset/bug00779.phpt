--TEST--
Test for PHP-779: Non-primary read preferences should set slaveOk bit
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$c = $mc->selectCollection(dbname(), 'bug00779');
$c->drop();
$c->insert(array('x' => 1), array('w' => 'majority'));

function check_slaveOkay($server, $query, $cursor_options)
{
    printf("Bit 2 (SlaveOk) is%s set\n", ($cursor_options['options'] & 1 << 2) ? '' : ' not');
}

$ctx = stream_context_create(array('mongodb' => array('log_query' => 'check_slaveOkay')));

// Connect to secondary in standalone mode
$mc = new MongoClient($rs['hosts'][1], array(), array('context' => $ctx));

$coll = $mc->selectCollection(dbname(), 'bug00779');

echo "Testing primary query with MongoCursor::setReadPreference()\n";
$cursor = $coll->find();
$cursor->setReadPreference(MongoClient::RP_PRIMARY);
try {
    iterator_to_array($cursor);
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting primary query with MongoCursor::setFlag()\n";
$cursor = $coll->find();
$cursor->setFlag(2, false);
try {
    iterator_to_array($cursor);
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting primary query\n";
$cursor = $coll->find();
try {
    iterator_to_array($cursor);
} catch (MongoCursorException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\nTesting primary count\n";
try {
    /* TODO: this will return an error document instead of throwing an
     * exception. Fix the expected output once PHP-781 is resolved.
     */
    $coll->count();
} catch (MongoException $e) {
    var_dump($e->getMessage(), $e->getCode());
}

echo "\n----\n";

echo "\nTesting non-primary query with MongoCursor::setReadPreference()\n";
$cursor = $coll->find();
$cursor->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED);
iterator_to_array($cursor);

echo "\nTesting non-primary query with MongoCursor::setFlag()\n";
$cursor = $coll->find();
$cursor->setFlag(2);
iterator_to_array($cursor);

echo "\nTesting non-primary query with MongoCollection::setReadPreference()\n";
$coll->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED);
$cursor = $coll->find();
iterator_to_array($cursor);

echo "\nTesting non-primary count with MongoCollection::setReadPreference()\n";
$coll->count();

?>
--EXPECTF--

Testing primary query with MongoCursor::setReadPreference()
Bit 2 (SlaveOk) is not set
string(%d) "%s:%d: not master and slaveOk=false"
int(13435)

Testing primary query with MongoCursor::setFlag()
Bit 2 (SlaveOk) is not set
string(%d) "%s:%d: not master and slaveOk=false"
int(13435)

Testing primary query
Bit 2 (SlaveOk) is not set
string(%d) "%s:%d: not master and slaveOk=false"
int(13435)

Testing primary count
Bit 2 (SlaveOk) is not set
string(%d) "Cannot run command count(): not master"
int(20)

----

Testing non-primary query with MongoCursor::setReadPreference()
Bit 2 (SlaveOk) is set

Testing non-primary query with MongoCursor::setFlag()
Bit 2 (SlaveOk) is set

Testing non-primary query with MongoCollection::setReadPreference()
Bit 2 (SlaveOk) is set

Testing non-primary count with MongoCollection::setReadPreference()
Bit 2 (SlaveOk) is set
