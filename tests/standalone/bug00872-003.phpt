--TEST--
Test for PHP-872: Driver needs to prevent \0 in database names.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$faulty = 'f0o' . chr(0) . 'o';

try {
	$d = $m->$faulty;
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}

$faulty = 'f1o' . chr(0) . 'o';

try {
	$c = $m->selectDb($faulty);
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECTF--
int(2)
string(44) "'\0' not allowed in database names: f0o\0..."
int(2)
string(44) "'\0' not allowed in database names: f1o\0..."
