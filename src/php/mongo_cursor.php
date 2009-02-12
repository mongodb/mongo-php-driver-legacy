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
 */

class MongoCursor {

  var $connection = NULL;

  private $_cursor = NULL;
  private $_startedIterating = false;

  private $_query = NULL;
  private $_fields = NULL;
  private $_limit = 0;
  private $_skip = 0;
  private $_ns = NULL;

  public function __construct( $conn = NULL, $ns = NULL, $query = NULL, $skip = NULL, $limit = NULL, $fields = NULL ) {
    $this->connection = $conn;
    $this->_ns = $ns;
    $this->_query = $query;
    $this->_skip = $skip;
    $this->_limit = $limit;
    $this->_fields = $fields;
  }

  /**
   * Return the next object to which this cursor points, and advance the cursor.
   * @return array the next object
   */
  public function next() {
    if( !$this->_startedIterating ) {
      $this->doQuery();
      $this->_startedIterating = true;
    }

    return mongo_next( $this->_cursor );
  }

  /**
   * Checks if there are any more elements in this cursor.
   * @return bool if there is another element
   */
  public function hasNext() {
    if( !$this->_startedIterating ) {
      $this->doQuery();
      $this->_startedIterating = true;
    }

    return mongo_has_next( $this->_cursor );
  }

  /**
   * Limits the number of results returned.
   * @param int $num the number of results to return
   * @return MongoCursor this cursor
   */
  public function limit( $num ) {
    if( $this->_startedIterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->_limit = (int)$num;
    return $this;
  }

  /**
   * Skips a number of results.
   * @param int $num the number of results to skip
   * @return MongoCursor this cursor
   */
  public function skip( $num) {
    if( $this->_startedIterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->_skip = (int)$num;
    return $this;
  }

  /**
   * Sorts the results by given fields.
   * @param array $fields the fields by which to sort
   * @return MongoCursor this cursor
   */
  public function sort( $fields ) {
    if( $this->_startedIterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->sort = $fields;
    return $this;
  }

  /**
   * Gives the database a hint about the query.
   * @param array $key_pattern
   * @return MongoCursor this cursor
   */
  public function hint( $key_pattern ) {
    if( $this->_startedIterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->hint = $key_pattern;
    return $this;
  }


  /**
   * Counts the number of records returned by this query.
   * @return int the number of records
   */
  public function count() {
    $db = substr( $this->_ns, 0, strpos( $this->_ns, "." ) );
    $coll = substr( $this->_ns, strpos( $this->_ns, "." )+1 );

    $cmd = array( "count" => $coll );
    if( $this->_query )
      $cmd[ "query" ] = MongoUtil::objToArray( $this->_query );

    $result = MongoUtil::dbCommand( $this->connection, $cmd, $db );
    if( $result )
      return $result[ "n" ];
    trigger_error( "count failed", E_USER_ERROR );
  }

  /**
   * Execute the query and set the cursor resource.
   */
  private function doQuery() {
    $q = MongoUtil::objToArray( $this->_query );
    $s = MongoUtil::objToArray( $this->_sort );
    $f = MongoUtil::objToArray( $this->_fields );
    $h = MongoUtil::objToArray( $this->_hint );

    $this->_cursor = mongo_query( $this->connection, $this->_ns, $q, (int)$this->_skip, ((int)$this->_limit) * -1, $s, $f, $h );
  }

  public static function getGridfsCursor( $cursor ) {
    $c = new MongoCursor();
    $c->_cursor = $cursor;
    $c->_startedIterating = true;
    return $c;
  }
}

?>
