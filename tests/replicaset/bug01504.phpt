--TEST--
Test for PHP-1504: Segfault when connecting to non-auth replica set with credentials
--DESCRIPTION--
This test requires that the replica set not have an arbiter, as drivers cannot
authenticate against it and an arbiter from a different replica set with the
same name is prone to being considered as a viable candidate for another set.
This may be related to SERVER-5479.

To create the replica set without an arbiter, ensure that the configuration in
make-servers.php has an odd number of nodes (e.g. remove the fourth node). This
will keep initRS() in myconfig.js from adding an arbiter, since it only does so
if the number of configured nodes is even.
--SKIPIF--
skip Manual test
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::RS, MongoLog::FINE, '/skipping/');

$rs = MongoShellServer::getReplicasetInfo();

$dsnWithAuth = sprintf('mongodb://some:password@%s/?replicaSet=%s', implode(',', $rs['hosts']), $rs['rsname']);
$dsnWithoutAuth = sprintf('mongodb://%s/?replicaSet=%s', implode(',', $rs['hosts']), $rs['rsname']);

for ($i = 0; $i < 2; $i++) {
    try {
        printf("Connecting to: %s\n", $dsnWithAuth);
        $m = new MongoClient($dsnWithAuth);
    } catch (\MongoException $e) {
        printf("Auth failed, connecting to: %s\n", $dsnWithoutAuth);
        $m = new MongoClient($dsnWithoutAuth);
    }
}

?>
==DONE==
--EXPECTF--
Connecting to: mongodb://some:password@%s/?replicaSet=%s
Auth failed, connecting to: mongodb://%s/?replicaSet=%s
Connecting to: mongodb://some:password@%s/?replicaSet=%s
- skipping '%s', database didn't match (NULL vs 'admin')
- skipping '%s', database didn't match (NULL vs 'admin')
- skipping '%s', database didn't match (NULL vs 'admin')
Auth failed, connecting to: mongodb://%s/?replicaSet=%s
==DONE==
