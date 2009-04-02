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

require_once "Mongo/Cursor.php";
require_once "Mongo/GridFS/File.php";

/**
 * Cursor for database file results.
 *
 * This is used by GridFS->find() to return results.
 *
 * Example:
 * <pre>
 *   $grid = $db->getGridFS();
 *   $gcursor = $grid->find(array('filename' => new MongoRegex('/squid/')));
 *   foreach ($gcursor as $filename => $file) {
 *       $file->write($filename);
 *   }
 * </pre>
 * If the collection contained files squid.jpg, mysquid.txt, and ceilingsquid.ico,
 * they would be written to the current directory with those names.
 * 
 * PHP version 5 
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoGridFSCursor extends MongoCursor
{

    /**
     * Create a new cursor.
     * 
     * @param resource $conn   database connection
     * @param string   $ns     db namespace
     * @param array    $query  database query
     * @param array    $fields fields to return
     */
    public function __construct(MongoGridFS $gridfs,
                                $conn = null, 
                                $ns = null, 
                                $query = array(), 
                                $fields = array()) 
    {
        $this->gridfs = $gridfs;
        parent::__construct($conn, $ns, $query, $fields);
    }


    /**
     * Return the next element.
     * 
     * @return array the next element or null
     */
    public function getNext()
    {
        $this->next();
        if ($this->current) {
          return new MongoGridFSFile($this->gridfs, $this->current);
        }
        return null;
    }

    /* **************
     *    Iterator
     * **************/

    /**
     * Returns the correct item
     *
     * @return array $obj
     */
    public function current()
    {
        return new MongoGridFSFile($this->gridfs, $this->current);
    }

    /**
     * Returns the key of the current item
     *
     * @return string The _id of the current item
     */
    public function key()
    {
        return (string)$this->current['filename'];
    }

}

?>
