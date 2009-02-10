<?php

class mongo_cursor {

  var $connection = NULL;

  private $cursor = NULL;
  private $started_iterating = false;

  private $query = NULL;
  private $fields = NULL;
  private $limit = 0;
  private $skip = 0;

  public function __construct( $conn, $ns, $query = NULL, $skip = NULL, $limit = NULL, $fields = NULL ) {
    $this->connection = $conn;
    $this->ns = $ns;
    $this->query = $query;
    $this->skip = $skip;
    $this->limit = $limit;
    $this->fields = $fields;
  }

  /**
   * Return the next object to which this cursor points, and advance the cursor.
   * @return array the next object
   */
  public function next() {
    if( !$this->started_iterating ) {
      $this->do_query();
      $this->started_iterating = true;
    }

    return mongo_next( $this->cursor );
  }

  /**
   * Checks if there are any more elements in this cursor.
   * @return bool if there is another element
   */
  public function has_next() {
    if( !$this->started_iterating ) {
      $this->do_query();
      $this->started_iterating = true;
    }

    return mongo_has_next( $this->cursor );
  }

  /**
   * Limits the number of results returned.
   * @param int $num the number of results to return
   * @return mongo_cursor this cursor
   */
  public function limit( $num ) {
    if( $this->started_iterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->limit = (int)$num;
    return $this;
  }

  /**
   * Skips a number of results.
   * @param int $num the number of results to skip
   * @return mongo_cursor this cursor
   */
  public function skip( $num) {
    if( $this->started_iterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->skip = (int)$num;
    return $this;
  }

  /**
   * Sorts the results by given fields.
   * @param array $fields the fields by which to sort
   * @return mongo_cursor this cursor
   */
  public function sort( $fields ) {
    if( $this->started_iterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->sort = $fields;
    return $this;
  }

  /**
   * Gives the database a hint about the query.
   * @param array $key_pattern
   * @return mongo_cursor this cursor
   */
  public function hint( $key_pattern ) {
    if( $this->started_iterating ) {
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
    $db = substr( $this->ns, 0, strpos( $this->ns, "." ) );
    $coll = substr( $this->ns, strpos( $this->ns, "." )+1 );

    $cmd = array( "count" => $coll );
    if( $this->query )
      $cmd[ "query" ] = mongo_util::obj_to_array( $this->query );

    $result = mongo_util::db_command( $this->connection, $cmd, $db );
    if( $result )
      return $result[ "n" ];
    trigger_error( "count failed", E_USER_ERROR );
  }

  /**
   * Execute the query and set the cursor resource.
   */
  private function do_query() {
    $q = mongo_util::obj_to_array( $this->query );
    $s = mongo_util::obj_to_array( $this->sort );
    $f = mongo_util::obj_to_array( $this->fields );
    $h = mongo_util::obj_to_array( $this->hint );

    $this->cursor = mongo_query( $this->connection, $this->ns, $q, (int)$this->skip, (int)$this->limit, $s, $f, $h );
  }
}

?>
