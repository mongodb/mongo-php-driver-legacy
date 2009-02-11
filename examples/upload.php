<html>
<head>
<title>Sample PHP Driver Usage</title>
</head>
<body>

Files in the db: 
<ol>
<?php 

include_once "src/php/mongo.php";

$m = new Mongo();

$files = $m->selectDB( "phpsite" )->getGridfs()->listFiles();
while( $files->hasNext() ) {
  $f = $files->next();
  echo "<li>" . $f[ "filename" ] . "</li><br />";
}

?>
</ol>


<br />
<br />

Upload a file to the database:
<form method="POST" enctype="multipart/form-data">
  Filename: <input type="file" name="f"><br />
  <input type="submit" value="upload" />
</form>


<?php

if( $_FILES["f"] ) {
  $id = $m->selectDB( "phpsite" )->getGridfs()->saveUpload( "f" );
  echo "saved $id!\n";
}


?>

</body>
</html>
