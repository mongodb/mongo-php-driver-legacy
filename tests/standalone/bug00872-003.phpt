--TEST--
Test for PHP-872: Driver needs to prevent \0 in database names.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$faulty = 'fo' . chr(0) . 'o';

try {
	$d = $m->$faulty;
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}

try {
	$c = $m->selectDb($faulty);
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECT--
int(2)
string(48) "Database name cannot contain null bytes: fo\0..."
int(2)
string(48) "Database name cannot contain null bytes: fo\0..."
