<?php
require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->makeStandalone(30000);
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}


