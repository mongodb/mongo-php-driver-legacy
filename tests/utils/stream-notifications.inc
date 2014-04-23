<?php

class MongoNotifications {
    public $reqs = array();
    public $inserts = array();


    function update($notification_code, $severity, $message, $message_code, $bytes_transferred, $bytes_max) {
        switch($notification_code) {
            case MONGO_STREAM_NOTIFY_TYPE_LOG:
                return $this->logMessage($message_code, $message);
            default:
        }
    }

    function logMessage($code, $message) {
        $data = json_decode($message, true);

        switch($code) {
            case MONGO_STREAM_NOTIFY_LOG_CMD_INSERT:
            case MONGO_STREAM_NOTIFY_LOG_CMD_UPDATE:
            case MONGO_STREAM_NOTIFY_LOG_CMD_DELETE:
                $reqid = $data["protocol_options"]["request_id"];
                $this->reqs[$reqid] = $data;
                $this->reqs[$reqid]["start"] = microtime(true);
                $this->inserts[] = $reqid;
            break;

            case MONGO_STREAM_NOTIFY_LOG_WRITE_REPLY:
                $reqid = $data["op"]["response_to"];

                $end = microtime(true);
                $total = $end - $this->reqs[$reqid]["start"];
                $data["total"] = $total;

                $this->reqs[$reqid]["response"] = $data;
                break;
        }
    }

    function getLastInsertMeta() {
        $ids = $this->getInsertRequestIDs();
        $id = array_pop($ids);
        return $this->getRequest($id);
    }
    function getInsertRequestIDs() {
        return $this->inserts;
    }
    function getRequest($n) {
        return $this->reqs[$n];
    }
    function getRequests() {
        return $this->reqs;
    }
}

/* Some Helper Functions */

/* Split a Server Hash to extract the server name from it */
function getServerName($hash) {
    list($server, $replicaset, $dbname, $pid) = explode(";", $hash);

    return $server;
}

/* Resolve the server type to a readable string */
function getServerType($type) {
    switch($type) {
        // FIXME: Do we not export these as constants?
        case 0x01: return "STANDALONE";
        case 0x02: return "PRIMARY";
        case 0x04: return "SECONDARY";
        case 0x10: return "MONGOS";
    }
}

/* Resolve Cursor (Query) Options to something meaningful */
function getCursorOptions($opts) {
    // FIXME: Do we not export these as constants?
    if (($opts & 0x04) == 0x04) {
        return "slaveOkay";
    }
    return "Primary Only";
}



function dump_writes($mn) {
    // Fetch our requests from our logger
    $reqs = $mn->getRequests();
    foreach($reqs as $request) {
        if ($request["response"]["op"]["response_to"] != $request["protocol_options"]["request_id"]) {
            throw Exception("Oboy, this shouldn't happen!");
        }
        /*
        printf("Server name: %s, type: %s, repsponse flags: %d\n",
            getServerName($request["server"]["hash"]),
            getServerType($request["server"]["type"]),
            $request["response"]["reply"]["flags"]
        );
        printf("Server execution time+network latency: %.8f sec\n\n", $request["response"]["total"]);
         */
        var_dump($request["write_options"], $request["protocol_options"]);
        echo "\n\n\n";
    }
}

// vim: set ft=php
