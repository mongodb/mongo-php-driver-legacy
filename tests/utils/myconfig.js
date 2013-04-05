var replTest;
var replTestAuth;
var standaloneTest;
var standaloneTestAuth;
var shardTest;
var shardTestAuth;
var bridgeTest;

/**
 * Initialize a replica set.
 *
 * If the number of servers is even, an additional member will be added to the
 * set as an arbiter process.
 *
 * If the server parameter is an array, its length will determine the number of
 * nodes in the replica set. Additionally, each element is expected to be an
 * object with configuration options for the node (e.g. tags).
 *
 * @param int|array servers Number of servers, or array of server config options
 * @param int       port    Starting port for replica set nodes
 * @param string    keyFile Path to private key file (activates auth if present)
 * @param object    root    Object with username/password fields for "admin" DB user
 * @param object    user    Object with username/password fields for "test" DB user
 * @return ReplSetTest
 */
function initRS(servers, port, keyFile, root, user) {
    servers = typeof servers !== 'undefined' ? servers : 3;
    port = typeof port !== 'undefined' ? port : 28000;

    // Do not reitinialize a replica set
    if ((keyFile && replTestAuth) || (!keyFile && replTest)) {
        return (keyFile ? replTestAuth : replTest);
    }

    // Save serverOpts before extracting servers from the array's length
    if (Array.isArray(servers)) {
        var serverOpts = servers;
        servers = servers.length;
    }

    var addArbiter = (servers % 2 == 0);

    var testOpts = {
        "name": keyFile ? "REPLICASET-AUTH" : "REPLICASET",
        "nodes": servers + (addArbiter ? 1 : 0),
        "startPort": port
    };

    var nodeOpts = {
        "nojournal" : "",
        "nopreallocj": "",
        "quiet": "",
        "logpath" : "/dev/null",
        "oplogSize": 10
    };

    if (keyFile) {
        nodeOpts.keyFile = keyFile;
    }

    var retval = new ReplSetTest(testOpts);
    retval.startSet(nodeOpts);
    var cfg = retval.getReplSetConfig();

    // Give the first member highest priority
    cfg.members[0].priority = 42;

    if (addArbiter) {
        cfg.members[servers].arbiterOnly = true;
    }

    // Apply server configuration options, if available
    if (typeof serverOpts !== 'undefined') {
        for (var i = 0; i < servers; i++) {
            cfg.members[i] = Object.extend(cfg.members[i], serverOpts[i]);
        }
    }

    retval.initiate(cfg);
    retval.awaitReplication();

    if (keyFile) {
        retval.getMaster().getDB("admin").addUser(root.username, root.password);
        retval.getMaster().getDB("admin").auth(root.username, root.password);
        retval.getMaster().getDB("test").addUser(user.username, user.password);
        replTestAuth = retval;
    } else {
        replTest = retval;
    }

    return retval;
}

/**
 * Initialize a standalone server.
 *
 * If the number of servers is even, an additional member will be added to the
 * set as an arbiter process.
 *
 * @param int     port Server port
 * @param boolean auth Whether to enable authentication
 * @param object  root Object with username/password fields for "admin" DB user
 * @param object  user Object with username/password fields for "test" DB user
 * @return Mongo
 */
function initStandalone(port, auth, root, user) {
    port = typeof port !== 'undefined' ? port : 27000;

    // Do not reitinialize a standalone server
    if ((auth && standaloneTestAuth) || (!auth && standaloneTest)) {
        return (auth ? standaloneTestAuth : standaloneTest);
    }

    var opts = {
        "dbpath" : MongoRunner.dataPath + port
    };

    if (auth) {
        opts.auth = "";
    }

    var retval = startMongodTest(port, false, false, opts);
    retval.port = port;

    assert.soon(function() {
        try {
            conn = new Mongo("127.0.0.1:" + port);
            return true;
        } catch(e) {
            printjson(e);
        }
        return false;
    }, "unable to connect to mongo program on port " + port, 600 * 1000);

    if (auth) {
        retval.getDB("admin").addUser(root.username, root.password);
        retval.getDB("admin").auth(root.username, root.password);
        retval.getDB("test").addUser(user.username, user.password);
        standaloneTestAuth = retval;
    } else {
        standaloneTest = retval;
    }

    return retval;
}

/**
 * Initialize a sharded cluster.
 *
 * @return ShardingTest
 */
function initShard() {
    // Do not reitinialize a sharded cluster
    if (shardTest) {
        return shardTest;
    }

    shardTest = new ShardingTest({
        "name": "SHARDING",
        "shards": 2,
        "rs": {
            "nodes": 3,
            "logpath": "/dev/null",
            "oplogSize": 10
        },
        "numReplicas": 2,
        "nopreallocj": true,
        "mongos": 2,
        "other": {
            "mongosOptions": {
                "logpath": "/dev/null"
            }
        }
    });

    ReplSetTest.awaitRSClientHosts(shardTest.s, shardTest.rs0.getSecondaries(), { ok : true, secondary : true });
    ReplSetTest.awaitRSClientHosts(shardTest.s, shardTest.rs1.getSecondaries(), { ok : true, secondary : true });
    ReplSetTest.awaitRSClientHosts(shardTest.s, shardTest.rs1.getPrimary(), { ok : true, ismaster : true });

    return shardTest;
}

/**
 * Initialize a mongobridge.
 *
 * @return Mongo
 */
function initBridge(port, delay) {
    delay = typeof delay !== 'undefined' ? delay : 300;

    // Do not reinitialize a mongobridge
    if (bridgeTest) {
        return bridgeTest;
    }

    var bridgePort = allocatePorts(1)[0];
    bridgeTest = startMongoProgram("mongobridge", "--port", bridgePort, "--dest", "localhost:" + port, "--delay", delay);
    bridgeTest.port = bridgePort;

    return bridgeTest;
}

/**
 * Get standalone host.
 *
 * @param boolean auth
 * @return string Host/port of standalone server
 */
function getStandaloneConfig(auth) {
    if (auth) {
        return standaloneTestAuth ? standaloneTestAuth.host : null;
    }
    return standaloneTest ? standaloneTest.host : null;
}

/**
 * Get replica set configuration.
 *
 * @param boolean auth
 * @return object
 */
function getReplicaSetConfig(auth) {
    if (auth) {
        return replTestAuth ? replTestAuth.getReplSetConfig() : null;
    }
    return replTest ? replTest.getReplSetConfig() : null;
}

/**
 * Get mongobridge host.
 *
 * @return string Host/port of mongobridge
 */
function getBridgeConfig() {
    return bridgeTest ? bridgeTest.host : null;
}

/**
 * Get sharded cluster hosts.
 *
 * @return array Array of host/port strings for each mongos
 */
function getShardConfig() {
    return shardTest ? [shardTest.s0.host, shardTest.s1.host] : null;
}

/**
 * Stop replica set primary.
 */
function killMaster() {
    replTest.stopMaster();
}

/**
 * Stop replica set.
 */
function killRS() {
    replTest.stopSet();
}

/**
 * Restart replica set primary.
 */
function restartMaster() {
    replTest.start(0, {remember: true}, true);
    var conn = replTest.liveNodes.slaves[1];
    ReplSetTest.awaitRSClientHosts(conn, replTest, replTest.nodes)
}

/**
 * Set MongoRunner dataDir and dataPath properties.
 *
 * @param string path
 */
function setDBDIR(path) {
    MongoRunner.dataDir = path;
    MongoRunner.dataPath = path;
}

/**
 * Add a user to standaloneTestAuth.
 *
 * @see _addUser()
 */
function addStandaloneUser(loginUser, newUser) {
    return _addUser(standaloneTestAuth, loginUser, newUser);
}

/**
 * Add a user to replTestAuth.
 *
 * @see _addUser()
 */
function addReplicasetUser(loginUser, newUser) {
    return _addUser(replTestAuth.getMaster(), loginUser, newUser);
}

/**
 * Add a user to the database.
 *
 * The loginUser will be used to authenticate before adding newUser.
 *
 * @param Mongo  conn      Database connection
 * @param object loginUser Object with db/username/password fields
 * @param object newUser   Object with db/username/password fields
 * @return object
 */
function _addUser(conn, loginUser, newUser) {
    conn.getDB(loginUser.db).auth(loginUser.username, loginUser.password);
    return conn.getDB(newUser.db).addUser(newUser.username, newUser.password);
}

/**
 * Shutdown all test servers, replica sets, shard clusters, and bridges.
 *
 * @param function callback Callback to execute after shutdown completes
 */
function shutdownEverything(callback) {
    if (typeof standaloneTest !== "undefined") {
        MongoRunner.stopMongod(standaloneTest.port);
    }
    if (typeof standaloneTestAuth !== "undefined") {
        MongoRunner.stopMongod(standaloneTestAuth.port);
    }
    if (typeof bridgeTest !== "undefined") {
        MongoRunner.stopMongod(bridgeTest.port);
    }
    if (typeof replTest !== "undefined") {
        replTest.stopSet();
    }
    if (typeof replTestAuth !== "undefined") {
        replTestAuth.stopSet();
    }
    if (typeof shardTest !== "undefined") {
        shardTest.stop();
    }
    if (typeof callback === "function") {
        callback();
    }
}
