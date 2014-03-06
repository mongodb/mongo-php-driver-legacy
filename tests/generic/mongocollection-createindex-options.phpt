--TEST--
MongoCollection::createIndex() options
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
		if (array_key_exists( 'unique', $index ) && $index['unique'] == true ) {
			echo "unique ";
		}
		if (array_key_exists( 'dropDups', $index ) && $index['dropDups'] == true ) {
			echo "dropDups ";
		}
		if (array_key_exists( 'sparse', $index ) && $index['sparse'] == true ) {
			echo "sparse ";
		}
		if (array_key_exists( 'expireAfterSeconds', $index ) ) {
			echo "expireAfterSeconds({$index['expireAfterSeconds']}) ";
		}
        echo json_encode( $index['key'] ), "\n";
    }
}

createResults( $c->ensureIndex( array( "indexE1" => 1 ), array( 'unique' => 1 )) );
createResults( $c->createIndex( array( "indexC1" => 1 ), array( 'unique' => 1 )) );

createResults( $c->ensureIndex( array( "indexE2" => 1 ), array( 'dropDups' => 1 )) );
createResults( $c->createIndex( array( "indexC2" => 1 ), array( 'dropDups' => 1 )) );

createResults( $c->ensureIndex( array( "indexE3" => 1 ), array( 'sparse' => 1 )) );
createResults( $c->createIndex( array( "indexC3" => 1 ), array( 'sparse' => 1 )) );

createResults( $c->ensureIndex( array( "indexE4" => 1 ), array( 'expireAfterSeconds' => 42 )) );
createResults( $c->createIndex( array( "indexC4" => 1 ), array( 'expireAfterSeconds' => 43 )) );

createResults( $c->ensureIndex( array( "indexE5" => 1 ), array( 'name' => 'index_E_five' )) );
createResults( $c->createIndex( array( "indexC5" => 1 ), array( 'name' => 'index_C_five' )) );

showIndexes($d->system->indexes->find( array('ns' => $ns) ));
?>
--EXPECT--
OK
OK
OK
OK
OK
OK
OK
OK
OK
OK
Indexes:
 - _id_: {"_id":1}
 - indexE1_1: unique {"indexE1":1}
 - indexC1_1: unique {"indexC1":1}
 - indexE2_1: dropDups {"indexE2":1}
 - indexC2_1: dropDups {"indexC2":1}
 - indexE3_1: sparse {"indexE3":1}
 - indexC3_1: sparse {"indexC3":1}
 - indexE4_1: expireAfterSeconds(42) {"indexE4":1}
 - indexC4_1: expireAfterSeconds(43) {"indexC4":1}
 - index_E_five: {"indexE5":1}
 - index_C_five: {"indexC5":1}
