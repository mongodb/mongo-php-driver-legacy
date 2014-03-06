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

/* setup */
echo "SETUP\n";
for($i = 0; $i < 5000; $i++) {
	    $c->insert( array( 'index' => $i ) );
}
echo "DONE\n";

function createResults($result)
{
    if ( $result === NULL ) {
        return;
    }
    echo $result['ok'] ? "OK\n" : "ERR\n";
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

try {
	createResults( $c->ensureIndex( array( "index" => 1, 'fieldA' => 1 ), array( 'socketTimeoutMS' => 1 )) );
} catch ( MongoCursorTimeoutException $e ) {
	echo $e->getCode(), ': ', $e->getMessage(), "\n";
}
try {
	createResults( $c->createIndex( array( "index" => 1, 'fieldB' => 1 ), array( 'socketTimeoutMS' => 1 )) );
} catch ( MongoCursorTimeoutException $e ) {
	echo $e->getCode(), ': ', $e->getMessage(), "\n";
}
sleep(3);
showIndexes($d->system->indexes->find( array('ns' => $ns) ));

?>
--EXPECTF--
SETUP
DONE
80: %s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
80: %s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
Indexes:
 - _id_: {"_id":1}
 - index_1_fieldA_1: {"index":1,"fieldA":1}
 - index_1_fieldB_1: {"index":1,"fieldB":1}
