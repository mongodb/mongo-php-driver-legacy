--TEST--
Test for PHP-1395: cursor reuse (foreach + iterator_to_array, with limit)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_getmore($server, $info) {
	echo "Issuing getmore\n";
}

function log_query($server, $info) {
	echo "Issuing query\n";
}

$ctx = stream_context_create(array(
	'mongodb' => array(
		'log_getmore' => 'log_getmore',
		'log_query' => 'log_query',
	),
));

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));

$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->save(array('_id' => 'test1'));
$c->save(array('_id' => 'test2'));
$c->save(array('_id' => 'test3'));
$c->save(array('_id' => 'test4'));
$cur = $c->find(array(), array('_id'))->limit(2);
$c = 0; foreach($cur as $dummy) { $c++; } var_dump($c);
var_dump(count(iterator_to_array($cur)));

?>
--EXPECTF--
Issuing query
Issuing query
int(2)
Issuing query
int(2)
