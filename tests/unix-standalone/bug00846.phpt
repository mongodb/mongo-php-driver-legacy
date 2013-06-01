--TEST--
Test for PHP-846: Connecting to standalone server over Unix Domain sockets
--SKIPIF--
<?php require_once "tests/utils/unix-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

MongoLog::setLevel(MongoLog::INFO);
MongoLog::setModule(MongoLog::PARSE);
$host = MongoShellServer::getUnixStandaloneInfo();
try {
    $mc = new MongoClient($host);
    echo "ok\n";
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage());
}

?>
--EXPECTF--
Notice: PARSE   INFO: Parsing /tmp/mongodb-%d.sock in %s on line %d

Notice: PARSE   INFO: - Found node: /tmp/mongodb-%d.sock:0 in %s on line %d

Notice: PARSE   INFO: - Connection type: STANDALONE in %s on line %d
ok

