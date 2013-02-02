<?php
require_once "tests/utils/server.inc";
exit("skip not for travis");

try {
    $server = new MongoShellServer;
    $server->makeReplicaset(4, 28000);
    $server->close();
} catch(Exception $e) {
    exit("skip " . $e->getMessage());
}

