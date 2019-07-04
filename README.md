# MyFileSystem

This is a simple file system. Here arrays are used to create an abstraction of hard disk. The file system is implemented on this abstract hard disk. The various operations that can be performed are as follows:

   2.  myfs> /* prompt given by this program */
   3.  myfs> mkfs "Your Drive Name" "block Size" "Total Size"MB /* creates the filesystem named "Drive name", with specified blocksize  */
   5.  myfs> use "Your Drive name" as C: /* the filesystem on "Your Drive name" will henceforth be accessed as C: */
   7.  myfs> cp "Source File" C:\"dest File" /* copy the file "Source File" from os to the filesystem C: as "Dest File" */
   8.  myfs> ls C: /* see the contents of the filesystem C: */
   9.  myfs> cp C:\"Sourece File" C:\"Dest File" /* copy the file "Source File" from C: to the filesystem C: as "dest File" */
  12.  myfs> rm C:\"File-Name" /* Delete the "File" from C: */
  13.  myfs> mv D:\"Source File" C:\"Dest File"  /* Move  "Source File" of D: to "Dest File" in C: */ 
  14.  myfs> create "Drive Name" "File Name" /*Create a file "File Name" in the drive "Drive Name"*/
  15.  myfs> write "Drive_Name:\File_Name" /*Write to "File_Name" in "Drive_Name"*/
  16.  myfs> display "Drive_Name:\File_Name" /*Display content of "File_Name" in "Drive_Name"*/
  16.  myfs> exit /* terminate the process */
