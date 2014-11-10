<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $cfg = $server->getStandaloneConfig(true);
    if (isset($needsmin)) {
        $needs = $needsmin;
    }
    if (isset($needs)) {
        $needsOp = isset($needsOp) ? $needsOp : '>=';
        $serverversion = $server->getServerVersion("STANDALONE-AUTH");
        if ( ! version_compare($serverversion, $needs, $needsOp)) {
            throw new Exception("Needs MongoDB $needsOp $needs, but is running $serverversion");
        }
    }
    if (isset($needsmax)) {
        $serverversion = $server->getServerVersion("STANDALONE-AUTH");
        if ( ! version_compare($serverversion, $needs, "<")) {
            throw new Exception("Needs MongoDB less then $needsmax, but is running $serverversion");
        }
    }
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}


