--TEST--
Test for PHP-999: mapReduce with inline output should respect read preference
--SKIPIF--
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php require_once 'tests/utils/replicaset.inc' ?>
--FILE--
<?php
require_once 'tests/utils/server.inc';

MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);
MongoLog::setCallback(function($module, $level, $msg) {
    if (strpos($msg, "command supports") !== false) {
        echo $msg, "\n";
        return;
    }
    if (strpos($msg, "forcing") !== false) {
        echo $msg, "\n";
        return;
    }
});

$rs = MongoShellServer::getReplicasetInfo();
$mc = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']));
$mc->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED);

$db = $mc->selectDB(dbname());
$db->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED);

echo "Testing with 'mapReduce' command string\n";

// Should be forced to primary ("inline" is a collection name)
$db->command(array(
    'mapReduce' => 'bug00999',
    'map' => new MongoCode('function(){}'),
    'reduce' => new MongoCode('function(key, value){ return 1; }'),
    'out' => 'inline',
));

// Should support read preference
$db->command(array(
    'mapReduce' => 'bug00999',
    'map' => new MongoCode('function(){}'),
    'reduce' => new MongoCode('function(key, value){}'),
    'out' => array('inline' => 1),
));

// Although the proper command is "mapReduce", the server also accepts "mapreduce", so we'll test that, too.

echo "Testing with 'mapreduce' command string\n";

// Should be forced to primary ("inline" is a collection name)
$db->command(array(
    'mapreduce' => 'bug00999',
    'map' => new MongoCode('function(){}'),
    'reduce' => new MongoCode('function(key, value){ return 1; }'),
    'out' => 'inline',
));

// Should support read preference
$db->command(array(
    'mapreduce' => 'bug00999',
    'map' => new MongoCode('function(){}'),
    'reduce' => new MongoCode('function(key, value){}'),
    'out' => array('inline' => 1),
));

?>
--EXPECTF--
Testing with 'mapReduce' command string
forcing primary for command
command supports Read Preferences
Testing with 'mapreduce' command string
forcing primary for command
command supports Read Preferences
