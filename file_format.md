The file format is very simple.

Each section has at least a 12-byte header that at least has these fields:

```
Size  Offset       Meaning  Remarks
 u32       0          size  including the bytes in the header, the size of the whole section/file
 u32       4  magic_number
 u32       8       version
```

Currently all version fields should be zeroed.

Magic numbers for the following types are defined:
```
0xe8ba0e52   file header
0xadd685f1   TA FIFO block
0xa1e831d2   Arbitrary message (should be 1st block if present)
0x78d6a805   Game info (must be 1st or second block if present)
0x5930a419   PVR register updates
0xd7be4263   VRAM block
```

Each header type has additional info:

```
File header
~~~~~~~~~~~
Size  Offset       Meaning  Remarks
 u32       0          size  WHOLE FILE size including the bytes in this header
 u32       4  magic_number
 u32       8       version
 u32      12    offset_to_first_block
```

```
TA FIFO block
~~~~~~~~~~~~~
Size  Offset       Meaning  Remarks
 u32       0          size  =20-? including the bytes in the header, the size of the whole section
 u32       4  magic_number
 u32       8       version
 u32      12         flags  (currently not defined, should be 0)
 u32      16        length  in bytes of TA FIFO info
 ?        20          data  data in the TA FIFO block
```

```
arbitrary message
~~~~~~~~~~~~~~~~~
Size  Offset       Meaning  Remarks
 u32       0          size  =16-? including the bytes in the header, the size of the whole section
 u32       4  magic_number
 u32       8       version
 u32      12        length  Length of arbitrary message. You should cap at maybe 10k?
 ?        16          data  The data in the message
```

```
Register update
~~~~~~~~~~~~~~~
Size  Offset       Meaning  Remarks
 u32       0          size  =16-? including the bytes in the header, the size of the whole section
 u32       4  magic_number
 u32       8       version
 u32      12         count  Count of register update sets
 ?        16          data  The data, structured as below
 
 Each register update is
 Size  Offset       Meaning
  u32       0       Address of register
  u32       4       Value of register/written to register
  u32       8       Size of write. 1=8bit, 2=16bit, 4=32bit
  
They are just packed together like that, however many there are.
```

```
VRAM block
~~~~~~~~~~
Size  Offset       Meaning  Remarks
 u32       0          size  =20-(16MB + 20) including the bytes in the header, the size of the whole section
 u32       4  magic_number
 u32       8       version
 u32      12        offset  from start of VRAM(0), where to start writing this in
 u32      16        length  in bytes of VRAM block
 ?        20          data  data in the TA FIFO block
```

```
basic game info header
~~~~~~~~~~~~~~~~~~~~~~
Size  Offset       Meaning  Remarks
 u32       0          size  =128 including the bytes in the header, the size of the whole section
 u32       4  magic_number
 u32       8       version
 i64      12     frame_num  Frame # this was captured, or -1 if none
 u32      20      name_len  The length of the name field, in bytes, max 49
 50       24          name  The name of the game, should be null-terminated, max 49 chars (+1 for null)
 u32      74  filename_len  The length of the name field
 50       78      filename  Same format as name above. Denotes filename of disc image if any, or of first in multi-file set  
```
