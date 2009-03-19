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
 * Gets an admin database connection.
 * 
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoAdmin extends MongoAuth
{

    /**
     * Creates a new admin session.
     * 
     * @param string $username  username
     * @param string $password  password
     * @param bool   $plaintext in plaintext, vs. encrypted
     * @param string $server    database server
     */
    public function __construct($username, 
                                $password, 
                                $plaintext=true, 
                                $server=null) 
    {
        parent::__construct("admin", $username, $password, $plaintext, $host, $port);
    }

    /** 
     * Lists all of the databases.
     *
     * @return array each database with its size and name
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
                                       array(MongoUtil::$QUERY_TRACING => 
                                             (int)$level), 
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
