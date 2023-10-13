
This is documentation for the asm version of GSL.

- Include GSLib_1.0 in your project 
- Modify the GSL_RAM define to location in RAM
- The library needs 263 bytes of RAM to work.

The library requires external files for use. These include; tiles, palette(s), metatiles and a scrolltable.
The UGT tool is included to produce these files. Please see the associated documentation.

A demonstration on the use of the library can be found in the asm\examples folder.


### Documentation of Public Calls ###

**GSE_InitaliseMap**
@in hl: Scrolltable Location
@in bc: MetatileTable Location

Initialises engine for passed scrolltable. 



**GSE_PositionWindow**
@in hl: Y
@in bc: X

Positions viewable window specified location. Not intended to be used for scrolling rather to positions screen 
at beginning. Note this sets the internals of the engine but does not update the contents of VDP. To update screen
contents GSE_RefreshScreen needs to be called.



**GSE_RefreshScreen**

Updates the entire VDP nametable based on internal positioning. This routine is active display safe so it can be 
used to load next stage while keeping interrupts active for say music routine. You will of course need to dim
the palette to avoid user seeing contents being written.



**GSE_MetatileLookup**
@in bc: Y
@in hl: X
@out hl: address of metatile entry in scrolltable (additionally result is kept in buffer)

Returns both the metatile id and address in scrolltable of a metatile for given x,y pixel coodinates. 

Using the return address, you can modify the map if it is stored in RAM. A call to GSE_MetatileUpdate will then
update this change on screen. An example scenario would be.

- Golden axe warrior swings axe in front of character.
- GSE_MetatileLookup is called for coordinates where axe was swung.
- Check for Metatile id referring to tree which can be cut, result is true.
- Using returned address change this metatile from tree to cut down stump.
- Call GSE_MetatileUpdate to have this change reflect on screen.

NOTE: the metatile address is buffered internally. When GSE_MetatileUpdate is called it will automatically
update the last entry looked up by GSE_MetatileLookup. See Below.



**GSE_TileLookup**
@in bc: Y
@in hl: X
@out hl: Nametable Entry

Returns nametable entry for given x,y pixel coordinates.



**GSE_MetatileUpdate**

Updates screen based on last GSE_MetatileLookup call. Results of last GSE_MetatileLookup call are internally
buffered so no details need to be passed. 

NOTE: This Routine assumes that the scrolltable entry in question is in current display. If you call this on 
an entry that is not in the current window space it will update the metatile relative to current display 
creating garbage. 



**GSE_MetatileUpdateSpecific**
@in hl: Y
@in de: X
@in bc: Scrolltable offset referring to metatile entry needing update.

Like GSE_MetatileUpdate except in this case values are passed. This function is more demo orientated 
however it can be used to create animation on screen etc. 

If you look at powerstrike 2 (sms version) on first stage you can see enemy turrets rising from sea, columns 
of fire appearing etc. This is the routine you would use to do this kind of thing however you would need to 
write your own tools sync changes against currently displayed screen contents.



**GSE_ActiveDisplayRoutine**

Prepares screen scroll for given signed x,y pixel values. Values represent desired scroll in pixels. The 
maximum range is -8 to 8. The values are not passed rather need to be written to ram values GSE_XUpdateRequest 
and GSE_YUpdateRequest.

The routine creates buffers for required row / column updates for the GSE_VBlankRoutine routine. This routine 
should be called whenever you desire scrolling.



**GSE_VBlankRoutine**

Writes buffered row, column, metatile updates to screen. It is safe to call this routine every vblank even if
no changes are required.



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