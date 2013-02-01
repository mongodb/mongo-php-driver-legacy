<?php
require_once "tests/utils/server.inc";
require dirname(__FILE__) . "/cfg.inc";

try {
    $server = new MongoShellServer;
    $server->makeReplicaset(4, 30300, dirname(__FILE__) . "/keyFile", $SUPER_USER, $NORMAL_USER);
    $server->close();
} catch(Exception $e) {
    exit("skip " . $e->getMessage());
}

