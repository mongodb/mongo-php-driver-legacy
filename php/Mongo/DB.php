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
 * Instances of this class are used to interact with a database.
 * To get a database:
 * <pre>
 *   $m = new Mongo(); // connect
 *   $db = $m->selectDatabase(); // get a database object
 * </pre>
 * 
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoDB
{

    public $connection = null;
    public $name       = null;

    /**
     * Creates a new database.
     * 
     * @param Mongo  $conn connection
     * @param string $name database name
     */
    public function __construct(Mongo $conn, $name) 
    {
        $this->connection = $conn->connection;
        $this->name       = $name;
    }

    /**
     * The name of this database.
     *
     * @return string this database's name
     */
    public function __toString() 
    {
        return $this->name;
    }

    /**
     * Fetches toolkit for dealing with files stored in this database.
     *
     * @param string $prefix name of the file collection
     *
     * @return MongoGridFS a new gridfs object for this database
     */
    public function getGridFS($prefix = "fs") 
    {
        return new MongoGridFS($this, $prefix);
    }

    /**
     * Gets this database's profiling level.
     *
     * @return int the profiling level
     */
    public function getProfilingLevel() 
    {
        $data = array(MongoUtil::$PROFILE => -1);
        $x    = MongoUtil::dbCommand($this->connection, $data, $this->name);
        if ($x[ "ok" ] == 1) {
            return $x[ "was" ];
        } else {
            return false;
        }
    }

    /**
     * Sets this database's profiling level.
     *
     * @param int $level profiling level
     *
     * @return int the old profiling level
     */
    public function setProfilingLevel($level) 
    {
        $data = array(MongoUtil::$PROFILE => (int)$level);
        $x    = MongoUtil::dbCommand($this->connection, $data, $this->name);
        if ($x[ "ok" ] == 1) {
            return $x[ "was" ];
        }
        return false;
    }

    /**
     * Drops this database.
     * Returns on success:
     * <pre>
     * array(1) {
     *   ["ok"]=>
     *   float(1)
     * }
     * </pre>
     *
     * Returns on error:
     * <pre>
     * array(2) {
     *   ['errmsg']=>
     *   string(...) "...",
     *   ['ok']=>
     *   float(...)
     * }
     * </pre>
     *
     * @return array the database response
     */
    public function drop() 
    {
        $data = array(MongoUtil::$DROP_DATABASE => 1);
        return MongoUtil::dbCommand($this->connection, $data, "$this");
    }

    /**
     * Repairs and compacts this database.
     *
     * @param bool $preserve_cloned_files if cloned files should be kept if the 
     *     repair fails
     * @param bool $backup_original_files if original files should be backed up
     *
     * @return array db response
     */
    function repair($preserve_cloned_files = false, $backup_original_files = false) 
    {
        $data = array(MongoUtil::$REPAIR_DATABASE => 1,
                      "preserveClonedFilesOnFailure" => (bool)$preserve_cloned_files,
                      "backupOriginalFiles" => $backup_original_files);
        return MongoUtil::dbCommand($this->connection, $data, $this->name);
    }


    /** 
     * Gets a collection.
     *
     * @param string $name the name of the collection
     *
     * @return MongoCollection the collection
     */
    public function selectCollection($name) 
    {
        return new MongoCollection($this, $name);
    }

    /** 
     * Creates a collection.
     *
     * @param string $name   the name of the collection
     * @param bool   $capped if the collection should be a fixed size
     * @param int    $size   if the collection is fixed size, its size in bytes
     * @param int    $max    if the collection is fixed size, the maximum 
     *     number of elements to store in the collection
     *
     * @return Collection a collection object representing the new collection
     */
    public function createCollection($name, $capped = false, $size = 0, $max = 0) 
    {
        $data = array(MongoUtil::$CREATE_COLLECTION => $name);
        if ($capped && $size) {
            $data[ "capped" ] = true;
            $data[ "size" ]   = $size;
            if ($max) {
                $data[ "max" ] = $max;
            }
        }

        MongoUtil::dbCommand($this->connection, $data, $this->name);
        return new MongoCollection($this, $name);
    }

    /**
     * Drops a collection.
     *
     * @param string|MongoCollection $coll collection to drop
     *
     * @return array the db response
     */
    public function dropCollection($coll) 
    {
        if ($coll instanceof MongoCollection) {
            return $coll->drop();
        } else {
            return $this->selectCollection($coll)->drop();
        }
    }

    /**
     * Get a list of collections in this database.
     *
     * @return array a list of collection names
     */
    public function listCollections() 
    {
        $nss     = $this->selectCollection("system.namespaces")->find();
        $ns_list = array();
        while ($nss->hasNext()) {
            $ns = $nss->next();
            $ns = $ns[ "name" ];
            if (strpos($ns, "\$") > 0) {
                continue;
            }
            array_push($ns_list, $ns);
        }
        return $ns_list;
    }

    /**
     * Gets information from the database about cursors.
     * Returns an array of the form:
     * <pre>
     * array(3) {
     *   ["byLocation_size"]=>
     *   int(...)
     *   ["clientCursors_size"]=>
     *   int(...)
     *   ["ok"]=>
     *   float(1)
     * }
     * </pre>
     *
     * @return array information about the cursor
     */
    public function getCursorInfo()
    {
        $a = array(MongoUtil::$INDEX_INFO => 1);
        return MongoUtil::dbCommand($this->connection, $a, "$this");
    }

    /**
     * Creates a database reference.
     *
     * @param string $ns  the collection the db ref will point to
     * @param mixed  $obj array or _id to refer to
     *
     * @return array the db ref, or null if the object was not a database object 
     *               or _id
     */
    public function createDBRef($ns, $obj) 
    {
        if (is_array($obj) &&
            array_key_exists('_id', $obj)) {
            return MongoDBRef::create("$ns", $obj["_id"]);
        } else if ($obj instanceof MongoId) {
            return MongoDBRef::create("$ns", $obj);
        }
        return null;
    }

    /**
     * Gets the value a db ref points to.
     *
     * @param array $ref db ref to check
     *
     * @return array the object or null
     */
    public function getDBRef($ref) 
    {
        if (MongoDBRef::isRef($ref)) {
            return MongoDBRef::get($this, $ref);
        }
        return null;
    }

    /**
     * Runs code on the database.
     * If successful, returns an array of the form:
     * <pre>
     * array(2) {
     *   ["retval"]=>
     *   ...
     *   ["ok"]=>
     *   float(1)
     * }
     * </pre>
     * If there was an error, returns an array of the form:
     * <pre>
     * array(3) {
     *   ["errno"]=>
     *   float(...)
     *   ["errmsg"]=>
     *   string(...) "..."
     *   ["ok"]=>
     *   float(0)
     * }
     * </pre>
     * The "errno" field may not be returned if there is no 
     * corresponding error number.
     *
     * @param string $code string of code
     * @param array  $args arguments to pass to first parameter
     *
     * @return the database response to the executed code
     */
    public function execute($code, $args=array()) 
    {
        $a = array('$eval' => $code, "args" => $args);
        return MongoUtil::dbCommand($this->connection,
                                     $a,
                                     "$this");
    }

}

define("MONGO_PROFILING_OFF", 0);
define("MONGO_PROFILING_SLOW", 1);
define("MONGO_PROFILING_ON", 2);


?>
