--TEST--
MongoCommandCursor (read preferences)
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
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
$document = $m->selectDb(dbname())->command(
	array(
		'aggregate' => 'cursorcmd', 
		'pipeline' => array( 
			array( '$limit' => 2 ) 
		), 
        "cursor" => (object)array(),
	)
);

$r = new MongoCommandCursor(
	$m, $document["hash"], $document["cursor"]
);

$r->rewind();
$info = $r->info();
echo $info['connection_type_desc'], "\n";
?>
--EXPECT--
SECONDARY
