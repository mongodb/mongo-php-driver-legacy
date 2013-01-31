<?php

class MongoShellServer {
    protected $fp;
    public function __construct() {
        $fp = fsockopen("localhost", 8000, $errno, $errstr, 2);
        if (!$fp) {
            throw new Exception($errstr, $errno);
        }
        $this->fp = $fp;
    }

    public function makeReplicaset($members = 4, $port = 28000) {
        return $this->verifyWrite("initRS($members,$port)");
    }

    public function killMaster() {
        return $this->verifyWrite("killMaster()");
    }

    public function restartMaster() {
        return $this->verifyWrite("restartMaster()");
    }

    protected function verifyWrite($command) {
        fwrite($this->fp, $command . "\n");
        $md5 = md5($command);
        fwrite($this->fp, "print('$md5')\n");
        $retval = "";
        while(!feof($this->fp)) {
            $line = fgets($this->fp);
            if (trim($line) == $md5) {
                return $retval;
            }
            $retval .= $line;
        }
        return false;
    }

    public function getReplicaSetConfig() {
        $config = json_decode($this->verifyWrite("getReplicaSetConfig()"), true);
        return self::prettifyRSInfo($config);
    }

    public function close() {
        if ($this->fp) {
            fwrite($this->fp, "quit\n");
            fclose($this->fp);
        }
        $this->fp = NULL;
    }

    public function __destruct() {
        return $this->close();
    }

    public static function getReplicaSetInfo() {
        /* If we are in the context of a REDIRECTTEST we have _ENV set */
        if (isset($_ENV["REPLICASET"])) {
            $config = json_decode($_ENV["REPLICASET"], true);
        } else {
            /* Otherwise we are running this in a standalone mode and need to bootstrap the replicaset on-demand */
            $o = new self;
            $o->makeReplicaset(4, 28000);
            $config = $o->getReplicaSetConfig();
            $o->close();
            return $config;
        }
        return self::prettifyRSInfo($config);
    }

    protected static function prettifyRSInfo($config) {
        $rsname = $config["_id"];
        $hosts  = array();
        foreach($config["members"] as $k => $v) {
            $hosts[] = $v["host"];
        }

        return array(
            "dsn"    => join(",", $hosts),
            "rsname" => $rsname,
            "hosts"  => $hosts,
        );
    }
}

function mongo() {
    $config = MongoShellServer::getReplicaSetInfo();
    return new Mongo($config["dsn"], array("replicaSet" => $config["rsname"]));
}
function new_mongo() {
    $config = MongoShellServer::getReplicaSetInfo();
    return new MongoClient($config["dsn"], array("replicaSet" => $config["rsname"]));
}
function dbname() {
    return "test";
}
