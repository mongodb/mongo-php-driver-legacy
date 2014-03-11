--TEST--
MongoCollection::createIndex() (scalar field/array of fields)
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

createResults( $c->ensureIndex("indexE1") );
createResults( $c->createIndex("indexC1") );

createResults( $c->ensureIndex( array( "indexE2" => 1 )) );
createResults( $c->createIndex( array( "indexC2" => 1 )) );

showIndexes($d->system->indexes->find( array('ns' => $ns) ));

dropResults( $c->deleteIndex("indexE1") );
dropResults( $c->deleteIndex("indexC1") );

dropResults( $c->deleteIndex( array( "indexE2" => 1 )) );
dropResults( $c->deleteIndex( array( "indexC2" => 1 )) );

showIndexes($d->system->indexes->find( array('ns' => $ns) ));
?>
--EXPECTF--
OK

Warning: MongoCollection::createIndex() expects parameter 1 to be array, string given in %s on line %d
OK
OK
Indexes:
 - _id_: {"_id":1}
 - indexE1_1: {"indexE1":1}
 - indexE2_1: {"indexE2":1}
 - indexC2_1: {"indexC2":1}
OK  
ERR index not found%S
OK  
OK  
Indexes:
 - _id_: {"_id":1}
