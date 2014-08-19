--TEST--
Test for PHP-1102: php driver throws exception when connect string contains an unresolvable hostname (2)
--SKIPIF--
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cfg = MongoShellServer::getReplicaSetInfo();
$opts = array('replicaSet' => $cfg['rsname']);

printLogs(MongoLog::ALL, MongoLog::WARNING, "/blacklisted/");

$hosts = "invalid-hostname.example.com:27018";
try {
    $mc = new MongoClient($hosts, $opts);
} catch(MongoConnectionException $e) {
    do {
        echo $e->getMessage(), "\n";
    } while($e = $e->getPrevious());
}
echo "All good\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
No candidate servers found
MongoClient::__construct(): php_network_getaddresses: getaddrinfo failed: %s
All good
===DONE===
