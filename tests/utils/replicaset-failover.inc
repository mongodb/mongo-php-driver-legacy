<?php
if (getenv('SKIP_SLOW_TESTS')) die('skip slow tests excluded by request');
exit("skip not for travis");


require_once "tests/utils/server.inc";

try {
    $server = new MongoShellServer;
    $server->makeReplicaset(4, 28000);
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}



