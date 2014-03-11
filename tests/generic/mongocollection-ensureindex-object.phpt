--TEST--
MongoCollection::ensureIndex() (scalar field/array of fields)
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

$i = new stdClass;
$i->indexE1 = 1;
createResults( $c->ensureIndex( $i ) );

$i = new stdClass;
$i->indexC1 = -1;
createResults( $c->ensureIndex( $i ) );

$i = new stdClass;
$i->indexC3 = '2d';
$i->indexC2 = -1;
createResults( $c->ensureIndex( $i ) );

showIndexes($d->system->indexes->find( array('ns' => $ns) ));
?>
--EXPECT--
OK
OK
OK
Indexes:
 - _id_: {"_id":1}
 - indexE1_1: {"indexE1":1}
 - indexC1_-1: {"indexC1":-1}
 - indexC3_2d_indexC2_-1: {"indexC3":"2d","indexC2":-1}
