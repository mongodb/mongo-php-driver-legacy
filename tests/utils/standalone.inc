<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->getStandaloneConfig();
    if (isset($needs)) {
        $op = isset($needsOp) ? $needsOp : 'gt';
        $serverversion = $server->getServerVersion("STANDALONE");
        if (version_compare($needs, $serverversion, $op)) {
            throw new Exception("Needs MongoDB $op $needs, but is running $serverversion");
        }
    }
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}

