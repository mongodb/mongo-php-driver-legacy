<?php

class mongo_collection {

  var $parent;
  var $connection;
  var $db;
  var $name = "";

  function __construct( mongo_db $db, $name ) {
    $this->parent = $db;
    $this->connection = $db->connection;
    $this->db = $db->name;
    $this->name = $name;
  }

  public function __toString() {
    return $this->db . "." . $this->name;
  }

  /**
   * Drops this collection.
   * @return array the db response
   */
  function drop() {
    $this->delete_indices();
    $data = array( mongo_util::$DROP => $this->name );
    return mongo_util::db_command( $this->connection, $data, $this->db );
  }

  /**
   * Validates this collection.
   * @param bool $scan_data only validate indices, not the base collection (optional)
   * @return array the database's evaluation of this object
   */
  function validate( $scan_data = false ) {
    $data = array( mongo_util::$VALIDATE => $this->name );
    if( $scan_data )
      $data[ "scandata" ] = true;
    return mongo_util::db_command( $this->connection, $data, $this->db );
  }

  /** Inserts an object or array into the collection.
   * @param object $iterable an object or array
   * @return array the associative array saved to the database
   */
  function insert( $iterable ) {
    $arr = mongo_util::obj_to_array( $iterable );
    $result = mongo_insert( $this->connection, (string)$this, $arr );
    if( $result )
      return $iterable;
    return false;
  }

  /** 
   * Querys this collection.
   * @param object $query the fields for which to search
   * @param int $skip number of results to skip
   * @param int $limit number of results to return
   * @param object $fields fields of each result to return
   * @return mongo_cursor a cursor for the search results
   */
  function find( $query = NULL, $skip = NULL, $limit = NULL, $fields = NULL ) {
    return new mongo_cursor( $this->connection, (string)$this, $query, $skip, $limit, $fields );
  }

  /** 
   * Querys this collection, returning a single element.
   * @param object $query the fields for which to search
   * @return object a record matching the search or NULL
   */
  function find_one( $query = NULL ) {
    return mongo_find_one( $this->connection, (string)$this, $query );
  }

  /**
   * Update records based on a given criteria.
   * <!--Options:
   * <dl>
   * <dt>upsert : bool</dt>
   * <dd>if $newobj should be inserted if no matching records are found</dd>
   * <dt>ids : bool </dt>
   * <dd>if if the _id field should be added in the case of an upsert</dd>
   * </dl>-->
   * @param object $criteria description of the objects to update
   * @param object $newobj the object with which to update the matching records
   * @param bool $upsert if the object should be inserted if the criteria isn't found
   * @return array the associative array saved to the db
   */
  function update( $criteria, $newobj, $upsert = false ) {
    $c = mongo_util::obj_to_array( $criteria );
    $obj = mongo_util::obj_to_array( $newobj );
    if( $upsert ) {
      $result = mongo_update( $this->connection, (string)$this, $c, $obj, $upsert );
    }
    else {
      $result = mongo_update( $this->connection, (string)$this, $c, $obj );
    }
    if( $result )
      return $obj;
    return false;
  }

  /**
   * Remove records from this collection.
   * @param object $criteria description of records to remove
   * @param bool $just_one remove at most one record matching this criteria
   * @return bool if the command was executed successfully
   */
  function remove( $criteria, $just_one = false ) {
    return mongo_remove( $this->connection, (string)$this, mongo_util::obj_to_array( $criteria ), $just_one );
  }

  /**
   * Creates an index on the given field(s), or does nothing if the index already exists.
   * @param string|array $keys field or fields to use as index
   */
  function ensure_index( $keys ) {
    $ns = $this->db . "." . $this->name;
    if( is_string( $keys ) ) {
      $keys = array( $keys => 1 );
    }
    $name = mongo_util::to_index_string( $keys );
    $coll = $this->parent->select_collection( "system.indexes" );
    $coll->insert( array( "name" => $name, "ns" => $ns, "key" => $keys ) );
  }
  
  /**
   * Deletes an index from this collection.
   * @param string|array $keys field or fields from which to delete the index
   */
  function delete_index( $key ) {
    $name = mongo_util::to_index_string( $key );
    $coll = $this->parent->select_collection( "system.indexes" );
    $coll->remove( array( "name" => $name ) );
  }

  /**
   * Delete all indices for this collection.
   */
  function delete_indices() {
    mongo_util::db_command( $this->connection, array( mongo_util::$DELETE_INDICES => $this->name ), $this->db );
  }

  /**
   * Counts the number of documents in this collection.
   * @return int the number of documents
   */
  function count() {
    $result = mongo_util::db_command( $this->connection, array( "count" => $this->name ), $this->db );
    if( $result )
      return $result[ "n" ];
    trigger_error( "count failed", E_USER_ERROR );
  }
}

?>
