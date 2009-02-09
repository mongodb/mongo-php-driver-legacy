<?php

class mongo_cursor {

  var $connection = NULL;

  private $cursor = NULL;
  private $started_iterating = false;

  private $query = NULL;
  private $fields = NULL;
  private $limit = NULL;
  private $skip = NULL;

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


  public function sort( $fields ) {
    if( $this->started_iterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->sort = $fields;
    return $this;
  }

  /**
   * Execute the query and set the cursor resource.
   */
  private function do_query() {
    $q = mongo_util::obj_to_array( $this->query );
    $s = mongo_util::obj_to_array( $this->sort );
    $f = mongo_util::obj_to_array( $this->fields );

    if( !is_null( $limit ) ) {
      $this->cursor = mongo_query( $this->connection, $this->ns, $q, (int)$this->skip, (int)$this->limit, $s, $f );
    }
    else {
      $this->cursor = mongo_query( $this->connection, $this->ns, $q, (int)$this->skip, $s, $f );
    }
  }
}

?>