/*-------------------------------------------------------------------------------

	BARONY
	File: drawminimap.cpp
	Desc: Handles drawing the Minimap in the bottom-right corner of the screen

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "../main.hpp"
#include "../game.hpp"
#include "../stat.hpp"
#include "../items.hpp"
#include "../sound.hpp"
#include "../menu.hpp"
#include "../player.hpp"
#include "interface.hpp"

/*-------------------------------------------------------------------------------

	drawMinimap

	Draws the game's minimap in the lower right corner of the screen

-------------------------------------------------------------------------------*/

/* minimap[][] is a 2D array of Sint8's, these are the meaning of their values:
 * 0 - No Floor
 * 1 - Walkable Space
 * 2 - Wall
 * 3 - Unused?
 * 4 - Unused?
 */

// Color Arrays - Colors are float values between 0.0f and 1.0f to avoid remapping.
// To get the desired color, take the RGB values (0 - 255) and divide it by 255.f. This will return the value as a viable float
										//{red.f / 255.f, grn.f / 255.f, blu.f / 255.f, alpha}
static const float COLOR_DARK_GREY[4]	= { 32.f / 255.f,  32.f / 255.f,  32.f / 255.f, 1.f}; // TODOR: SEEMINGLY UNUSED
static const float COLOR_GREY[4]		= { 64.f / 255.f,  64.f / 255.f,  64.f / 255.f, 1.f}; // TODOR: SEEMINGLY UNUSED
static const float COLOR_LIGHT_RED[4]	= {255.f / 255.f,  64.f / 255.f,  64.f / 255.f, 1.f}; // Minotaur Aiming
static const float COLOR_LIGHT_GREEN[4]	= { 64.f / 255.f, 255.f / 255.f,  64.f / 255.f, 1.f}; // Local Player Aiming
static const float COLOR_LIGHT_BLUE[4]	= { 64.f / 255.f,  64.f / 255.f, 255.f / 255.f, 1.f}; // Other Players Aiming
static const float COLOR_PURPLE[4]		= {128.f / 255.f,  64.f / 255.f, 128.f / 255.f, 1.f}; // Monsters
static const float COLOR_BLACK[4]		= { 32.f / 255.f,  12.f / 255.f,           0.f, 1.f}; // Undiscovered or No Floor
static const float COLOR_MAROON[4]		= { 96.f / 255.f,  24.f / 255.f,           0.f, 1.f}; // Walkable
static const float COLOR_ORANGE[4]		= {192.f / 255.f,  64.f / 255.f,           0.f, 1.f}; // Wall and Boulders
static const float COLOR_DARK_RED[4]	= {192.f / 255.f,           0.f,           0.f, 1.f}; // Minotaur
static const float COLOR_GREEN[4]		= {          0.f, 192.f / 255.f,           0.f, 1.f}; // Local Player and Follower Aiming
static const float COLOR_DARK_GREEN[4]	= {          0.f, 128.f / 255.f,           0.f, 1.f}; // Followers
static const float COLOR_CYAN[4]		= {          0.f, 192.f / 255.f, 192.f / 255.f, 1.f}; // Minotaur for Colorblind Mode
static const float COLOR_BLUE[4]		= {          0.f,           0.f, 192.f / 255.f, 1.f}; // Other Players Aiming
static const float COLOR_RED[4]			= {          1.f,           0.f,           0.f, 1.f}; // Ladders and Portals TODOR: (NOT END PORTAL)
static const float COLOR_TURQUOISE[4]	= {          0.f,           1.f,           1.f, 1.f}; // Ladders, Portals, and Minotaur Aiming for Colorblind Mode TODOR: (NOT END PORTAL)

// For easier modification, and more readable code, glColor4fv() is used over glColor4f. A pointer to an array of floats is used
static const float* const pColor_Unmapped			= COLOR_BLACK;
static const float* const pColor_Walkable			= COLOR_MAROON;
static const float* const pColor_Wall				= COLOR_ORANGE;
static const float* const pColor_Unknown1			= COLOR_DARK_GREY;	// TODOR: SEEMINGLY UNUSED
static const float* const pColor_Unknown2			= COLOR_GREY;		// TODOR: SEEMINGLY UNUSED
static const float* const pColor_Exit				= COLOR_RED;
static const float* const pColorblind_Exit			= COLOR_TURQUOISE;
static const float* const pColor_Monster			= COLOR_PURPLE;
static const float* const pColor_LocalPlayer		= COLOR_GREEN;
static const float* const pColor_LocalPlayerAiming	= COLOR_LIGHT_GREEN;
static const float* const pColor_Player				= COLOR_BLUE;
static const float* const pColor_PlayerAiming		= COLOR_LIGHT_BLUE;
static const float* const pColor_Follower			= COLOR_DARK_GREEN;
static const float* const pColor_FollowerAiming		= COLOR_GREEN;
static const float* const pColor_Minotaur			= COLOR_DARK_RED;
static const float* const pColor_MinotaurAiming		= COLOR_LIGHT_RED;
static const float* const pColorblind_Minotaur		= COLOR_CYAN;
static const float* const pColorblind_MinotaurAiming= COLOR_TURQUOISE;

static const GLfloat MINIMAPSCALE = 4.f; // A single cell in the Map is 4x4

bool bHasMinotaurRoared = false;

/* 
 *
 */
void Draw_Level()
{
	// The maximum size of a Map should be no greater than 255x255
	for ( Uint8 iMapX = 0; iMapX < map.width; iMapX++ )
	{
		for ( Uint8 iMapY = 0; iMapY < map.height; iMapY++ )
		{
			if ( minimap[iMapY][iMapX] == 0 )		// No Floor
			{
				glColor4fv(pColor_Unmapped);
			}
			else if ( minimap[iMapY][iMapX] == 1 ) // Walkable Space
			{
				glColor4fv(pColor_Walkable);
			}
			else if ( minimap[iMapY][iMapX] == 2 ) // Wall
			{
				glColor4fv(pColor_Wall);
			}
			else if ( minimap[iMapY][iMapX] == 3 ) // TODOR: SEEMINGLY UNUSED
			{
				glColor4fv(pColor_Unknown1);
			}
			else if ( minimap[iMapY][iMapX] == 4 ) // TODOR: SEEMINGLY UNUSED
			{
				glColor4fv(pColor_Unknown2);
			}

			// The location of the quad in the Minimap
			const GLfloat tileOffsetX = xres + (static_cast<GLfloat>(iMapX) * MINIMAPSCALE);
			const GLfloat tileOffsetY = static_cast<GLfloat>(iMapY) * MINIMAPSCALE;

			// Tiles are 4px by 4px, so they have to be scaled appropriately
			const GLfloat scaledTileWidth  = static_cast<GLfloat>(map.width)  * MINIMAPSCALE;
			const GLfloat scaledTileHeight = static_cast<GLfloat>(map.height) * MINIMAPSCALE;

			glBegin(GL_QUADS);
			// Draw the Quad by listing the position of each vertex. Listed in this order: Top-Right, Top-Left, Bottom-Left, Bottom-Right 
			glVertex2f(tileOffsetX - scaledTileWidth,                scaledTileHeight - tileOffsetY - MINIMAPSCALE);
			glVertex2f(tileOffsetX - scaledTileWidth + MINIMAPSCALE, scaledTileHeight - tileOffsetY - MINIMAPSCALE);
			glVertex2f(tileOffsetX - scaledTileWidth + MINIMAPSCALE, scaledTileHeight - tileOffsetY);
			glVertex2f(tileOffsetX - scaledTileWidth,                scaledTileHeight - tileOffsetY);
			glEnd();
		}
	}
} // Draw_Level()

/* 
 *
 */
void Draw_Exit(Entity* const pEntity)
{
	// TODOR: What is this for /exactly/?
	if ( pEntity->x >= 0.0 && pEntity->y >= 0.0 && pEntity->x < map.width << 4 && pEntity->y < map.height << 4 )
	{
		// Check the position of the Exit, if it has been found, process it
		Sint8 entityMapX = static_cast<Sint8>(floor(pEntity->x / 16.0));
		Sint8 entityMapY = static_cast<Sint8>(floor(pEntity->y / 16.0));

		if ( minimap[entityMapY][entityMapX] )
		{
			if ( ticks % 40 - ticks % 20 )
			{
				// Check the position of the Exit, if it has been found, process it
				const GLfloat entityOffsetX = xres + entityMapX * MINIMAPSCALE;
				const GLfloat entityOffsetY = entityMapY * MINIMAPSCALE;

				// Tiles are 4px by 4px, so they have to be scaled appropriately
				const GLfloat scaledTileWidth  = static_cast<GLfloat>(map.width)  * MINIMAPSCALE;
				const GLfloat scaledTileHeight = static_cast<GLfloat>(map.height) * MINIMAPSCALE;

				if ( colorblind )
				{
					glColor4fv(pColorblind_Exit);
				}
				else
				{
					glColor4fv(pColor_Exit);
				}

				glBegin(GL_QUADS);
				// entityMapX * MINIMAPSCALE + xres - map.edith * MINIMAPSCALE (+ MINIMAPSCALE)
				glVertex2f(entityOffsetX - scaledTileWidth,					scaledTileHeight - entityOffsetY - MINIMAPSCALE);
				glVertex2f(entityOffsetX - scaledTileWidth + MINIMAPSCALE,	scaledTileHeight - entityOffsetY - MINIMAPSCALE);
				glVertex2f(entityOffsetX - scaledTileWidth + MINIMAPSCALE,	scaledTileHeight - entityOffsetY);
				glVertex2f(entityOffsetX - scaledTileWidth,					scaledTileHeight - entityOffsetY);
				glEnd();
			}
		}
	}
} // Draw_Exit()

/* 
 *
 */
void Draw_Boulder(Entity* const pEntity)
{
	// TODOR: What is this for /exactly/?
	Sint8 entityMapX = std::min<Sint8>(std::max<Sint8>(0.0, pEntity->x / 16.0), map.width - 1);
	Sint8 entityMapY = std::min<Sint8>(std::max<Sint8>(0.0, pEntity->y / 16.0), map.height - 1);
	if ( minimap[entityMapY][entityMapX] == 1 || minimap[entityMapY][entityMapX] == 2 )
	{
		// Check the position of the Exit, if it has been found, process it
		const GLfloat entityOffsetX = xres + entityMapX * MINIMAPSCALE;
		const GLfloat entityOffsetY = entityMapY * MINIMAPSCALE;

		// Tiles are 4px by 4px, so they have to be scaled appropriately
		const GLfloat scaledTileWidth  = static_cast<GLfloat>(map.width)  * MINIMAPSCALE;
		const GLfloat scaledTileHeight = static_cast<GLfloat>(map.height) * MINIMAPSCALE;

		glColor4fv(pColor_Wall);

		glBegin(GL_QUADS);
		glVertex2f(entityOffsetX - scaledTileWidth,					scaledTileHeight - entityOffsetY - MINIMAPSCALE);
		glVertex2f(entityOffsetX - scaledTileWidth + MINIMAPSCALE,	scaledTileHeight - entityOffsetY - MINIMAPSCALE);
		glVertex2f(entityOffsetX - scaledTileWidth + MINIMAPSCALE,	scaledTileHeight - entityOffsetY);
		glVertex2f(entityOffsetX - scaledTileWidth,					scaledTileHeight - entityOffsetY);
		glEnd();
	}
} // Draw_Boulder()



/* 
 *
 */
bool DoesPlayerHaveWarningEffect(Entity* const pEntity)
{
	if ( stats[clientnum]->ring != nullptr )
	{
		if ( stats[clientnum]->ring->type == RING_WARNING )
		{
			return true;
		}
	}
	else if ( stats[clientnum]->shoes != nullptr )
	{
		if ( stats[clientnum]->shoes->type == ARTIFACT_BOOTS )
		{
			return true;
		}
	}

	return false;
} // DoesPlayerHaveWarningEffect()

/* 
 *
 */
void Draw_Monster(Entity* const pEntity, const float* const pColor)
{
	// The location of the quad on the screen
	const GLfloat screenOffsetX = static_cast<GLfloat>(MINIMAPSCALE + xres - map.width * MINIMAPSCALE);
	const GLfloat screenOffsetY = static_cast<GLfloat>(MINIMAPSCALE + xres - map.width * MINIMAPSCALE);

	// The location of the quad in the Minimap
	const GLfloat entityOffsetX = xres + static_cast<GLfloat>(floor(pEntity->x / 16.0)) * MINIMAPSCALE;
	const GLfloat entityOffsetY = static_cast<GLfloat>(floor(pEntity->y / 16.0)) * MINIMAPSCALE;

	// Tiles are 4px by 4px, so they have to be scaled appropriately
	const GLfloat scaledTileWidth  = static_cast<GLfloat>(map.width)  * MINIMAPSCALE;
	const GLfloat scaledTileHeight = static_cast<GLfloat>(map.height) * MINIMAPSCALE;

	glColor4fv(pColor);
	glBegin(GL_QUADS);
	// Draw the quad by listing the position of each vertex. Listed in this order: Top-Right, Top-Left, Bottom-Left, Bottom-Right 
	glVertex2f(entityOffsetX - scaledTileWidth,                scaledTileHeight - entityOffsetY - MINIMAPSCALE);
	glVertex2f(entityOffsetX - scaledTileWidth + MINIMAPSCALE, scaledTileHeight - entityOffsetY - MINIMAPSCALE);
	glVertex2f(entityOffsetX - scaledTileWidth + MINIMAPSCALE, scaledTileHeight - entityOffsetY);
	glVertex2f(entityOffsetX - scaledTileWidth,                scaledTileHeight - entityOffsetY);
	glEnd();
} // Draw_Monster()

/* 
 *
 */
void Render_MinimapCircle(Entity* const pEntity, const float* const pColor, const float* const pAimingColor)
{
	// The screen offset of the first pixel
	const GLfloat circleCoordX = static_cast<GLfloat>(xres - map.width * MINIMAPSCALE + static_cast<int>((pEntity->x / 4.0)));
	const GLfloat circleCoordY = static_cast<GLfloat>(map.height * MINIMAPSCALE - static_cast<int>((pEntity->y / 4.0)));

	// Set the Color
	glColor4fv(pColor);

	// Draw the first pixel
	glBegin(GL_POINTS);
	glVertex2f(circleCoordX, circleCoordY);
	glEnd();

	// Draw the circle
	drawCircle(circleCoordX - 1, yres - circleCoordY - 1, 3, pColor, 255);
	
	// Use a brighter color shade for the Aiming marker
	glColor4fv(pAimingColor);

	// The forward offset to draw the Aiming marker
	GLfloat circleOffsetX = 0.f;
	GLfloat circleOffsetY = 0.f;

	// Draw the Aiming marker by drawing incrementally forward
	for ( Uint8 iAimingSegement = 0; iAimingSegement < 4; iAimingSegement++ )
	{
		// Increment the marker's position forward
		if ( cos(pEntity->yaw) > 0.4 )
		{
			circleOffsetX++;
		}
		else if ( cos(pEntity->yaw) < -0.4 )
		{
			circleOffsetX--;
		}

		if ( sin(pEntity->yaw) > 0.4 )
		{
			circleOffsetY++;
		}
		else if ( sin(pEntity->yaw) < -0.4 )
		{
			circleOffsetY--;
		}

		glBegin(GL_POINTS);
		glVertex2f(circleCoordX + circleOffsetX, circleCoordY - circleOffsetY);
		glEnd();
	}
} // Render_MinimapCircle()

/* 
 *
 */
void Draw_Minotaur(Entity* const pEntity)
{
	if ( (ticks % 120) - (ticks % 60) )
	{
		if ( !(bHasMinotaurRoared) )
		{
			playSound(116, 64);
		}

		bHasMinotaurRoared = true;

		if ( colorblind )
		{
			Render_MinimapCircle(pEntity, pColorblind_Minotaur, pColorblind_MinotaurAiming);
		}
		else
		{
			Render_MinimapCircle(pEntity, pColor_Minotaur, pColor_MinotaurAiming);
		}
	}
	else
	{
		bHasMinotaurRoared = false;
	}
} // Draw_Minotaur()

/* 
 *
 */
void drawMinimap()
{
	Uint32 color;

	int x, y, i;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Draw the current Floor
	//glBegin(GL_QUADS);
	Draw_Level();
	//glEnd();

	/*
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glViewport(0, 0, xres, yres);
	glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glBindTexture(GL_TEXTURE_2D, 0);*/

	// Draw the Exits, Boulders, and Monsters
	//glBegin(GL_QUADS);
	for ( node_t* iMapEntityNode = map.entities->first; iMapEntityNode != nullptr; iMapEntityNode = iMapEntityNode->next )
	{
		Entity* pEntity = static_cast<Entity*>(iMapEntityNode->element);
		if ( pEntity->sprite == 161 || (pEntity->sprite >= 254 && pEntity->sprite < 258) )   // Exits (Ladders, Portals) TODOR: NOT END PORTAL
		{
			Draw_Exit(pEntity);
		}
		else if ( pEntity->sprite == 245 )     // Boulders
		{
			Draw_Boulder(pEntity);
		}
		else if( pEntity->behavior == &actMonster ) // Monsters
		{
			// Check if the Entity is a Follower
			bool bIsEntityAFollower = false;

			for ( node_t* iFollowerEntityNode = stats[clientnum]->FOLLOWERS.first; iFollowerEntityNode != nullptr; iFollowerEntityNode = iFollowerEntityNode->next )
			{
				if ( *(static_cast<Uint32*>(iFollowerEntityNode->element)) == pEntity->getUID() )
				{
					bIsEntityAFollower = true;
					Render_MinimapCircle(pEntity, pColor_Follower, pColor_FollowerAiming);
					break;
				}
			}

			if ( !(bIsEntityAFollower) )
			{
				// If the Entity is not a Follower, check if the Local Player has the Warning Effect
				if ( DoesPlayerHaveWarningEffect(pEntity) )
				{
					// Prevent the Warning Effect from being drawn if Minotaur is being drawn
					if ( pEntity->sprite != 239 )
					{
						Draw_Monster(pEntity, pColor_Monster);
					}
					else
					{
						if ( !bHasMinotaurRoared )
						{
							Draw_Monster(pEntity, pColor_Monster);
						}
					}
				}
			}
		} 
		else if ( pEntity->behavior == &actPlayer )
		{
			// Check if the Player is the Local Player
			if ( pEntity->skill[2] == clientnum )
			{
				Render_MinimapCircle(pEntity, pColor_LocalPlayer, pColor_LocalPlayerAiming);
			}
			else
			{
				Render_MinimapCircle(pEntity, pColor_Player, pColor_PlayerAiming);
			}
		}
		else if ( pEntity->sprite == 237 ) // "minotaur_head.vox"
		{
			Draw_Minotaur(pEntity);
		}
	}
	//glEnd();

	// Draw the Local Player and other Players/Followers
	/*
	for ( node_t* iMapEntityNode = map.entities->first; iMapEntityNode != nullptr; iMapEntityNode = iMapEntityNode->next )
	{
		Entity* pEntity = static_cast<Entity*>(iMapEntityNode->element);

		bool bIsEntityPlayer = false;		// If the Entity is a Player or a Follower, draw them as a blue circle
		bool bIsEntityLocalPlayer = false;  // If the Entity is the Local Player, draw them as a green circle

		if ( pEntity->behavior == &actPlayer )
		{
			bIsEntityPlayer = true;

			// Check if the Player is the Local Player
			if ( pEntity->skill[2] == clientnum )
			{
				bIsEntityLocalPlayer = true;
			}
		}
		else if ( pEntity->behavior == &actMonster )
		{
			// Check if the Entity is a Follower
			for ( node_t* iFollowerEntityNode = stats[clientnum]->FOLLOWERS.first; iFollowerEntityNode != nullptr; iFollowerEntityNode = iFollowerEntityNode->next )
			{
				if ( *(static_cast<Uint32*>(iFollowerEntityNode->element)) == pEntity->getUID() )
				{
					bIsEntityPlayer = true;
					break;
				}
			}
		}

		// Draw the valid Entities
		if ( bIsEntityPlayer )
		{
			// Local Player = Green, Other Players/Followers = Blue
			if ( bIsEntityLocalPlayer )
			{
				glColor4fv(pColor_LocalPlayer);
				//color = SDL_MapRGB(mainsurface->format, 0, 192, 0);
			}
			else
			{
				glColor4fv(pColor_Player);
				//color = SDL_MapRGB(mainsurface->format, 0, 0, 192);
			}

			// draw the first pixel
			x = xres - map.width * MINIMAPSCALE + (int)(pEntity->x / (16.f / MINIMAPSCALE));
			y = map.height * MINIMAPSCALE - (int)(pEntity->y / (16.f / MINIMAPSCALE));
			if ( softwaremode )
			{
				//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAP
			}
			else
			{
				//glColor4f(((Uint8)(color >> mainsurface->format->Rshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f, 1);
				glBegin(GL_POINTS);
				glVertex2f(x, y);
				glEnd();
			}

			// Draw a circle. Local Player = Green, Other Players/Followers = Blue
			if ( bIsEntityLocalPlayer )
			{
				drawCircle(x - 1, yres - y - 1, 3, pColor_LocalPlayer, 255);
			}
			else
			{
				drawCircle(x - 1, yres - y - 1, 3, pColor_Player, 255);
			}

			x = 0;
			y = 0;
			for ( i = 0; i < 4; i++ )
			{
				// move forward
				if ( cos(pEntity->yaw) > .4 )
				{
					x++;
				}
				else if ( cos(pEntity->yaw) < -.4 )
				{
					x--;
				}
				if ( sin(pEntity->yaw) > .4 )
				{
					y++;
				}
				else if ( sin(pEntity->yaw) < -.4 )
				{
					y--;
				}

				// get brighter color shade
				if ( bIsEntityLocalPlayer )
				{
					glColor4fv(pColor_LocalPlayerAiming);
					//color = SDL_MapRGB(mainsurface->format, 64, 255, 64);
				}
				else
				{
					glColor4fv(pColor_PlayerAiming);
					//color = SDL_MapRGB(mainsurface->format, 64, 64, 255);
				}

				// draw the pixel
				if ( softwaremode )
				{
					//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAP
				}
				else
				{
					//glColor4f(((Uint8)(color >> mainsurface->format->Rshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Gshift)) / 255.f, ((Uint8)(color >> mainsurface->format->Bshift)) / 255.f, 1);
					glBegin(GL_POINTS);
					glVertex2f(xres - map.width * MINIMAPSCALE + (int)(pEntity->x / (16.f / MINIMAPSCALE)) + x, map.height * MINIMAPSCALE - (int)(pEntity->y / (16.f / MINIMAPSCALE)) - y);
					glEnd();
				}
			}
		}
	}*/

	/*
	// draw minotaur
	if ( players[clientnum] == nullptr )
	{
		return;
	}
	for ( node_t* iMapEntityNode = map.entities->first; iMapEntityNode != nullptr; iMapEntityNode = iMapEntityNode->next )
	{
		Entity* pEntity = static_cast<Entity*>(iMapEntityNode->element);
		if ( pEntity->sprite == 239 )
		{
			if ( ticks % 120 - ticks % 60 )
			{
				if ( !minotaur_timer )
				{
					playSound(116, 64);
				}
				minotaur_timer = 1;
				if ( !colorblind )
				{
					glColor4fv(pColor_Minotaur);
					//color = SDL_MapRGB(mainsurface->format, 192, 0, 0);
				}
				else
				{
					glColor4fv(pColorblind_Minotaur);
					//color = SDL_MapRGB(mainsurface->format, 0, 192, 192);
				}

				// draw the first pixel
				x = xres - map.width * MINIMAPSCALE + (int)(pEntity->x / (16.f / MINIMAPSCALE));
				y = map.height * MINIMAPSCALE - (int)(pEntity->y / (16.f / MINIMAPSCALE));
				if ( softwaremode )
				{
					//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAP
				}
				else
				{
					//glColor4f(((Uint8)(color >> 16)) / 255.f, ((Uint8)(color >> 8)) / 255.f, ((Uint8)(color)) / 255.f, 1);
					glBegin(GL_POINTS);
					glVertex2f(x, y);
					glEnd();
				}

				// draw a circle
				if ( !colorblind )
				{
					drawCircle(x - 1, yres - y - 1, 3, pColor_Minotaur, 255);
				}
				else
				{
					drawCircle(x - 1, yres - y - 1, 3, pColorblind_Minotaur, 255);
				}

				x = 0;
				y = 0;
				for ( i = 0; i < 4; i++ )
				{
					// move forward
					if ( cos(pEntity->yaw) > .4 )
					{
						x++;
					}
					else if ( cos(pEntity->yaw) < -.4 )
					{
						x--;
					}
					if ( sin(pEntity->yaw) > .4 )
					{
						y++;
					}
					else if ( sin(pEntity->yaw) < -.4 )
					{
						y--;
					}

					// get brighter color shade
					if ( !colorblind )
					{
						glColor4fv(pColor_MinotaurAiming);
						//color = SDL_MapRGB(mainsurface->format, 255, 64, 64);
					}
					else
					{
						glColor4fv(pColorblind_MinotaurAiming);
						//color = SDL_MapRGB(mainsurface->format, 64, 255, 255);
					}

					// draw the pixel
					if ( softwaremode )
					{
						//SPG_Pixel(screen,(int)(players[c]->x/16)+564+x+xres/2-(status_bmp->w/2),(int)(players[c]->y/16)+yres-71+y,color); //TODO: NOT a PLAYERSWAR
					}
					else
					{
						//glColor4f(((Uint8)(color >> 16)) / 255.f, ((Uint8)(color >> 8)) / 255.f, ((Uint8)(color)) / 255.f, 1);
						glBegin(GL_POINTS);
						glVertex2f(xres - map.width * MINIMAPSCALE + (int)(pEntity->x / (16.f / MINIMAPSCALE)) + x, map.height * MINIMAPSCALE - (int)(pEntity->y / (16.f / MINIMAPSCALE)) - y);
						glEnd();
					}
				}
			}
			else
			{
				minotaur_timer = 0;
			}
		}
	}*/
}