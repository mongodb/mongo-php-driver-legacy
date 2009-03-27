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

    protected $connection = null;

    protected $cursor           = null;
    protected $startedIterating = false;

    protected $query  = null;
    protected $fields = null;
    protected $limit  = 0;
    protected $skip   = 0;
    protected $ns     = null;

    private static $_ERR_CURSOR_MOD = 
      "cannot modify cursor after beginning iteration.";

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
        $this->connection = $conn;
        $this->ns         = $ns;
        $this->query      = array("query" => $query);
        $this->fields     = $fields;
        $this->sort       = array();
        $this->hint       = array();
    }

    /**
     * Return the next object to which this cursor points, and advance the cursor.
     *
     * @return array the next object
     */
    public function next() 
    {
        if (!$this->startedIterating) {
            $this->doQuery();
            $this->startedIterating = true;
        }

        return mongo_next($this->cursor);
    }

    /**
     * Checks if there are any more elements in this cursor.
     *
     * @return bool if there is another element
     */
    public function hasNext() 
    {
        if (!$this->startedIterating) {
            $this->doQuery();
            $this->startedIterating = true;
        }

        return mongo_has_next($this->cursor);
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
        if ($this->startedIterating) {
            throw new MongoCursorException(MongoCursor::$_ERR_CURSOR_MOD);
        }
        $this->limit = -1 * (int)$num;
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
        if ($this->startedIterating) {
            throw new MongoCursorException(MongoCursor::$_ERR_CURSOR_MOD);
        }
        $this->limit = (int)$num;
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
        if ($this->startedIterating) {
            throw new MongoCursorException(MongoCursor::$_ERR_CURSOR_MOD);
        }
        $this->skip = (int)$num;
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
        if ($this->startedIterating) {
            throw new MongoCursorException(MongoCursor::$_ERR_CURSOR_MOD);
        }
        $this->query['orderby'] = $fields;
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
        if ($this->startedIterating) {
            throw new MongoCursorException(MongoCursor::$_ERR_CURSOR_MOD);
        }
        $this->query['$hint'] = $key_pattern;
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
        $db   = substr($this->ns, 0, strpos($this->ns, "."));
        $coll = substr($this->ns, strpos($this->ns, ".")+1);

        $cmd = array("count" => $coll, "query" => $this->query["query"]);

        $result = MongoUtil::dbCommand($this->connection, $cmd, $db);
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
    protected function doQuery() 
    {
        $this->cursor = mongo_query($this->connection, 
                                     $this->ns, 
                                     $this->query, 
                                     (int)$this->skip, 
                                     (int)$this->limit, 
                                     $this->fields);
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
        $c                   = new MongoCursor();
        $c->cursor           = $cursor;
        $c->startedIterating = true;
        return $c;
    }
}

?>
