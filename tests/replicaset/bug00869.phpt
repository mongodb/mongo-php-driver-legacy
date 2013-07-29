--TEST--
Test for PHP-869: Primary Prefered Read Preference without matching tag should still select primary is possible
--SKIPIF--
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array(
    'replicaSet' => $rs['rsname'],
));

$show = false;

MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::RS);
MongoLog::setCallback( 'printMsgs' );

function printMsgs($a, $b, $msg)
{
	global $show;

	if (preg_match( '/^pick/', $msg)) {
		$show = true;
	}

	if (preg_match( '/added primary regardless/', $msg) || $show) {
		echo $msg, "\n";
	}
}

// Doesn't match anything
$mc->setReadPreference(MongoClient::RP_PRIMARY_PREFERRED, array(array("dc" => "sf")));
$c = $mc->selectCollection(dbname(), 'bug869');
$c->findOne();


?>
--EXPECTF--
candidate_matches_tags: added primary regardless of tags: %s:%d;REPLICASET;.;%d
pick server: the primary
- connection: type: PRIMARY, socket: %d, ping: %d, hash: %s:%d;REPLICASET;.;%d
  - tag: dc:ny
  - tag: server:0
