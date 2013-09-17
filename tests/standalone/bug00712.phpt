--TEST--
PHP-712: findAndModify returns empty array when nothing is found.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectDb(dbname())->bug712;
$c->drop();

$c->insert( array( 'x' => 1 ) );

$a = $c->findAndModify(
	array( 'x' => 1 ),
	array( '$set' => array( 'x' => 1 ) ),
	array( '_id' => 0, 'x' => 0 )
);

$b = $c->findAndModify(
	array( 'x' => 2 ),
	array( '$set' => array( 'x' => 1 ) ),
	array( '_id' => 0, 'x' => 0 )
);

var_dump( $a, $b );
?>
--EXPECTF--
array(0) {
}
NULL

