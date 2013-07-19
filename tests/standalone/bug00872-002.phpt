--TEST--
Test for PHP-872: Driver needs to prevent \0 in collection names.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);

$faulty = 'fo' . chr(0) . 'o';

try {
	$c = $m->selectDb(dbname())->$faulty;
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}

try {
	$c = $m->selectDb(dbname())->selectCollection( $faulty );
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}

try {
	$c = $m->selectCollection( dbname(), $faulty );
} catch( MongoException $e ) {
	var_dump($e->getCode());
	var_dump($e->getMessage());
}
?>
--EXPECTF--
int(2)
string(39) "MongoDB::__construct(): invalid name fo"
int(2)
string(39) "MongoDB::__construct(): invalid name fo"
int(2)
string(39) "MongoDB::__construct(): invalid name fo"
