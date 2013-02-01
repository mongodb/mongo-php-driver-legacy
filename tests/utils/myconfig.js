var replTest ;
var standaloneTest ;
var shardTest ;
var bridgeTest ;
function initRS(servers, port) {
    if (replTest) {
        return;
    }
    replTest = new ReplSetTest( {name: "REPLICASET", nodes: servers ? servers : 3, "startPort": port ? port : 28000 } );
    replTest.startSet({"nojournal" : "", "nopreallocj": "", "quiet": "", "logpath" : "/tmp/php-mongodb-driver-logs-rs", "logappend": ""})
    cfg = replTest.getReplSetConfig()
    cfg.members[0].priority = 42
    replTest.initiate(cfg)
    /*
    for(i=1; i<=servers; i++) {
        replTest.add();
    }
    */
    replTest.awaitReplication()
}
function initStandalone(port) {
    if (standaloneTest) {
        return;
    }
    standaloneTest = startMongodTest(port);
    assert.soon( function() {
        try {
            conn = new Mongo("127.0.0.1:" + port);
            return true;
        } catch( e ) {
            printjson( e )
        }
        standaloneTest = null;
        return false;
    }, "unable to connect to mongo program on port " + port, 600 * 1000);

}
function initShard() {
    if (shardTest) {
        return;
    }
    shardTest = new ShardingTest( {name: "SHARDING", shards: 2, rs: {nodes: 3, "logpath" : "/tmp/php-mongodb-driver-logs-shard-rs", "logappend": "" }, numReplicas: 2, nopreallocj: true, mongos: 2, other: { mongosOptions: {"logpath" : "/tmp/php-mongodb-driver-logs-shard", "logappend": ""}}});
    ReplSetTest.awaitRSClientHosts( shardTest.s, shardTest.rs0.getSecondaries(), { ok : true, secondary : true });
    ReplSetTest.awaitRSClientHosts( shardTest.s, shardTest.rs1.getSecondaries(), { ok : true, secondary : true });
    ReplSetTest.awaitRSClientHosts( shardTest.s, shardTest.rs1.getPrimary(), { ok : true, ismaster : true });
}
function initBridge(port, delay) {
    if (bridgeTest) {
        return;
    }
    bridgeTest = startMongoProgram( "mongobridge", "--port", allocatePorts(1)[0], "--dest", "localhost:" + port, "--delay", delay ? delay : 300 );
}
function getStandaloneConfig() {
    if (standaloneTest) {
        return standaloneTest.host;
    }
    return null;
}
function getReplicaSetConfig() {
    return replTest.getReplSetConfig()
}
function getBridgeConfig() {
    return bridgeTest.host;
}

function getShardConfig() {
    return [shardTest.s0.host,shardTest.s1.host];
}

function killMaster() {
    replTest.stopMaster();
}
function killRS() {
    replTest.stopSet();
}

function restartMaster() {
    replTest.start(0, {remember: true}, true)
    conn = replTest.liveNodes.slaves[1]
    ReplSetTest.awaitRSClientHosts(conn, replTest, replTest.nodes)
}
