<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->getBridgeConfig();
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}


