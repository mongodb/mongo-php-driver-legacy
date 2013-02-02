<?php
require_once "tests/utils/server.inc";
exit("skip not for travis");

try {
    $server = new MongoShellServer;
    $server->makeStandalone(30100, true);
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}


