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

require_once "Mongo/Util.php";
require_once "Mongo/Auth.php";

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

    const LOG_OFF = 0;
    const LOG_W   = 1;
    const LOG_R   = 2;
    const LOG_RW  = 3;
    
    const TRACE_OFF  = 0;
    const TRACE_SOME = 1;
    const TRACE_ON   = 2;


    /**
     * Logs in to a given database.
     *
     * @param string $username  username
     * @param string $password  password
     * @param bool   $plaintext in plaintext, vs. encrypted
     *
     * @return bool if login was successful
     */
    public function login($username, $password, $plaintext=true) 
    {
        return parent::login(MongoUtil::ADMIN, $username, $password, $plaintext);
    }

    /** 
     * Lists all of the databases.
     *
     * @return array each database with its size and name
     */
    public function listDBs() 
    {
        $data   = array(MongoUtil::LIST_DATABASES => 1);
        $result = $this->db->command($data);
        if ($result['ok']) {
            return $result[ "databases" ];
        } else {
            return $result;
        }
    }

    /**
     * Shuts down the database.
     *
     * @return array database response
     */
    public function shutdown() 
    {
        return $this->db->command(array(MongoUtil::SHUTDOWN => 1 ));
    }

    /**
     * Turns logging on/off.
     *
     * @param int $level logging level
     *
     * @return array database response
     */
    public function setLogging($level ) 
    {
        return $this->db->command(array(MongoUtil::LOGGING => (int)$level ));
    }

    /**
     * Sets tracing level.
     *
     * @param int $level trace level
     *
     * @return array database response
     */
    public function setTracing($level ) 
    {
        return $this->db->command(array(MongoUtil::TRACING => (int)$level ));
    }

    /**
     * Sets only the query tracing level.
     *
     * @param int $level trace level
     *
     * @return array database response
     */
    public function setQueryTracing($level ) 
    {
        return $this->db->command(array(MongoUtil::QUERY_TRACING => (int)$level));
    }

}

