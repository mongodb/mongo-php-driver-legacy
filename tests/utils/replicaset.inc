<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->getReplicaSetConfig();
    if (isset($needs)) {
        $op = isset($needsOp) ? $needsOp : 'gt';
        $serverversion = $server->getServerVersion("REPLICASET");
        if (version_compare($needs, $serverversion, $op)) {
            throw new Exception("Needs MongoDB $op $needs, but is running $serverversion");
        }
    }
    $server->close();
} catch(Exception $e) {
    exit("skip " . $e->getMessage());
}

