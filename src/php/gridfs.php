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
 * @package Mongo
 */

/**
 * Utilities for storing and retreiving files from the database.
 * 
 * @package Mongo
 */
class MongoGridfs {
  private $_resource;
  private $_prefix;
  private $_db;

  public function __construct( $db, $prefix = "fs" ) {
    $this->_resource = mongo_gridfs_init( $db->connection, $db->name, $prefix );
    $this->_prefix = $prefix;
    $this->_db = $db;
  }

  /**
   * Lists all files matching a given criteria.
   * @param array $query criteria to match
   * @return mongo_cursor cursor over the list of files
   */
  public function listFiles( $query = NULL ) {
    if( is_null( $query ) )
      $query = array();
    return MongoCursor::getGridfsCursor( mongo_gridfs_list( $this->_resource, $query ) );
  }

  /**
   * Stores a file in the database.
   * @param string $filename the name of the file
   * @return mongo_id the database id for the file
   */
  public function storeFile( $filename ) {
    return mongo_gridfs_store( $this->_resource, $filename );
  }

  /**
   * Retreives a file from the database.
   * @param array|string $query the filename or criteria for which to search
   * @return mongo_gridfs_file the file
   */
  public function findFile( $query ) {
    if( is_string( $query ) )
      $query = array( "filename" => $query );
    return new MongoGridfsFile( mongo_gridfs_find( $this->_resource, $query ) );
  }

  /**
   * Saves an uploaded file to the database.
   * @param string $name the name field of the uploaded file
   * @return mongo_id the id of the uploaded file
   */
  public function saveUpload( $name ) {
    if( !$name || !is_string( $name ) ||
        !$_FILES || !$_FILES[ $name ] ) {
      return false;
    }

    $tmp = $_FILES[ $name ]["tmp_name"];
    $name = $_FILES[ $name ]["name"];
    
    $this->storeFile( $tmp );

    // make the filename more paletable
    $coll = $this->_db->selectCollection( $this->_prefix . ".files" );
    $obj = $coll->findOne( array( "filename" => $tmp ) );    
    $obj[ "filename" ] = $name;
    $coll->update( array( "filename" => $tmp ), $obj );

    return $obj[ "_id" ];
  }
}

/**
 * Utilities for getting information about files from the database.
 * 
 * @package Mongo
 */
class MongoGridfsFile {
  private $_file;

  public function __construct( $file ) {
    $this->_file = $file;
  }

  /**
   * Checks that the file exists.
   * @return bool if the file exists
   */
  public function exists() {
    return mongo_gridfile_exists( $this->_file );
  }

  /**
   * Returns this file's filename.
   * @return string the filename
   */
  public function getFilename() {
    return mongo_gridfile_filename( $this->_file );
  }

  /**
   * Returns this file's size.
   * @return int the file size
   */
  public function getSize() {
    return mongo_gridfile_size( $this->_file );
  }
}

?>
