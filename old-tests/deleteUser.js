db = connect("admin");
db.system.users.remove( { name: "testUser" } );
