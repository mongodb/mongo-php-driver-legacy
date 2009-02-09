<?php

class mongo_gridfs {
  private $resource;

  public function __construct( $conn, $dbname, $prefix = "fs" ) {
    $this->resource = mongo_gridfs_init( $conn, $dbname, $prefix );
  }

  /**
   * Lists all files matching a given criteria.
   * @param array $query criteria to match
   * @return mongo_cursor cursor over the list of files
   */
  public function list_files( $query = NULL ) {
    if( is_null( $query ) )
      $query = array();
    mongo_gridfs_list( $this->resource, $query );
  }

  /**
   * Stores a file in the database.
   * @param string $filename the name of the file
   * @return mongo_id the database id for the file
   */
  public function store_file( $filename ) {
    return mongo_gridfs_store( $this->resource, $filename );
  }

  /**
   * Retreives a file from the database.
   * @param array|string $query the filename or criteria for which to search
   * @return mongo_gridfs_file the file
   */
  public function find_file( $query ) {
    if( is_string( $query ) )
      $query = array( "filename" => $query );
    return new mongo_gridfs_file( mongo_gridfs_find( $this->resource, $query ) );
  }
}


class mongo_gridfs_file {
  private $file;

  public function __construct( $file ) {
    $this->file = $file;
  }

  /**
   * Checks that the file exists.
   * @return bool if the file exists
   */
  public function exists() {
    return mongo_gridfile_exists( $this->file );
  }

  /**
   * Returns this file's filename.
   * @return string the filename
   */
  public function get_filename() {
    return mongo_gridfile_filename( $this->file );
  }

  /**
   * Returns this file's size.
   * @return int the file size
   */
  public function get_size() {
    return mongo_gridfile_size( $this->file );
  }
}

?>
