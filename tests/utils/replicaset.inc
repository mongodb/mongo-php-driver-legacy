<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->getReplicaSetConfig();
    $server->close();
} catch(Exception $e) {
    exit("skip " . $e->getMessage());
}

