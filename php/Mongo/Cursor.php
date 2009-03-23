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

require_once "Mongo/CursorException.php";

/**
 * Result object for database query.
 *
 * The database is not actually queried until next() or hasNext()
 * is called.  Before the database is queried, commands can be strung
 * together, as in:
 * <pre>
 *   $cursor = $collection->find()->limit( 10 );
 *   // database has not yet been queried, so more search options can be added:
 *   $cursor = $cursor->sort( array( "a" => 1 ) );
 *
 *   echo $cursor->next(); 
 *   // now database has been queried and more options cannot be added
 * </pre>
 * 
 * PHP version 5 
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoCursor
{

    protected $_connection = null;

    protected $_cursor           = null;
    protected $_startedIterating = false;

    protected $_query  = null;
    protected $_fields = null;
    protected $_limit  = 0;
    protected $_skip   = 0;
    protected $_ns     = null;

    /**
     * Create a new cursor.
     * 
     * @param resource $conn   database connection
     * @param string   $ns     db namespace
     * @param array    $query  database query
     * @param array    $fields fields to return
     */
    public function __construct($conn = null, 
                                $ns = null, 
                                $query = array(), 
                                $fields = array()) 
    {
        $this->_connection = $conn;
        $this->_ns         = $ns;
        $this->_query      = $query;
        $this->_fields     = $fields;
        $this->_sort       = array();
        $this->_hint       = array();
    }

    /**
     * Return the next object to which this cursor points, and advance the cursor.
     *
     * @return array the next object
     */
    public function next() 
    {
        if (!$this->_startedIterating) {
            $this->_doQuery();
            $this->_startedIterating = true;
        }

        return mongo_next($this->_cursor);
    }

    /**
     * Checks if there are any more elements in this cursor.
     *
     * @return bool if there is another element
     */
    public function hasNext() 
    {
        if (!$this->_startedIterating) {
            $this->_doQuery();
            $this->_startedIterating = true;
        }

        return mongo_has_next($this->_cursor);
    }

    /**
     * Limits the number of results returned.
     *
     * @param int $num the number of results to return
     *
     * @return MongoCursor this cursor
     *
     * @throws MongoCursorException if iteration phase has started
     */
    public function limit($num) 
    {
        if ($this->_startedIterating) {
            throw new MongoCursorException("cannot modify cursor after beginning iteration.");
        }
        $this->_limit = -1 * (int)$num;
        return $this;
    }

    /**
     * Limits the number of results returned in the initial database response.
     * This cannot be used in conjunction with limit(), whichever earlier will
     * be ignored:
     * <pre>
     * // 20 returned
     * $cursor1 = $collection->find()->softLimit(10)->limit(20); 
     * // all documents returned, 10 sent to client initially
     * $cursor2 = $collection->find()->limit(20)->softLimit(10);
     * </pre>
     *
     * @param int $num the number of results to return
     *
     * @return MongoCursor this cursor
     *
     * @throws MongoCursorException if iteration phase has started
     */
    public function softLimit($num) 
    {
        if ($this->_startedIterating) {
            throw new MongoCursorException("cannot modify cursor after beginning iteration.");
        }
        $this->_limit = (int)$num;
        return $this;
    }

    /**
     * Skips a number of results.
     *
     * @param int $num the number of results to skip
     *
     * @return MongoCursor this cursor
     *
     * @throws MongoCursorException if iteration phase has started
     */
    public function skip($num) 
    {
        if ($this->_startedIterating) {
            throw new MongoCursorException("cannot modify cursor after beginning iteration.");
        }
        $this->_skip = (int)$num;
        return $this;
    }

    /**
     * Sorts the results by given fields.
     *
     * @param array $fields the fields by which to sort
     *
     * @return MongoCursor this cursor
     *
     * @throws MongoCursorException if iteration phase has started
     */
    public function sort($fields) 
    {
        if ($this->_startedIterating) {
            throw new MongoCursorException("cannot modify cursor after beginning iteration.");
        }
        $this->_sort = $fields;
        return $this;
    }

    /**
     * Gives the database a hint about the query.
     *
     * @param array $key_pattern indexes to use for the query
     *
     * @return MongoCursor this cursor
     *
     * @throws MongoCursorException if iteration phase has started
     */
    public function hint($key_pattern) 
    {
        if ($this->_startedIterating) {
            throw new MongoCursorException("cannot modify cursor after beginning iteration.");
        }
        $this->_hint = $key_pattern;
        return $this;
    }

    /**
     * Counts the number of records returned by this query.
     *
     * @return int the number of records
     *
     * @throws MongoException if count fails
     */
    public function count() 
    {
        $db   = substr($this->_ns, 0, strpos($this->_ns, "."));
        $coll = substr($this->_ns, strpos($this->_ns, ".")+1);

        $cmd = array("count" => $coll);
        if ($this->_query) {
            $cmd[ "query" ] = $this->_query;
        }

        $result = MongoUtil::dbCommand($this->_connection, $cmd, $db);
        if ($result) {
            return $result[ "n" ];
        }
        throw new MongoException("count failed");
    }

    /**
     * Execute the query and set the cursor resource.
     *
     * @return void 
     */
    protected function _doQuery() 
    {
        $this->_cursor = mongo_query($this->_connection, 
                                     $this->_ns, 
                                     $this->_query, 
                                     (int)$this->_skip, 
                                     (int)$this->_limit, 
                                     $this->_sort, 
                                     $this->_fields, 
                                     $this->_hint);
    }

    /**
     * Special cursor used by GridFS functions.
     * 
     * @param resource $cursor a database cursor
     *
     * @return MongoCursor a new cursor
     */
    public static function getGridFSCursor($cursor) 
    {
        $c                    = new MongoCursor();
        $c->_cursor           = $cursor;
        $c->_startedIterating = true;
        return $c;
    }
}

?>
