--TEST--
Test for PHP-647: Queries should not be sent to recovering secondaries
--SKIPIF--
<?php echo "skip Manual test until it gets fixed"; ?>
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--INI--
mongo.is_master_interval=1
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo();
$m->setReadPreference(MongoClient::RP_SECONDARY_PREFERRED);

$explain = $m->selectCollection(dbname(), 'test')->find()->explain();
var_dump($explain['server'] === "$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT");

set_maintenance("$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT", true);

// Wait a full isMaster cycle so the driver acknowledges the new RS state
sleep(2 * ini_get('mongo.is_master_interval'));

$explain = $m->selectCollection(dbname(), 'test')->find()->explain();
var_dump($explain['server'] === "$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT");

set_maintenance("$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT", false);

--EXPECT--
bool(true)
bool(true)
