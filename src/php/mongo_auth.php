<?php
/**
 *  Copyright 2009 10gen, Inc.
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * PHP version 5 
 *
 * @category DB
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @version  CVS: 000000
 * @link     http://www.mongodb.org
 */


/**
 * Use <pre>getAuth()</pre> to log in for an authenticated session.
 * 
 * @category DB
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoAuth extends Mongo
{

    public $connection;
    private $db;

    /**
     * Communicates with the database to log in a user.
     * 
     * @param connection $conn     database connection
     * @param string     $db       db name
     * @param string     $username username
     * @param string     $password plaintext password
     *
     * @return array the database response
     */
    public static function getUser($conn, $db, $username, $password ) 
    {
        $ns   = $db . ".system.users";
        $user = mongo_find_one($conn, $ns, array("user" => $username));
        if (!$user) {
            return false;
        }
        $pwd = $user[ "pwd" ];

        // get the nonce
        $result = MongoUtil::dbCommand($conn, array(MongoUtil::$NONCE => 1 ), $db);
        if (!$result[ "ok" ]) {
            return false;
        }
        $nonce = $result[ "nonce" ];

        // create a digest of nonce/username/pwd
        $digest = md5($nonce . $username . $pwd);
        $data   = array(MongoUtil::$AUTHENTICATE => 1, 
                        "user" => $username, 
                        "password" => $password,
                        "nonce" => $nonce,
                        "key" => $digest);

        // send everything to the db and pray
        return MongoUtil::dbCommand($conn, $data, $db);
    }

    /**
     * Attempt to create a new authenticated session.
     *
     * @param string $host       a database connection
     * @param int    $port       a database connection
     * @param string $db         the name of the db
     * @param string $username   the username
     * @param string $password   the password
     *
     * @return MongoAuth an authenticated session or false if login was unsuccessful
     */
    public function __construct($host, $port, $db, $username, $password, $plaintext=true) 
    {
        $this->db = $db;
        if ($plaintext) {
            $hash = md5("mongo$password");
        }
        else {
            $hash = $password;
        }

        if (!$host) {
            $host = get_cfg_var("mongo.default_host");
            if (!$host ) {
                $host = MONGO_DEFAULT_HOST;
            }
        }
        if (!$port ) {
            $port = get_cfg_var("mongo.default_port");
            if (!$port ) {
                $port = MONGO_DEFAULT_PORT;
            }
        }
        $auto_reconnect = MongoUtil::getConfig("mongo.auto_reconnect");

        $addr             = "$host:$port";
        //echo "add: ". $addr."_".$username."_".$hash."<br/>";
        $this->connection = mongo_pconnect($addr, $username, $hash, $auto_reconnect, true);
        //echo "connection? |".$this->connection."|<br/>";
        if (!$this->connection ) {
            if (!$plaintext) {
                trigger_error("can't login with hash password", E_USER_WARNING);
                $this->loggedIn = false;
                return $this;
            }
            $this->connection = mongo_pconnect($addr, $username, $hash, $auto_reconnect, false);
            if (!$this->connection) {
                trigger_error("couldn't connect to mongo", E_USER_WARNING);
                $this->loggedIn = false;
                return $this;
            }
            $result = MongoAuth::getUser($this->connection, $db, $username, $password);
            if (!$result[ "ok" ]) {
                trigger_error("couldn't log in", E_USER_WARNING);
                $this->loggedIn = false;
                return $this;
            }
        }
        $this->loggedIn = true;
    }

    /**
     * Returns "Authenticated", for reasons I can't remember.
     *
     * @return string "Authenticated"
     */
    public function __toString() 
    {
        return "Authenticated";
    }

    /**
     * Ends authenticated session.
     *
     * @return boolean if successfully ended
     */
    public function logout() 
    {
        $data   = array(MongoUtil::$LOGOUT => 1);
        $result = MongoUtil::dbCommand($this->connection, $data, $this->db);

        if (!$result[ "ok" ]) {
            // trapped in the system forever
            return false;
        }

        return true;
    }
}

/**
 * Use <pre>getAuth()</pre> from the admin database to log in for an admin session.
 * 
 * @category DB
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoAdmin extends MongoAuth
{

    /**
     * Creates a new admin session.  To get a new session, call 
     * MongoAuth::getAuth() using the admin database.
     * 
     * @param connection $conn db connection
     */
    public function __construct($host, $port, $username, $password, $plaintext=true) 
    {
        $this->db = "admin";
        parent::__construct($host, $port, $this->db, $username, $password, $plaintext);
    }

    /** 
     * Lists all of the databases.
     *
     * @return Array each database with its size and name
     */
    public function listDBs() 
    {
        $data   = array(MongoUtil::$LIST_DATABASES => 1);
        $result = MongoUtil::dbCommand($this->connection, $data, $this->db);
        if ($result) {
            return $result[ "databases" ];
        } else {
            return false;
        }
    }

    /**
     * Shuts down the database.
     *
     * @return bool if the database was successfully shut down
     */
    public function shutdown() 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$SHUTDOWN => 1 ), 
                                       $this->db);
        return $result[ "ok" ];
    }

    /**
     * Turns logging on/off.
     *
     * @param int $level logging level
     *
     * @return bool if the logging level was set
     */
    public function setLogging($level ) 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$LOGGING => (int)$level ), 
                                       $this->db);
        return $result[ "ok" ];
    }

    /**
     * Sets tracing level.
     *
     * @param int $level trace level
     *
     * @return bool if the tracing level was set
     */
    public function setTracing($level ) 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$TRACING => (int)$level ), 
                                       $this->db);
        return $result[ "ok" ];
    }

    /**
     * Sets only the query tracing level.
     *
     * @param int $level trace level
     *
     * @return bool if the tracing level was set
     */
    public function setQueryTracing($level ) 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$QUERY_TRACING => (int)$level ), 
                                       $this->db);
        return $result[ "ok" ];
    }

}

define("MONGO_LOG_OFF", 0);
define("MONGO_LOG_W", 1);
define("MONGO_LOG_R", 2);
define("MONGO_LOG_RW", 3);

define("MONGO_TRACE_OFF", 0);
define("MONGO_TRACE_SOME", 1);
define("MONGO_TRACE_ON", 2);


?>
