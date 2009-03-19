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
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @version  CVS: 000000
 * @link     http://www.mongodb.org
 */

/**
 * A connection point between the Mongo database and PHP.
 * 
 * This class is used to initiate a connection and for high-level commands.
 * A typical use is:
 * <pre>
 *   $m = new Mongo(); // connect
 *   $db = $m->selectDatabase(); // get a database object
 * </pre>
 * 
 * @category Database
 * @package  DB_Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class Mongo
{
    const MONGO_DEFAULT_HOST = "localhost";
    const MONGO_DEFAULT_PORT = "27017";

    public $connection = null;

    private $_server;
    private $_paired;
    private $_persistent;

    /** 
     * Creates a new database connection.
     *
     * @param string $server     the address and port of the database server
     * @param bool   $connect    if the connection should be made
     * @param bool   $persistent if the connection should be persistent
     * @param bool   $paired     if the connection is with paired database servers
     *
     * @throws MongoConnectionException if it could not connect
     */
    public function __construct($server=null, 
                                $connect=true, 
                                $persistent = false, 
                                $paired=false) 
    {
        if ($connect) {
            $this->_connectUtil($server, "", "", $persistent, $paired);
        } else {
            $this->_server     = $server;
            $this->_persistent = $persistent;
            $this->_paired     = $paired;
            $this->connected   = false;
        }
    }

    /**
     * Connect to a database server.
     *
     * @param string $server server address
     *
     * @return bool if a connection was successfully made
     *
     * @throws MongoConnectionException if it could not connect
     */
    public function connect($server=null) 
    {
        return _connectUtil($server, "", "", false, false);
    }

    /**
     * Connect to paired database servers.
     *
     * @param string $server1 first server address
     * @param string $server2 second server address
     *
     * @return bool if a connection was successfully made
     *
     * @throws MongoConnectionException if it could not connect
     */
    public function pairConnect($server1, $server2) 
    {
        return _connectUtil("${server1},${server2}", "", "", false, true);
    }

    /**
     * Create a persistent connection to a database server.
     *
     * @param string $server   server address 
     * @param string $username username
     * @param string $password password
     *
     * @return bool if a connection was successfully made
     *
     * @throws MongoConnectionException if it could not connect
     */
    public function persistConnect($server, $username="", $password="") 
    {
        return _connectUtil($server, $username, $password, true, false);
    }

    /**
     * Create a persistent connection to paired database servers.
     *
     * @param string $server1  first server address
     * @param string $server2  second server address
     * @param string $username username
     * @param string $password password
     *
     * @return bool if a connection was successfully made
     *
     * @throws MongoConnectionException if it could not connect
     */
    public function pairPersistConnect($server1, 
                                       $server2, 
                                       $username="", 
                                       $password="") 
    {
        return _connectUtil("${server1},${server2}", 
                            $username, 
                            $password, 
                            true, 
                            true);
    }

    /**
     * Actually connect.
     *
     * @param string $server     database address
     * @param string $username   username
     * @param string $password   password
     * @param bool   $persistent if connection is persistent
     * @param bool   $paired     if connection is paired
     *
     * @return bool if connection was made
     */ 
    private function _connectUtil($server, 
                                  $username, 
                                  $password, 
                                  $persistent, 
                                  $paired)
    {
        $this->_paired     = $paired;
        $this->_persistent = $persistent;

        // close any current connections
        if ($this->connected) {
            $this->close();
            $this->connected = false;
        }

        if (!$server) {
            $host   = get_cfg_var("mongo.default_host");
            $host   = $host ? $host : Mongo::MONGO_DEFAULT_HOST;
            $port   = get_cfg_var("mongo.default_port");
            $port   = $port ? $port : Mongo::MONGO_DEFAULT_PORT;
            $server = "${host}:${port}";
        }
        $this->_server = $server;

        $lazy             = false;
        $this->connection = mongo_connect((string)$server,
                                          (string)$username, 
                                          (string)$password, 
                                          (bool)$persistent, 
                                          (bool)$paired, 
                                          (bool)$lazy);

        if (!$this->connection) {
            $this->connected = false;
            throw new MongoConnectionException("Could not connect to $server");
        } else {
            $this->connected = true;
        }
        return $this->connected;
    }

    /**
     * String representation of this connection.
     *
     * @return string hostname and port for this connection
     */
    public function __toString() 
    {
        return $this->_server;
    }

    /** 
     * Gets a database.
     *
     * @param string $dbname the database name
     *
     * @return MongoDB a new db object
     */
    public function selectDB($dbname = null) 
    {
        if ($dbname == null || $dbname == "" ) {
            trigger_error("Invalid database name.", E_USER_WARNING);
            return false;
        }
        return new MongoDB($this, $dbname);
    }

    /** 
     * Gets a database collection.
     * This allows you to get a collection directly.
     * <pre>
     *   $m = new Mongo();
     *   $c = $m->selectCollection("foo", "bar.baz");
     * </pre>
     *
     * @param string|MongoDB $db         the database name
     * @param string          $collection the collection name
     *
     * @return MongoCollection a new collection object
     */
    public function selectCollection( $db, $collection ) 
    {
        if ($db == null || $db == "" ) {
            trigger_error("Invalid database name.", E_USER_WARNING);
            return false;
        }
        if ($collection == null || $collection == "" ) {
            trigger_error("Invalid collection name.", E_USER_WARNING);
            return false;
        }
        if (is_string($db)) {
            return new MongoCollection(new MongoDB($this, $db), $collection);
        } else {
            return new MongoCollection($db, $collection);
        }
    }

    /**
     * Drops a database.
     *
     * @param string|MongoDB $db the database to drop
     *
     * @see MongoDB::drop()
     *
     * @return array db response 
     */
    public function dropDB($db) 
    {
        if ($db instanceof MongoDB) {
            return $db->drop();
        } else {
            return $this->selectDB("$db")->drop();
        }
    }

    /**
     * Repairs and compacts a database.
     *
     * @param MongoDB $db                    the database to repair
     * @param bool     $preserve_cloned_files if cloned files should be kept if the 
     *                                        repair fails
     * @param bool     $backup_original_files if original files should be backed up
     *
     * @return array db response
     */
    public function repairDB( MongoDB $db, 
                              $preserve_cloned_files = false, 
                              $backup_original_files = false ) 
    {
        return $db->repair($preserve_cloned_files, $backup_original_files);
    }

    /**
     * Check if there was an error on the most recent db operation performed.
     *
     * @return string the error, if there was one, or null
     */
    public function lastError() 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$LAST_ERROR => 1 ), 
                                       MongoUtil::$ADMIN);
        return $result[ "err" ];
    }

    /**
     * Checks for the last error thrown during a database operation.
     *
     * @return array the error and the number of operations ago it occured
     */
    public function prevError() 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$PREV_ERROR => 1 ), 
                                       MongoUtil::$ADMIN);
        unset($result[ "ok" ]);
        return $result;
    }

    /**
     * Clears any flagged errors on the connection.
     *
     * @return bool if successful
     */
    public function resetError() 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$RESET_ERROR => 1 ), 
                                       MongoUtil::$ADMIN);
        return (bool)$result[ "ok" ];
    }

    /**
     * Creates a database error.
     *
     * @return array a notification that an error occured
     */
    public function forceError() 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array(MongoUtil::$FORCE_ERROR => 1 ), 
                                       MongoUtil::$ADMIN);
        unset($result[ "ok" ]);
        return $result;
    }

    /**
     * Checks which server is master.
     *
     * @return array info about the pair
     */
    public function masterInfo() 
    {
        if (!$this->_paired) {
            return array("errmsg" => "non-paired connection",
                         "ok" => (float)0);
        }
        return MongoUtil::dbCommand($this->connection, 
                                     array("ismaster" => 1), 
                                     MongoUtil::$ADMIN);
    }

    /**
     * Closes this database connection.
     *
     * @return bool if the connection was successfully closed
     */
    public function close() 
    {
        mongo_close($this->connection);
        $this->connected = false;
    }
}



require_once "Mongo/Exception.php";
require_once "Mongo/ConnectionException.php";
require_once "Mongo/CursorException.php";
require_once "Mongo/Auth.php";
require_once "Mongo/Admin.php";
require_once "Mongo/DB.php";
require_once "Mongo/DB/Ref.php";
require_once "Mongo/Collection.php";
require_once "Mongo/Cursor.php";
require_once "Mongo/Util.php";
require_once "Mongo/GridFS.php";
require_once "Mongo/GridFS/File.php";

?>
