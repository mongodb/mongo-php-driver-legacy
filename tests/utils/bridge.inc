<?php
require_once "tests/utils/server.inc";
exit("skip not for travis");

try {
    $server = new MongoShellServer;
    $server->makeStandalone(30000);
    $server->makeBridge(30000, 1000);
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}


