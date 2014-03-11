--TEST--
MongoCollection::createIndex() (empty spec)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$name = dbname();
$colName = 'indexTest';
$d = $m->$name;
$ns = "$name.$colName";
$c = $d->$colName;

$c->drop();

function createResults($result)
{
    if ( $result === NULL ) {
        return;
    }
    echo $result['ok'] ? "OK\n" : "ERR\n";
}

function dropResults($result)
{
    if ( $result === NULL ) {
        return;
    }
    echo $result['ok'] ? 'OK  ' : 'ERR ';
    if ( array_key_exists( 'errmsg', $result ) ) {
        echo $result['errmsg'];
    }
    echo "\n";
}

function showIndexes($res)
{
    echo "Indexes:\n";
    foreach ( $res as $index ) {
        echo ' - ', $index['name'], ': ';
        echo json_encode( $index['key'] ), "\n";
    }
}

try {
	createResults( $c->ensureIndex( array() ) );
} catch (MongoException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
try {
	createResults( $c->createIndex( array() ) );
} catch (MongoException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}

showIndexes($d->system->indexes->find( array('ns' => $ns) ));

createResults( $c->ensureIndex( array("indexE1" => 1), array() ) );
createResults( $c->createIndex( array("indexC1" => 1), array() ) );

showIndexes($d->system->indexes->find( array('ns' => $ns) ));
?>
--EXPECTF--
22
index specification has no elements
22
index specification has no elements
Indexes:
OK
OK
Indexes:
 - _id_: {"_id":1}
 - indexE1_1: {"indexE1":1}
 - indexC1_1: {"indexC1":1}
