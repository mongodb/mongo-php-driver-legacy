--TEST--
Test for PHP-1382: hasNext() returns true, but getNext() returns NULL (with getmore)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_getmore($server, $info) {
	echo "Issuing getmore\n";
}

$ctx = stream_context_create(array(
	'mongodb' => array(
		'log_getmore' => 'log_getmore',
	),
));

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));

$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->save(array('_id' => 'test1'));
$c->save(array('_id' => 'test2'));
$c->save(array('_id' => 'test3'));
$cur = $c->find(array(), array('_id' => 1))->batchSize(2);
while ($cur->hasNext()) {
	$info = $cur->info(); echo 'a: ', @$info['at'], ' - ', @$info['numReturned'], "\n";
	$arr = $cur->getNext();
	$info = $cur->info(); echo 'b: ', @$info['at'], ' - ', @$info['numReturned'], "\n";
	var_dump($arr['_id']);
}

?>
--EXPECT--
a: 0 - 2
b: 0 - 2
string(5) "test1"
a: 0 - 2
b: 1 - 2
string(5) "test2"
Issuing getmore
a: 1 - 3
b: 2 - 3
string(5) "test3"
