var replTest ;
var replTestAuth ;
var standaloneTest ;
var standaloneTestAuth ;
var shardTest ;
var shardTestAuth ;
var bridgeTest ;

function initRS(servers, port, keyFile, root, user) {
    if ((keyFile && replTestAuth) || (!keyFile && replTest)) {
        return;
    }
    testopts = {name: "REPLICASET", nodes: servers ? servers : 3, "startPort": port ? port : 28000 };
    opts = {"nojournal" : "", "nopreallocj": "", "quiet": "", "logpath" : "/dev/null", "oplogSize": "10"};


    if (keyFile) {
        testopts.name = "REPLICASET-AUTH";
        opts.keyFile = keyFile;
    }

    retval = new ReplSetTest( testopts );

    retval.startSet(opts)
    cfg = retval.getReplSetConfig()
    cfg.members[0].priority = 42
    retval.initiate(cfg)
    retval.awaitReplication()

    if (keyFile) {
        retval.getMaster().getDB("admin").addUser(root.username, root.password)
        retval.getMaster().getDB("admin").auth(root.username, root.password)
        retval.getMaster().getDB("test").addUser(user.username, user.password)
        replTestAuth = retval;
    } else {
        replTest = retval;
    }
    return retval;
}
function initStandalone(port,auth,root,user) {
    if ((auth && standaloneTestAuth) || (!auth && standaloneTest)) {
        return;
    }

    port = port | 27000;
    opts = {}
    if (auth) {
        opts.auth = "";
    }
    retval = startMongodTest(port, false, false, opts);
    assert.soon( function() {
        try {
            conn = new Mongo("127.0.0.1:" + port);
            return true;
        } catch( e ) {
            printjson( e )
        }
        return false;
    }, "unable to connect to mongo program on port " + port, 600 * 1000);
    if (auth) {
        retval.getDB("admin").addUser(root.username, root.password)
        retval.getDB("admin").auth(root.username, root.password)
        retval.getDB("test").addUser(user.username, user.password)
        standaloneTestAuth = retval;
    } else {
        standaloneTest = retval;
    }
    return retval;

}
function initShard() {
    if (shardTest) {
        return;
    }
    shardTest = new ShardingTest( {name: "SHARDING", shards: 2, rs: {nodes: 3, "logpath" : "/dev/null", "oplogSize": "10" }, numReplicas: 2, nopreallocj: true, mongos: 2, other: { mongosOptions: {"logpath" : "/dev/null"}}});
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




function getStandaloneConfig(auth) {
    if (auth) {
        return standaloneTestAuth ? standaloneTestAuth.host : null;
    }
    return standaloneTest ? standaloneTest.host : null;
}
function getReplicaSetConfig(auth) {
    return auth ? replTestAuth.getReplSetConfig() : replTest.getReplSetConfig()
}
function getBridgeConfig() {
    return bridgeTest ? bridgeTest.host : null;
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
