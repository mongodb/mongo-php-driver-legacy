<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->getReplicaSetConfig();
    if (isset($needs)) {
        $serverversion = $server->getServerVersion("REPLICASET");
        if (version_compare($serverversion, $needs, "lt")) {
            throw new Exception("needs MongoDB $needs, but is running $serverversion");
        }
    }
    $server->close();
} catch(Exception $e) {
    exit("skip " . $e->getMessage());
}

