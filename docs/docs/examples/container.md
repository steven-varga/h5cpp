## File[^1]
<div id="object" style="float: right">
	![Object](../pix/FF-IH_FileObject.gif)
</div>



HDF5 files are organized in a hierarchical structure, with two primary structures: groups and datasets.

* HDF5 group: a grouping structure containing instances of zero or more groups or datasets, together with supporting metadata.
* HDF5 dataset: a multidimensional array of data elements, together with supporting metadata.


Working with groups and group members is similar in many ways to working with directories and files in UNIX. As with UNIX directories and files, objects in an HDF5 file are often described by giving their full (or absolute) path names.
<div id="group" style="float: left">
	![File](../pix/FF-IH_FileGroup.gif)
</div>

`/` signifies the root group. `/foo` signifies a member of the root group called foo. `/foo/zoo` signifies a member of the group foo, which in turn is a member of the root group. Any HDF5 group or dataset may have an associated attribute list. An HDF5 attribute is a user-defined HDF5 structure that provides extra information about an HDF5 object.

For those who are interested, this section takes a look at the low-level elements of the file as the file is written to disk (or other storage media) and the relation of those low-level elements to the higher level elements with which users typically are more familiar. The HDF5 API generally exposes only the high-level elements to the user; the low-level elements are often hidden. The rest of this Introduction does not assume an understanding of this material.

The format of an HDF5 file on disk encompasses several key ideas of the HDF4 and AIO file formats as well as addressing some shortcomings therein. The new format is more self-describing than the HDF4 format and is more uniformly applied to data objects in the file.

 
[^1]: Lifted from HDF5 CAPI documentation

