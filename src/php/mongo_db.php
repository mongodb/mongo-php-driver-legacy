<?php

class mongo_db {

  var $connection = NULL;
  var $name = NULL;

  public function __construct( mongo $conn, $name ) {
    $this->connection = $conn->connection;
    $this->name = $name;
  }

  public function __toString() {
    return $this->name;
  }


  public function get_auth( $username, $password ) {
    return mongo_auth::get_auth( $this->connection, $this->name, $username, $password );
  }

  /** 
   * Returns the name of the database currently in use.
   * @return string the name of the database
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Gets this database's profiling level.
   * @return int the profiling level
   */
  public function get_profiling_level() {
    $data = array( mongo_util::$PROFILE => -1 );
    $x = mongo_util::db_command( $this->connection, $data, $this->name );
    if( $x[ "ok" ] == 1 )
      return $x[ "was" ];
    else
      return false;
  }

  /**
   * Sets this database's profiling level.
   * @return int the old profiling level
   */
  public function set_profiling_level( $level ) {
    $data = array( mongo_util::$PROFILE => (int)$level );
    $x = mongo_util::db_command( $this->connection, $data, $this->name );
    if( $x[ "ok" ] == 1 ) {
      return $x[ "was" ];
    }
    return false;
  }

  /**
   * Drops this database.
   * @return array db response
   */
  public function drop() {
    $data = array( mongo_util::$DROP_DATABASE => $this->name );
    return mongo_util::db_command( $this->connection, $data );
  }

  /**
   * Repairs and compacts this database.
   * @param bool $preserve_cloned_files if cloned files should be kept if the repair fails (optional)
   * @param bool $backup_original_files if original files should be backed up (optional)
   * @return array db response
   */
  function repair( $preserve_cloned_files = false, $backup_original_files = false ) {
    $data = array( mongo_util::$REPAIR_DATABASE => 1 );
    if( $preserve_cloned_files )
      $data[ "preserveClonedFilesOnFailure" ] = true;
    if( $backup_original_files )
      $data[ "backupOriginalFiles" ] = true;
    return mongo_util::db_command( $this->connection, $data, $this->name );
  }


  /** 
   * Gets a collection.
   * @param string $name the name of the collection
   */
  public function select_collection( $name ) {
    return new mongo_collection( $this, $name );
  }

  /** 
   * Creates a collection.
   * @param string $name the name of the collection
   * @param bool $capped if the collection should be a fixed size
   * @param int $size if the collection is fixed size, its size in bytes
   * @param int $max if the collection is fixed size, the maximum 
   *     number of elements to store in the collection
   * @return Collection a collection object representing the new collection
   */
  public function create_collection( $name, $capped = false, $size = 0, $max = 0 ) {
    $data = array( mongo_util::$CREATE_COLLECTION => $name );
    if( $capped && $size ) {
      $data[ "capped" ] = true;
      $data[ "size" ] = $size;
      if( $max )
        $data[ "max" ] = $max;
    }

    mongo_util::db_command( $this->connection, $data );
    return new mongo_collection( $this, $name );
  }

  /**
   * Drops a collection.
   * @param mongo_collection $coll
   * @return array the db response
   */
  public function drop_collection( mongo_collection $coll ) {
    return $coll->drop();
  }

}

?>
