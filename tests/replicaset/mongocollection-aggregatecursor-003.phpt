--TEST--
MongoCollection::aggregateCursor (aggregate on secondary)
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
$command = array(
	array( '$limit' => 2 )
);

$rs = MongoShellServer::getReplicasetInfo();
$m = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$d = $m->selectDB($dbname);
$d->setWriteConcern(4); // there are four data carrying nodes
$d->cursorcmd->drop();

for ($i = 0; $i < 10; $i++) {
	$d->cursorcmd->insert(array('article_id' => $i));
}

// ==== begin tests, RP on MongoClient
$m->setReadPreference(MongoClient::RP_SECONDARY);
$d = $m->selectDB($dbname);
$c = $d->cursorcmd;

$r = $c->aggregateCursor( $command );

$r->rewind();
$info = $r->info();
echo $info['connection_type_desc'], "\n";

$m->setReadPreference(MongoClient::RP_PRIMARY);
// ==== RP on MongoDB
$d = $m->selectDB($dbname);
$d->setReadPreference(MongoClient::RP_SECONDARY);
$c = $d->cursorcmd;

$r = $c->aggregateCursor( $command );

$r->rewind();
$info = $r->info();
echo $info['connection_type_desc'], "\n";

// ==== RP on MongoCollection
$d = $m->selectDB($dbname);
$c = $d->cursorcmd;
$c->setReadPreference(MongoClient::RP_SECONDARY);

$r = $c->aggregateCursor( $command );

$r->rewind();
$info = $r->info();
echo $info['connection_type_desc'], "\n";
?>
--EXPECT--
SECONDARY
SECONDARY
SECONDARY
