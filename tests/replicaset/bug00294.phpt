--TEST--
Test for PHP-294: Workaround for sending commands to secondaries
--SKIPIF--
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

function isSecondary($mc, $server) {
    list($host, $port) = explode(':', $server, 2);

    foreach ($mc->getHosts() as $member) {
        if ($host == $member['host'] && $port == $member['port']) {
            return 2 == $member['state'];
        }
    }

    return false;
}

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));

$coll = $mc->selectCollection('phpunit', 'php294');
$coll->drop();
$coll->insert(array('x' => 1), array('w' => 'majority'));

$cmd = $mc->selectCollection('phpunit', '$cmd');
$count = $cmd->findOne(array('count' => 'php294'));
var_dump($count['ok'] && 1 == $count['n']);

$explain = $cmd->find(array('count' => 'php294'))->explain();
var_dump(isSecondary($mc, $explain['server']));

$cmd->setReadPreference(MongoClient::RP_SECONDARY);
$explain = $cmd->find(array('count' => 'php294'))->explain();
var_dump(isSecondary($mc, $explain['server']));

?>
--EXPECTF--
bool(true)
bool(false)
bool(true)
