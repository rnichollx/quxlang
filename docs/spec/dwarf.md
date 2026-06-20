# Version Code

The version code to be used in Quxlang with DWARF shall have the following properties:

If the entire version number is the value of 0, this indicates a build without any version information. Otherwise, the least significant 8 bits represent the Major version of the langauge, unless they are 0 or greater than 127. The value of 0 indicates a preview, special, experimental, or development version.  A value greater than 127 is reserved for any major version which requires more than 7 bits to represent the major version. The remaining bits have a format which is defined by the major version. As no major version has been released yet, the meaning of these bits are currently undefined.