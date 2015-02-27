--TEST--
Test for PHP-1395: cursor reuse (2x hasNext/getNext, with limit)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_getmore($server, $info) {
	echo "Issuing getmore\n";
}

$showQuery = false;
function log_query($server, $info) {
	global $showQuery;

	if ($showQuery) {
		echo "Issuing query\n";
	}
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
$showQuery = true;
$cur = $c->find(array(), array('_id' => 1))->limit(2);
$c = 0; while($cur->hasNext()) { $cur->getNext(); $c++; } var_dump($c);
$cur->reset();
$c = 0; while($cur->hasNext()) { $cur->getNext(); $c++; } var_dump($c);

?>
--EXPECTF--
Issuing query
int(2)
Issuing query
int(2)
