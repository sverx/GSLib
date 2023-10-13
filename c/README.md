
### This is documentation for the c version of GSL. ###

- Include GSLib_1.0 in your project 

The library requires external files for use. These include; tiles, palette(s), metatiles and a scrolltable.
The UGT tool is included to produce these files. Please see the associated documentation.

A demonstration on the use of the library can be found in the c\examples folder.


### Documentation of Public Calls ###

**void GSL_initializeMap(void *scrolltable, void *metatiles)**
Initalizes the engine using the provided scrolltable and metatiles

**void GSL_positionWindow(unsigned int X, unsigned int Y)**
Sets position of screen to (x,y) for initial use

**void GSL_refreshVDP()**
updates the entire screen for initial use or after a GSL_positionWindow() call.

**unsigned int * GSL_metatileLookup(unsigned int x, unsigned int y)**
returns pointer to the metatile located at position (x,y) in scrolltable.

**void GSL_tileLookup(unsigned int x, unsigned int y)**
Returns the nametable entry of the tile located at position (x,y)

**void GSL_scroll(char x, char y)**
Internally scrolls the map by offsets provided. Range of (-8 to +8).

**void GSL_metatileUpdate()**
Screen update of last metatile looked up by GSL_metatileLookup()

**void GSL_metatileUpdateCustom(unsigned int x, unsigned int y, unsigned int offset)**
This is used in cases where user knows the x,y, and array index of metatile to update.
Useful for animation type stuff.
NOTE: this is a more advanced feature that puts the onus on the coder to manage properly.

**void GSL_VBlank()**
Call this in vblank to have pending scroll and metatile updates processed.

**unsigned int GSL_getMapWidthInPixels()**
Returns map width in pixels.

**unsigned int GSL_getMapHeightInPixels()**
eturns map height in pixels.

**unsigned int GSL_getMapWidthInMetatiles()**
returns map width in metatiles.

**unsigned int GSL_getMapHeightInMetatiles()**
returns map height in metatiles.

**unsigned int GSL_getScrolltableSize()**
returns scrolltable size in bytes.

**unsigned int GSL_getCurrentX()**
returns current internal X position of map.

**unsigned int GSL_getCurrentY()**
returns current internal Y position of map.

**unsigned char * GSL_getScrolltableAddress()**
returns pointer to base scrolltable address.


### General Use ###

To start initalize your map.

- Initalise you map using void GSL_initializeMap(void *scrolltable, void *metatiles).
- Set up the starting (x,y) position of the screen using GSL_positionWindow(unsigned int X, unsigned int Y).
- call void GSL_refreshVDP() to update the entire contents of the nametable.

Once you have started your game loop structure it like so.

- Process user input, enemy logic, game logic etc.
- Call void GSL_scroll(char x, char y) requesting the desired scroll changes for next frame.
- Wait for vblank
- Call void GSL_VBlank() to update vdp with changes.
- Do remaining vblank logic.
- Loop

For a more details explanation please see the example code in c\example


### Data Formats ###

**Metatiles**

Metatiles are stored as raw Nametable entries in order left to right, top to bottom for a total of 4 entries 
(8 bytes). Metatile index 0 is used to contain meta information for the table and is not used. 

Meta information for index 0 is as follows (in same order as file format)...
 - (2 bytes) Length of metatile table in bytes 
 - (6 bytes) unused



**Scrolltable Data**
Scrolltable data is a representation of map using metatiles. Entries are stored as modified Metatile entries in
order left to right, top to bottom. The modified metatile format is (metatile_id << 3) & 248) + ((metatile_id >> 5) & 7).

Scrolltable contains a header before metatile entries. Header is 13 bytes long and is structured...

 - (2 bytes) GSL_ScrolltableSize - size in bytes
 - (2 bytes) GSL_WidthInMetatiles - width in metatile entries
 - (2 bytes) GSL_HeightInMetatiles - height in metatile entries
 - (2 bytes) GSL_WidthInPixels 
 - (2 bytes) GSL_HeightInPixels
 - (2 bytes) GSL_VerticalAddition - width in metatiles * 13
 - (1 bytes) GSL_OptionByte (Lookups require generation of table, highest bit signals to generate table).

### Additional Notes ###

- Engine allows screen to be positioned outside of bounds. Managing window is seen as outside scope of engine
and positioning screen outside of bounds can be useful.