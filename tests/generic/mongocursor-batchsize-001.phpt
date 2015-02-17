--TEST--
MongoCursor::batchSize() after iteration start
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";
$dsn = MongoShellServer::getStandaloneInfo();

function log_getmore($server, $cursor_options)
{
	echo "\n", __METHOD__, "\n";
}

$ctx = stream_context_create(
	array(
		"mongodb" => array( "log_getmore" => "log_getmore",)
	)
);

$m = new MongoClient($dsn, array(), array("context" => $ctx));

$d = $m->selectDB(dbname());
$d->batchsize->drop();

$p = str_repeat("0123456789", 128);

for ($i = 0; $i < 32; $i++) {
	$d->batchsize->insert( array( 'article_id' => $i, 'pad' => $p ) );
}

$cursor = $d->batchsize->find( array( 'article_id' => array( '$gte' => 2 ) ) );

$cursor->batchSize(4);
while($r = $cursor->getNext()) {
	echo $r['article_id'], ' ';
	if ($r['article_id'] == 7) {
		$cursor->batchSize(3);
	}
	if ($r['article_id'] == 14) {
		$cursor->batchSize(5);
	}
}

?>
--EXPECTF--
2 3 4 5 
log_getmore
6 7 8 9 
log_getmore
10 11 12 
log_getmore
13 14 15 
log_getmore
16 17 18 19 20 
log_getmore
21 22 23 24 25 
log_getmore
26 27 28 29 30 
log_getmore
31
