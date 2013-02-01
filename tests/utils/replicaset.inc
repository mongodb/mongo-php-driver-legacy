<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->makeReplicaset(4, 28000);
    $server->close();
} catch(Exception $e) {
    exit("skip " . $e->getMessage());
}

