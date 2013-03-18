--TEST--
Test for PHP-611: Segfault when no candidate servers found.
--SKIPIF--
<?php if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request'); ?>
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$hostname = hostname();
$opts = array(
    "readPreference"     => MongoClient::RP_PRIMARY_PREFERRED,
    "readPreferenceTags" => "dc:no;dc:eu;",
    "replicaSet"         => rsname(),
);
try {
    $m = new MongoClient($hostname, $opts);
} catch(MongoConnectionException $e) {
    echo $e->getMessage(), "\n";
}
echo "I'm alive\n";
?>
--EXPECT--
No candidate servers found
I'm alive
