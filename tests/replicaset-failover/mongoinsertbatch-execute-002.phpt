--TEST--
Test for PHP-1048: Segfault killing cursors after failover
--SKIPIF--
<?php require_once "tests/utils/replicaset-failover.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
ini_set("mongo.ping_interval", 60);
ini_set("mongo.is_master_interval", 60);

$s = new MongoShellServer;
$cfg = $s->getReplicaSetConfig();
$opts = array('replicaSet' => $cfg['rsname']);
$mc = new MongoClient($cfg["dsn"], $opts);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

for($i=0; $i<506; $i++) {
    $collection->insert(array("document" => $i));
}

$c = $collection->find();
$i = 0;
try {
    foreach($c as $v) {
        if (++$i == 100) {
            echo "Killing master after $i\n";
            $s->killMaster();
            echo "Killing done\n";
            $s->restartMaster();
            echo "Restarting done\n";

            try {
                $retval = $collection->insert(array("one", "more", "document"));
            } catch(Exception $e) {
                echo get_class($e), ": ", $e->getMessage(), "\n";
            }
        }
    }
} catch(Exception $e){
    echo get_class($e), ": ", $e->getMessage(), "\n";
}
echo "Test completed\n";

?>
===DONE===
<?php exit(0); ?>
--CLEAN--
<?php require_once "tests/utils/fix-master.inc"; ?>
--EXPECTF--
Killing master after 100
Killing done
Restarting done
MongoCursorException: localhost:30200: Remote server has closed the connection
MongoConnectionException: the connection has been terminated, and this cursor is dead
Test completed
===DONE===

