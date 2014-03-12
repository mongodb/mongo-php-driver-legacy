--TEST--
MongoCommandCursor (read preferences)
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";
$dbname = dbname();
/*
MongoLog::setLevel( MongoLog::ALL );
MongoLog::setModule( MongoLog::ALL );
MongoLog::setCallback( function( $a, $b, $c ) { echo $c, "\n"; } );
*/
$rs = MongoShellServer::getReplicasetInfo();
$m = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$d = $m->selectDB($dbname);
$d->cursorcmd->drop();

for ($i = 0; $i < 10; $i++) {
	$d->cursorcmd->insert(array('article_id' => $i));
}

$m->setReadPreference(MongoClient::RP_SECONDARY);

$r = new MongoCommandCursor(
	$m, "{$dbname}.cursorcmd", 
	array(
		'aggregate' => 'cursorcmd', 
		'pipeline' => array( 
			array( '$limit' => 2 ) 
		), 
	)
);

$r->rewind();
$info = $r->info();
echo $info['connection_type_desc'], "\n";
?>
--EXPECT--
SECONDARY
