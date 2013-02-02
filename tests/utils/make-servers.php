<?php
require_once "tests/utils/server.inc";
mkdir("/data/db", 0777, true);

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
$retry = true;
do {
    try {
        $server = new MongoShellServer;


        echo "Making Standalone.... ";
        t();
        $standalone = microtime(true);
        $server->makeStandalone(30000);
        sprintf("DONE (%.2f secs)\n", t());
        $str = $server->getStandaloneConfig();
        // Travis fail fast
        if (strlen($str) == 5) {
            var_dump($str);
            exit(2);
        }


        exit(0);
        echo "Making Authenticated Standalone.... ";
        t();
        $standalone = microtime(true);
        $server->makeStandalone(30100, true);
        sprintf("DONE (%.2f secs)\n", t());
        var_dump($server->getStandaloneConfig(true));


        echo "Making Bridge.... ";
        echo "doen't work in travis, mongobridge is not installed by default\n";
        /*
        $sc = $server->getStandaloneConfig();
        list($shost, $sport) = explode(":", trim($sc));
        $server->makeBridge($sport, 1000);
        printf("DONE (%.2f secs)\n", t());
        var_dump($server->getBridgeConfig());
         */


        echo "Making shard.... ";
        $server->makeShard(2);
        printf("DONE (%.2f secs)\n", t());
        var_dump($server->getShardConfig());

        echo "Making ReplicaSet.... ";
        $server->makeReplicaset(4, 30200);
        printf("DONE (%.2f secs)\n", t());
        var_dump($server->getReplicaSetConfig());

        echo "Making Authenticated ReplicaSet.... ";
        $server->makeReplicaset(4, 30300, dirname(__FILE__) . "/keyFile");
        printf("DONE (%.2f secs)\n", t());
        var_dump($server->getReplicaSetConfig(true));

        $server->close();
    } catch(Exception $e) {
        if ($retry) {
            $retry = false;
            $pid = pcntl_fork();
            if ($pid > 0) {
                sleep(1);
                continue;
            }
            if (!$pid) {
                posix_setsid();
                require_once dirname(__FILE__) . "/daemon.php";
                exit(0);
            }
        }

        echo $e->getMessage(), "\n";
        exit(1);
    }
    echo "We have liftoff \n";
    break;
} while(true);

