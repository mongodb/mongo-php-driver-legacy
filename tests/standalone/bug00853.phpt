--TEST--
Test for PHP-853: MongoCollection::batchInsert() exceptions can obscure BSON encoding exceptions.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectDb(dbname())->bug853;
$c->drop();

$document = array(
	'foo' => 42,
	'broken-utf8' => 'F' . chr( 180 ),
);

try {
	$c->insert( $document );
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}

try {
	$c->batchInsert( array( $document ) );
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}

?>
--EXPECTF--
int(12)
string(19) "non-utf8 string: F%s"
int(12)
string(19) "non-utf8 string: F%s"
