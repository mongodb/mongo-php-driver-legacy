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
 * A connection point between the Mongo database and PHP.
 * 
 * This class is used to initiate a connection and for high-level commands.
 * A typical use is:
 * <pre>
 *   $m = new Mongo(); // connect
 *   $db = $m->selectDatabase(); // get a database object
 * </pre>
 * 
 * @category DB
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class Mongo
{

    var $connection = null;

    /** 
     * Creates a new database connection.
     *
     * @param string $host       the host name
     * @param int    $port       the db port
     * @param bool   $persistent if the connection should be persistent
     */
    public function __construct($host = NULL, $port = NULL, $persistent = false) 
    {
        $this->host = $host ? $host : get_cfg_var("mongo.default_host");
        $this->host = $host ? $host : MONGO_DEFAULT_HOST;
        $this->port = $port ? $port : get_cfg_var("mongo.default_port");
        $this->port = $port ? $port : MONGO_DEFAULT_PORT;
        
        $auto_reconnect = MongoUtil::getConfig("mongo.auto_reconnect");
        $addr           = "$this";

        if ($persistent) {
          $username = "";
          $password = "";
          $lazy     = false;
          $this->connection = mongo_pconnect($addr, $username, $password, $auto_reconnect, $lazy);
        } else {
          $this->connection = mongo_connect($addr, $auto_reconnect);
        }

        if (!$this->connection ) {
            trigger_error("couldn't connect to mongo", E_USER_WARNING);
            $this->connected = false;
        }
        $this->connected = true;
    }

    /**
     * String representation of this connection.
     *
     * @return string hostname and port for this connection
     */
    public function __toString() 
    {
        return $this->host . ":" . $this->port;
    }

    /** 
     * Gets a database.
     *
     * @param string $dbname the database name
     *
     * @return MongoDB a new db object
     */
    public function selectDB( $dbname = null ) 
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
     * @param string         $collection the collection name
     *
     * @return MongoCollection a new collection object
     */
    public function selectCollection( $db = null, $collection = null ) 
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
     * @param MongoDB $db the database to drop
     *
     * @return array db response
     */
    public function dropDB( MongoDB $db ) 
    {
        return $db->drop();
    }

    /**
     * Repairs and compacts a database.
     *
     * @param MongoDB $db                    the database to repair
     * @param bool    $preserve_cloned_files if cloned files should be kept if the repair fails
     * @param bool    $backup_original_files if original files should be backed up
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

define("MONGO_DEFAULT_HOST", "localhost");
define("MONGO_DEFAULT_PORT", "27017");

require_once "mongo_auth.php";
require_once "mongo_db.php";
require_once "mongo_collection.php";
require_once "mongo_cursor.php";
require_once "mongo_util.php";
require_once "gridfs.php";

?>
