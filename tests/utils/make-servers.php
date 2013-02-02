<?php
/*
 * The only bootup specific type of servers set
 *      MONGO_SERVER_[SERVER_TYPE]=yes
 * in your environment before running this script.
 *
 * To bootup all exception a specific type of server set
 *      SKIP_MONGO_SERVER_[SERVER_TYPE]=yes
 */
require_once "tests/utils/server.inc";
require "tests/utils/cfg.inc";

function t() {
    static $last;
    if ($last) {
        $current = microtime(true);
        $retval = $current - $last;
        $last = $current;
        return $retval;
    }
    $last = microtime(true);
}
function makeServer($SERVERS, $server, $bit) {
    echo "Making " . $SERVERS[$bit] . ".. ";
    t();
    switch($bit) {
    case STANDALONE:
        $server->makeStandalone(30000);
        $dsn = $server->getStandaloneConfig();
        break;
    case STANDALONE_AUTH:
        $server->makeStandalone(30100, true);
        $dsn = $server->getStandaloneConfig(true);
        break;
    case STANDALONE_BRIDGE:
        $sc = $server->getStandaloneConfig();
        list($shost, $sport) = explode(":", trim($sc));
        $server->makeBridge($sport, 1000);
        $dsn = $server->getBridgeConfig();
        break;
    case MONGOS:
        $server->makeShard(2);
        $cfg = $server->getShardConfig();
        $dsn = join(",", $cfg);
        break;
    case REPLICASET:
        $server->makeReplicaset(4, 30200);
        $cfg = $server->getReplicaSetConfig();
        $dsn = $cfg["dsn"];
        break;
    case REPLICASET_AUTH:
        $retval = $server->makeReplicaset(4, 30300, dirname(__FILE__) . "/keyFile");
        var_dump($retval);
        $cfg = $server->getReplicaSetConfig(true);
        var_dump($cfg);
        $dsn = $cfg["dsn"];
        break;
    default:
        var_dump("No idea what to do about $bit");
        exit(32);
    }
    printf("DONE (%.2f secs): %s\n", t(), $dsn);
}

$SERVERS = array(
    "STANDALONE"        => 0x01,
    "STANDALONE_AUTH"   => 0x02,
    "STANDALONE_BRIDGE" => 0x04,
    "MONGOS"            => 0x08,
    "REPLICASET"        => 0x10,
    "REPLICASET_AUTH"   => 0x20,
);

$ALL_SERVERS = 0;
$BOOTSTRAP = 0;
foreach($SERVERS as $server => $bit) {
    define($server, $bit);
    $ALL_SERVERS |= $bit;
}

foreach($SERVERS as $server => $bit) {
    if (getenv("MONGO_SERVER_$server")) {
        $BOOTSTRAP |= $bit;
    }
}
if (!$BOOTSTRAP) {
    $BOOTSTRAP = $ALL_SERVERS;
    foreach($SERVERS as $server => $bit) {
        if (getenv("SKIP_MONGO_SERVER_$server")) {
            $BOOTSTRAP &= ~$bit;
        }
    }
}
function makeDaemon() {
    $pid = pcntl_fork();
    if ($pid > 0) {
        sleep(1);
        return;
    }
    if (!$pid) {
        posix_setsid();
        require_once dirname(__FILE__) . "/daemon.php";
        exit(0);
    }
}

try {
    $server = new MongoShellServer;
} catch(Exception $e) {
    makeDaemon();
    try {
        $server = new MongoShellServer;
    } catch(Exception $e)  {
        echo $e->getMessage();
        exit(2);
    }
}

foreach($SERVERS as $k => $bit) {
    if ($BOOTSTRAP & $bit) {
        makeServer(array_flip($SERVERS), $server, $bit);
    }
}

$server->close();

echo "We have liftoff \n";

