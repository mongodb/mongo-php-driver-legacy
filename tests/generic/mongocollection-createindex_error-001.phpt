--TEST--
MongoCollection::createIndex() (errors)
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
	createResults( $c->ensureIndex( false ) );
} catch (MongoException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}

try {
	createResults( $c->ensureIndex( STDIN ) );
} catch (MongoException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}

showIndexes($d->system->indexes->find( array('ns' => $ns) ));
?>
--EXPECT--
22
empty string passed as key field
22
index specification has to be an array
Indexes:
