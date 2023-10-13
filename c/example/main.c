
#include "bank2.h"
#include "bank3.h"
#include "assets\resources.c"
#include "libs\SMSlib.h"
#include "libs\GSLib.c"
#include "libs\PSGlib.c"

#define DIRECTION_UP 			0
#define DIRECTION_DOWN 			1
#define DIRECTION_LEFT  		2
#define DIRECTION_RIGHT  		3

#define ACTION_MOVE  			0
#define ACTION_ATTACK  			1
#define ACTION_STATIONARY 		2

#define METATILE_GREEN_TREE					17
#define METATILE_TREE_STUMP					56
#define METATILE_CUT_CACTUS					144
#define METATILE_SAND						176
#define METATILE_DUNGEON_ENTRANCE			32
#define METATILE_TOMBSTONE					169
#define METATILE_INTERACTIVE_TOMBSTONE_1	120
#define METATILE_INTERACTIVE_TOMBSTONE_2	145
#define METATILE_VERTICAL_BRIDGE			16
#define METATILE_GRASS						89



void processSpritesActiveDisplay();
void processSpritesVBlank();
void processUserInput();
void processUpKey();
void processDownKey();
void processLeftKey();
void processRightKey();
void processAttackKey();
void processAttackInteraction(unsigned char * metatile);
void checkForAttackInteraction();

unsigned int playerX = 904;
unsigned int playerY = 928;
unsigned char playerSpriteX = 136;
unsigned char playerSpriteY = 96;
unsigned char animationCount = 0;

unsigned char actionCount = 0;
unsigned char action = ACTION_STATIONARY;
unsigned char actionButtonWatch = 0;

const unsigned int * spriteTileOffsets = spriteMoveDown;
const unsigned char * attackSprites;
const unsigned char * currentAttackSprites;

unsigned char scrollXOffset = 0;
unsigned char scrollYOffset = 0;
unsigned char spriteXOffset = 0;
unsigned char spriteYOffset = 0;
unsigned int playerXOffset = 0;
unsigned int playerYOffset = 0;
unsigned char direction = DIRECTION_DOWN;

unsigned char scrolltable[scrolltable_bin_size];

SMS_EMBED_SEGA_ROM_HEADER(9999,1);	

void main(void) 
{
	unsigned int ks; 
	unsigned char playerMetatile;
	
	SMS_VRAMmemset(0x4000, 0x00, 0x4000);
	SMS_VRAMmemset(0xC000, 0x00, 0x0020);
	SMS_loadTiles(&tiles_bin, 0, tiles_bin_size);
	SMS_loadBGPalette(&palette_bin);
	SMS_loadTiles(&sprite_tiles_bin, 256, sprite_tiles_bin_size);
	SMS_loadSpritePalette(&sprite_palette_bin);
	
	for(;;)
	{
		// copy scrolltable to RAM
		// NOTE: this allows us to modify the map for hightened player interaction
		// in this case cutting down lots of trees, activating bridges, etc.
		for (int i = 0; i < scrolltable_bin_size; i++) scrolltable[i] = *(scrolltable_bin + i);
		
		// initalise General Scroll Library
		GSL_initializeMap(&scrolltable, &metatiles_bin);
		GSL_positionWindow(768,832);
		GSL_refreshVDP();
		
		SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);
		SMS_displayOn();
		
		PSGPlay(&village_psg);
		
		playerX = 904;
        playerY = 928;
		playerSpriteX = 136;
		playerSpriteY = 96;
		actionCount = 0;
		
		for(;;)
		{
			// check if player is standing on dungeon entrance (completion status)
			playerMetatile = *(GSL_metatileLookup(playerX, playerY)); 
			if (playerMetatile == METATILE_DUNGEON_ENTRANCE) break;
			
			SMS_initSprites(); 
			ks = SMS_getKeysStatus(); 
			if (!(ks & PORT_A_KEY_2)) actionButtonWatch = 0;
			
			// Player actions take so many steps to complete. 
			//   if actionCount == 0 then new actions can be started.
			//   if actionCount > 0 then an action is being processed.
			if (actionCount == 0) processUserInput();
			if (actionCount != 0) actionCount--;
			
			playerX += playerXOffset;
			playerY += playerYOffset;
			playerSpriteX += spriteXOffset;
			playerSpriteY += spriteYOffset;
			GSL_scroll(scrollXOffset,scrollYOffset); // << GSL_scroll with offsets to scroll map.
			
			processSpritesActiveDisplay();
			
			SMS_waitForVBlank(); 
			GSL_VBlank();  // <<< Call GSL_VBlank to process any pending scroll / metatile updates.
			processSpritesVBlank();
			PSGFrame();
		} 
		
		SMS_displayOff();
	}
}



void processSpritesActiveDisplay()
{
	// Add player sprite to sprite table for this frame.
	SMS_addSprite(playerSpriteX + 0xF8, playerSpriteY + 0xF8, 0);
	SMS_addSprite(playerSpriteX + 0x00, playerSpriteY + 0xF8, 1);
	SMS_addSprite(playerSpriteX + 0xF8, playerSpriteY + 0x00, 2);
	SMS_addSprite(playerSpriteX + 0x00, playerSpriteY + 0x00, 3);
	
	
	// Attack animations require additional sprites
	if (action == ACTION_ATTACK)
	{
		// We check for attack interaction with map on actionCount 5 of an attack.
		if (actionCount == 5) checkForAttackInteraction();
		
		currentAttackSprites = attackSprites + ((actionCount & 14) << 3);
		SMS_addSprite(playerSpriteX + *(currentAttackSprites + 0), playerSpriteY + *(currentAttackSprites + 1), *(currentAttackSprites + 2));
		SMS_addSprite(playerSpriteX + *(currentAttackSprites + 3), playerSpriteY + *(currentAttackSprites + 4), *(currentAttackSprites + 5));
		SMS_addSprite(playerSpriteX + *(currentAttackSprites + 6), playerSpriteY + *(currentAttackSprites + 7), *(currentAttackSprites + 8));
		SMS_addSprite(playerSpriteX + *(currentAttackSprites + 9), playerSpriteY + *(currentAttackSprites + 10), *(currentAttackSprites + 11));
	}
}


void processSpritesVBlank()
{
	// Update player sprites in VRAM. Tiles are updated each frame.
	UNSAFE_SMS_copySpritestoSAT();
	
	// Attack animations require max 8 tiles be copied to vram.
	if (action == ACTION_ATTACK) 
	{
		UNSAFE_SMS_load4Tiles(sprite_tiles_bin + *(spriteTileOffsets + ((actionCount & 14) >> 1)), 256);
		UNSAFE_SMS_load4Tiles(128 + sprite_tiles_bin + *(spriteTileOffsets + ((actionCount & 14) >> 1)), 260);
		
	}
	// Other animations only require 4 tiles be copied to vram.
	else UNSAFE_SMS_load4Tiles(sprite_tiles_bin + *(spriteTileOffsets + animationCount), 256);
}



// Test for user input and call appropriate method if so.
void processUserInput()
{
	unsigned int ks = SMS_getKeysStatus(); 
	
	// reset offsets to 0 clearing previous action.
	action = ACTION_STATIONARY;
	scrollXOffset = 0;
	scrollYOffset = 0;
	playerXOffset = 0;
	playerYOffset = 0;
	spriteXOffset = 0;
	spriteYOffset = 0;
	
	if (actionButtonWatch == 0 && (ks & PORT_A_KEY_2))
	{
		processAttackKey();
		return;
	}
	else if (ks & PORT_A_KEY_UP)
	{
		processUpKey();
		return;
	}
	
	else if (ks & PORT_A_KEY_DOWN)
	{
		processDownKey();
		return;
	}
	
	else if (ks & PORT_A_KEY_LEFT)
	{
		processLeftKey();
		return;
	}
	
	else if (ks & PORT_A_KEY_RIGHT)
	{
		processRightKey();
		return;
	}
}


void processUpKey()
{
	// lookup metatile directly above player
	// NOTE: hitbox of player is 2 tiles wide, we need to make 2 metatile checks.
	unsigned char topLeftMetatile = *(GSL_metatileLookup(playerX - 8, playerY - 1));
	unsigned char topRightMetatile = *(GSL_metatileLookup(playerX + 7, playerY - 1));
	
	// Is movement up blocked via metatile or end of screen?
	if (playerY == 8 || ((metatilesMetaLUT[topLeftMetatile] & 1) == 0 && (metatilesMetaLUT[topRightMetatile] & 1)  == 0))
	{
		// animate but do not move.
		action = ACTION_MOVE;
		direction = DIRECTION_UP;
		animationCount = (animationCount - 1) & 3;
		actionCount = 4;
		spriteTileOffsets = spriteMoveUp;
		
		scrollXOffset = 0;
		scrollYOffset = 0;
		playerXOffset = 0;
		playerYOffset = 0;
		spriteXOffset = 0;
		spriteYOffset = 0;
		
		return;
	}
	
	// Quality of life code. If the path up is clipped on one side only have the player
	// shift to the left or right so they can continue up.
	if ((metatilesMetaLUT[topLeftMetatile] & 1) == 1 && (metatilesMetaLUT[topRightMetatile] & 1)  == 0)
	{
		processLeftKey();
		return;
	}
	if ((metatilesMetaLUT[topLeftMetatile] & 1) == 0 && (metatilesMetaLUT[topRightMetatile] & 1)  == 1)
	{
		processRightKey();
		return;
	}
	
	// Movement up is passable. Process movement
	action = ACTION_MOVE;
	direction = DIRECTION_UP;
	animationCount = (animationCount - 1) & 3;
	actionCount = 4;
	spriteTileOffsets = spriteMoveUp;
	
	scrollXOffset = 0;
	playerXOffset = 0;
	spriteXOffset = 0;
	playerYOffset = 0xFFFE;
	
	// Basic window management.
	// Keep player centered except when at edge of screen.
	if (playerY <= 96 || playerY > GSL_getMapHeightInPixels() - 96)
	{
		spriteYOffset = 0xFE;
		scrollYOffset = 0;
	}
	else
	{
		spriteYOffset = 0;
		scrollYOffset = 0xFE;
	}
}


void processDownKey()
{
	// lookup metatile directly below player
	// NOTE: hitbox of player is 2 tiles wide, we need to make 2 metatile checks.
	unsigned char bottomLeftMetatile = *(GSL_metatileLookup(playerX - 8, playerY + 8));
	unsigned char bottomRightMetatile = *(GSL_metatileLookup(playerX + 7, playerY + 8));
	
	// Is movement down blocked via metatile or end of screen?
	if (playerY == GSL_getMapHeightInPixels() - 8 || ((metatilesMetaLUT[bottomLeftMetatile] & 1) == 0 && (metatilesMetaLUT[bottomRightMetatile] & 1) == 0))
	{
		// animate but do not move.
		action = ACTION_MOVE;
		direction = DIRECTION_DOWN;
		animationCount = (animationCount - 1) & 3;
		actionCount = 4;
		spriteTileOffsets = spriteMoveDown;
		
		scrollXOffset = 0;
		scrollYOffset = 0;
		playerXOffset = 0;
		playerYOffset = 0;
		spriteXOffset = 0;
		spriteYOffset = 0;
		
		return;
	}
	
	// Quality of life code. If the path down is clipped on one side only have the player
	// shift to the left or right so they can continue down.
	if ((metatilesMetaLUT[bottomLeftMetatile] & 1) == 1 && (metatilesMetaLUT[bottomRightMetatile] & 1) == 0)
	{
		processLeftKey();
		return;
	}
	if ((metatilesMetaLUT[bottomLeftMetatile] & 1) == 0 && (metatilesMetaLUT[bottomRightMetatile] & 1) == 1)
	{
		processRightKey();
		return;
	}
	
	// Movement down is passable. Process movement
	action = ACTION_MOVE;
	direction = DIRECTION_DOWN;
	animationCount = (animationCount - 1) & 3;
	actionCount = 4;
	spriteTileOffsets = spriteMoveDown;
	
	scrollXOffset = 0;
	playerXOffset = 0;
	spriteXOffset = 0;
	playerYOffset = 2;
	
	// Basic window management.
	// Keep player centered except when at edge of screen.
	if (playerY < 96 || playerY >= GSL_getMapHeightInPixels() - 96)
	{
		spriteYOffset = 2;
		scrollYOffset = 0;
	}
	else
	{
		spriteYOffset = 0;
		scrollYOffset = 2;
	}
}


void processLeftKey()
{
	// lookup metatile directly to the left of player
	// NOTE: hitbox of player is only 1 tile high, top half has no collision!
	unsigned char lowerLeftMetatile = *(GSL_metatileLookup(playerX - 9, playerY));
	
	// Is movement left blocked via metatile or end of screen?
	if (playerX == 16 || (metatilesMetaLUT[lowerLeftMetatile] & 1) == 0)
	{
		// animate but do not move.
		action = ACTION_MOVE;
		direction = DIRECTION_LEFT;
		animationCount = (animationCount - 1) & 3;
		actionCount = 4;
		spriteTileOffsets = spriteMoveLeft;
		
		scrollXOffset = 0;
		scrollYOffset = 0;
		playerXOffset = 0;
		playerYOffset = 0;
		spriteXOffset = 0;
		spriteYOffset = 0;
		
		return;
	}
	
	// Movement left is passable. Process movement
	action = ACTION_MOVE;
	direction = DIRECTION_LEFT;
	animationCount = (animationCount - 1) & 3;
	actionCount = 4;
	spriteTileOffsets = spriteMoveLeft;
	
	playerXOffset = 0xFFFE;
	scrollYOffset = 0;
	playerYOffset = 0;
	spriteYOffset = 0;
	
	// Basic window management.
	// Keep player centered except when at edge of screen.
	if (playerX <= 136 || playerX > GSL_getMapWidthInPixels() - 120)
	{
		spriteXOffset = 0xFE;
		scrollXOffset = 0;
	}
	else
	{
		spriteXOffset = 0;
		scrollXOffset = 0xFE;
	}
}



void processRightKey()
{
	// lookup metatile directly to right of player.
	// NOTE: hitbox of player is only 1 tile high, top half has no collision!
	unsigned char lowerRightMetatile = *(GSL_metatileLookup(playerX + 8, playerY));
	
	// Is movement right blocked via metatile or end of screen?
	if (playerX == GSL_getMapWidthInPixels() - 8 || (metatilesMetaLUT[lowerRightMetatile] & 1) == 0)
	{
		// animate but do not move.
		action = ACTION_MOVE;
		direction = DIRECTION_RIGHT;
		animationCount = (animationCount - 1) & 3;
		actionCount = 4;
		spriteTileOffsets = spriteMoveRight;
		
		scrollXOffset = 0;
		scrollYOffset = 0;
		playerXOffset = 0;
		playerYOffset = 0;
		spriteXOffset = 0;
		spriteYOffset = 0;
		
		return;
	}
	
	// Movement right is passable. Process movement
	action = ACTION_MOVE;
	direction = DIRECTION_RIGHT;
	animationCount = (animationCount - 1) & 3;
	actionCount = 4;
	spriteTileOffsets = spriteMoveRight;
	
	playerXOffset = 2;
	scrollYOffset = 0;
	playerYOffset = 0; 
	spriteYOffset = 0;
	
	// Basic window management.
	// Keep player centered except when at edge of screen.
	if (playerX < 136 || playerX >= GSL_getMapWidthInPixels() - 120)
	{
		spriteXOffset = 2;
		scrollXOffset = 0;
	}
	else
	{
		spriteXOffset = 0;
		scrollXOffset = 2;
	}
}


void processAttackKey()
{
	// value used to check if this is new attack button press.
	actionButtonWatch = 1;
	
	// set sprite animation variables based on attack direction.
	
	switch (direction)
	{
		case DIRECTION_UP:
			attackSprites = spriteAttackUpMeta;
			spriteTileOffsets = spriteAttackUp;
			break;
			
		case DIRECTION_DOWN:
			attackSprites = spriteAttackDownMeta;
			spriteTileOffsets = spriteAttackDown;
			break;
			
		case DIRECTION_LEFT: 
			attackSprites = spriteAttackLeftMeta;
			spriteTileOffsets = spriteAttackLeft;
			break;
			
		case DIRECTION_RIGHT:
			attackSprites = spriteAttackRightMeta;
			spriteTileOffsets = spriteAttackRight;
			break;
	}
	
	// set up remaining attack animation variables.
	actionCount = 9;
	animationCount = 0;
	scrollXOffset = 0;
	scrollYOffset = 0;
	action = ACTION_ATTACK;
	
}


/* checkForAttackInteraction()
 * 
 * This call is made part way through the attack animation.
 * 
 * When attacking with axe we look up the two metatiles in front of axe battler.
 * If the player is perfectly aligned with a metatile this will result in the same metatile
 * being looked up twice which is fine.
*/
void checkForAttackInteraction()
{
	unsigned char * metatile;
	
	// lookup first metatile then test the result against known interactive metatiles.
	if (direction == DIRECTION_UP) 
		metatile = GSL_metatileLookup(playerX - 8, playerY - 1);
	else if (direction == DIRECTION_DOWN) 
		metatile = GSL_metatileLookup(playerX - 8, playerY + 8);
	else if (direction == DIRECTION_LEFT) 
		metatile = GSL_metatileLookup(playerX - 9, playerY - 8);
	else metatile = GSL_metatileLookup(playerX + 8, playerY - 8);
	processAttackInteraction(metatile);
	
	// lookup second metatile then test the result against known interactive metatiles.
	if (direction == DIRECTION_UP) 
		metatile = GSL_metatileLookup(playerX + 7, playerY - 1);
	else if (direction == DIRECTION_DOWN) 
		metatile = GSL_metatileLookup(playerX + 7, playerY + 8);
	else if (direction == DIRECTION_LEFT) 
		metatile = GSL_metatileLookup(playerX - 9, playerY + 7);
	else metatile = GSL_metatileLookup(playerX + 8, playerY + 7);
	processAttackInteraction(metatile);
}



/* processAttackInteraction(unsigned char * metatile)
 *
 * This tests the metatile looked up during attack against a list of interactive metatiles.
 *
 * In a real game you would probably structure this so each stage / map header contains a set
 * number of interactive elements and corrosponding actions.
 *
 * Since this is a quick hack i have hardcoded the values and actions.
*/
void processAttackInteraction(unsigned char * metatile)
{
	// *************************************************************
	// The following two are simple metatile interactions / updates.
	// The metatile is tested and if matching replaced by another metatile.
	
	// GSL_metatileUpdate() will update the screen contents based on the last GSL_metatileLookup() call
	// so as long as there are no additional GSL_metatileLookup() between our first lookup and update we
	// we are set!
	
	
	// is the metatile a tree that can be cut down?
	if (*metatile == METATILE_GREEN_TREE)
	{
		// replace the tree metatile with a tree stump metatile
		*metatile = METATILE_TREE_STUMP;
		
		// que this change to be updated on screen in next vblank.
		GSL_metatileUpdate();
	}
	
	
	// is the metatile a cactus that can be cut down?
	if (*metatile == METATILE_CUT_CACTUS) 
	{
		// replace the cactus metatile with a sand metatile.
		*metatile = METATILE_SAND; 
		
		// que this change to be updated on screen in next vblank.
		GSL_metatileUpdate();
	}
	
	
	// ***************************************************************
	// The following two are more complex interactions.
	// In these cases the metatiles we want to update extend beyond the looked up metatile.
	
	// We use GSL_metatileUpdateCustom(). This method requires the coder to 
	// provide both an (x,y) coodinate pointing to the metatile within the map as well as 
	// an offset for scrolltable pointing to the entry to be updated.
	
	
	// is this interactive tombstone 1?
	if (*metatile == METATILE_INTERACTIVE_TOMBSTONE_1)
	{
		// Replace the interactive tombstone with a regular non interactive one.
		// We do this to remove any further unnecessary updates in future.
		// We don't need to GSL_metatileUpdate() since the graphics are the same.
		*metatile = METATILE_TOMBSTONE; 
		
		// Next remove a fence consisting of two metatiles.
		// First we update the underlying scrolltable (which is in ram)
		// changing those fence metatiles to passable grass metatiles.
		// We add the offset to the base scrolltable address and change value.
		*(GSL_getScrolltableAddress() + (51*64 + 28)) = METATILE_GRASS;
		
		// Secondly we call our GSL_metatileUpdateCustom() method passing an (x,y) as 
		// well as the offset. Offset is calculates as (y*mapwidth + x)
		GSL_metatileUpdateCustom(448, 816, (51*64 + 28));
		
		// This is a repeat of above for the second fence metatile.
		*(GSL_getScrolltableAddress() + (51*64 + 29)) = METATILE_GRASS;
		GSL_metatileUpdateCustom(464, 816, (51*64 + 29));
	}
	
	
	// is this interactive tombstone 2?
	if (*metatile == METATILE_INTERACTIVE_TOMBSTONE_2)
	{
		// Replace the interactive tombstone with a regular non interactive one.
		*metatile = METATILE_TOMBSTONE; 
		
		// Three more custom updates just like the above.
		*(GSL_getScrolltableAddress() + (43*64 + 28)) = METATILE_VERTICAL_BRIDGE;
		GSL_metatileUpdateCustom(448, 688, (43*64 + 28));
		
		*(GSL_getScrolltableAddress() + (44*64 + 28)) = METATILE_VERTICAL_BRIDGE;
		GSL_metatileUpdateCustom(448, 704, (44*64 + 28));
		
		*(GSL_getScrolltableAddress() + (45*64 + 28)) = METATILE_VERTICAL_BRIDGE;
		GSL_metatileUpdateCustom(448, 720, (45*64 + 28));
	}
}








	


