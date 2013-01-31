var replTest ;
var standaloneTest ;
var shardTest ;
var bridgeTest ;
function initRS(servers, port) {
    if (replTest) {
        return;
    }
    replTest = new ReplSetTest( {name: "REPLICASET", nodes: servers ? servers : 3, "startPort": port ? port : 28000 } );
    replTest.startSet({"nojournal" : "", "nopreallocj": "", "quiet": "", "profile": "0"})
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
}
function initShard() {
    if (shardTest) {
        return;
    }
    shardTest = new ShardingTest( {name: "SHARDING", shards: 3, rs: {nodes: 3}, numReplicas: 3, nopreallocj: true, mongos: 2});
}
function initBridge(port, delay) {
    if (bridgeTest) {
        return;
    }
    bridgeTest = startMongoProgram( "mongobridge", "--port", allocatePorts(1)[0], "--dest", "localhost:" + port, "--delay", delay ? delay : 300 );
}
function getStandaloneConfig() {
    return standaloneTest.host;
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
