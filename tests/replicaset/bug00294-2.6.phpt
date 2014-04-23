--TEST--
Test for PHP-294: Workaround for sending commands to secondaries
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php $needs = "2.5.5"; ?>
<?php if (version_compare(PHP_VERSION, "5.4.0", "le")) { exit("skip This test requires PHP version PHP5.4+"); }?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';
require_once "tests/utils/stream-notifications.inc";

$mn = new MongoNotifications;
$ctx = stream_context_create(
    array(),
    array(
        "notification" => array($mn, "update")
    )
);



$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']), array("context" => $ctx));

$coll = $mc->selectCollection('phpunit', 'php294');
$coll->drop();
$coll->insert(array('x' => 1), array('w' => 'majority'));
$meta = $mn->getLastInsertMeta();
var_dump($meta["server"]["type"] == 2);

$cmd = $mc->selectCollection('phpunit', '$cmd');
$count = $cmd->findOne(array('count' => 'php294'));
var_dump($count['ok'] && 1 == $count['n']);

$explain = $cmd->find(array('count' => 'php294'))->limit(1)->explain();

$cmd->setReadPreference(MongoClient::RP_SECONDARY);
$explain = $cmd->find(array('count' => 'php294'))->limit(1)->explain();
var_dump($explain);

?>
--EXPECTF--
bool(true)
bool(true)
array(2) {
  ["n"]=>
  float(1)
  ["ok"]=>
  float(1)
}
