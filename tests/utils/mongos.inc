<?php
require_once "tests/utils/server.inc";
exit("skip not for travis");

try {
    $server = new MongoShellServer;
    $server->makeShard(2);
    $server->close();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}



