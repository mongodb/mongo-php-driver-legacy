<?php
require_once "tests/utils/server.inc";
exit("skip not for travis");

try {
    $server = new MongoShellServer;
    $server->makeReplicaset(4, 30300, dirname(__FILE__) . "/keyFile");
    $server->close();
} catch(Exception $e) {
    exit("skip " . $e->getMessage());
}

