--TEST--
Test for PHP-872: Driver needs to prevent \0 in key names.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$c = $m->selectDb(dbname())->bug872;
$c->drop();

$document = array(
	'fo' . chr(0) . 'o' => 42,
);

try {
	$c->insert( $document );
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECTF--
int(2)
string(32) "'\0' not allowed in key: fo\0..."
