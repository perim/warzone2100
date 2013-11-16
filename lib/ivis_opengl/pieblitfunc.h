/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/***************************************************************************/
/*
 * pieBlitFunc.h
 *
 * patch for exisitng ivis rectangle draw functions.
 *
 */
/***************************************************************************/

#ifndef _pieBlitFunc_h
#define _pieBlitFunc_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "texture-font.h"
#include "textdraw.h"
#include "piedef.h"
#include "piepalette.h"

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
#define NUM_BACKDROPS 7

/* These are Qamly's hacks used for map previews. They need to be power of
 * two sized until we clean up this mess properly. */
#define BACKDROP_HACK_WIDTH 512
#define BACKDROP_HACK_HEIGHT 512

/***************************************************************************/
/*
 *	Global Classes
 */
/***************************************************************************/

enum GFXTYPE
{
	GFX_TEXTURE,
	GFX_COLOUR,
	GFX_COLOUR_BLEND,
	GFX_TEXTURE_PERSISTENT, // texture handled by tex.cpp
	GFX_TEXTURE_INDEXED,  // used for text rendering
	GFX_COUNT
};

/// Generic graphics using VBOs drawing class
class GFX
{
public:
	/// Initialize class and allocate GPU resources
	GFX(GFXTYPE type, GLenum drawType = GL_TRIANGLES, int coordsPerVertex = 3);

	/// Destroy GPU held resources
	~GFX();

	/// Load texture data from file, allocate space for it, and put it on the GPU
	void loadTexture(const char *filename, GLenum filter = GL_LINEAR);

	/// Allocate space on the GPU for texture of given parameters. If image is non-NULL,
	/// then that memory buffer is uploaded to the GPU.
	void makeTexture(int width, int height, GLenum filter = GL_LINEAR, GLenum format = GL_RGBA, const GLvoid *image = NULL);

	/// Upload given memory buffer to already allocated texture space on the GPU
	void updateTexture(const GLvoid *image, int width = -1, int height = -1);

	/// Associate a texture in the global texture table to this graphics object.
	void associateTexture(int i);

	/// Upload vertex and texture buffer data to the GPU
	void buffers(int vertices, const GLvoid *vertBuf, const GLvoid *texBuf, const GLvoid *auxBuf2 = NULL, int polygons = 0, const uint16_t *indices = NULL);

	/// Draw everything
	void draw();

private:
	GFXTYPE mType;
	GLenum mFormat;
	int mWidth;
	int mHeight;
	GLenum mDrawType;
	int mCoordsPerVertex;
	GLuint mBuffers[VBO_COUNT];
	GLuint mTexture;
	int mSize;
};

/// Queued job for the graphics drawing class. We store jobs here until we are ready
/// to actually draw something to avoid unnecessary bus traffic and to be able to compact
/// the draw calls as much as possible.
struct GFXJob
{
	GFXJob() : vertices(0), texPage(-1), polygons(-1), texColour(WZCOL_WHITE), task(NULL) {}

	GFXTYPE type;
	GLenum drawType;
	int coordsPerVertex;
	int vertices;
	int animation; // animation if non-zero
	QVector<GLfloat> verts;
	QVector<GLfloat> texcoords;
	QVector<UBYTE> colours;
	QVector<uint16_t> indices;
	int texPage;
	int polygons;
	PIELIGHT texColour;
	GFX *task;
};

/// Draw the text right justified
#define TEXT_JUSTIFY_RIGHT    1
/// Draw the text center justified
#define TEXT_JUSTIFY_CENTER   2
/// Draw the text downwards
#define TEXT_ROTATE_270       4
/// Truncate the text at width ending with '...'
#define TEXT_TRUNCATE         8

class GFXQueue
{
public:
	~GFXQueue();
	void draw(); // draw all
	void clear();

	void line(float x0, float y0, float x1, float y1, PIELIGHT colour);
	void rect(float x0, float y0, float x1, float y1, PIELIGHT colour, GFXTYPE type = GFX_COLOUR, int animation = 0); // pie_BoxFill replacement
	void shadowBox(float x0, float y0, float x1, float y1, float pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill);
	void box(float x0, float y0, float x1, float y1, PIELIGHT first, PIELIGHT second);
	void transBoxFill(float x0, float y0, float x1, float y1, PIELIGHT colour = WZCOL_TRANSPARENT_BOX);
	void imageFile(IMAGEFILE *imageFile, int id, float x, float y, PIELIGHT colour = WZCOL_WHITE);
	void imageFile(QString filename, float x, float y, float width = -0.0f, float height = -0.0f);
	void imageFileTc(Image image, Image imageTc, int x, int y, PIELIGHT colour);
	void imageFile(Image image, float x, float y) { imageFile(image.images, image.id, x, y); }
	void drawBlueBox(float x, float y, float w, float h) { rect(x - 1, y - 1, x + w + 1, y + h + 1, WZCOL_MENU_BORDER); rect(x, y , x + w, y + h, WZCOL_MENU_BACKGROUND); }
	Vector2f text(iV_fonts fontType, const QString &text, float x, float y, PIELIGHT colour = WZCOL_FORM_TEXT, int flags = 0, float width = 0.0f);
	Vector2f textSize(iV_fonts fontType, const QString &text);
	float textLineSize(iV_fonts fontType);
	float textAboveBase(iV_fonts fontType);
	float textBelowBase(iV_fonts fontType);

private:
	QList<GFXJob> jobs; // queued up jobs, sort and merge into tasks on demand
	GFXJob &findJob(GFXTYPE type, GLenum drawType, int coordsPerVertex, int texPage = -1, PIELIGHT texColour = WZCOL_WHITE, int animation = 0);
};

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
void iV_ShadowBox(int x0, int y0, int x1, int y1, int pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill);
extern void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour);
extern void iV_Box2(int x0,int y0, int x1, int y1, PIELIGHT first, PIELIGHT second);
static inline void iV_Box(int x0,int y0, int x1, int y1, PIELIGHT first) { iV_Box2(x0, y0, x1, y1, first, first); }
extern void pie_BoxFill(int x0,int y0, int x1, int y1, PIELIGHT colour);
extern void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y);
void iV_DrawImage2(const QString &filename, float x, float y, float width = -0.0f, float height = -0.0f);
void iV_DrawImageTc(Image image, Image imageTc, int x, int y, PIELIGHT colour);
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width);
void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height);

static inline void iV_DrawImage(Image image, int x, int y) { iV_DrawImage(image.images, image.id, x, y); }
static inline void iV_DrawImageTc(IMAGEFILE *imageFile, unsigned id, unsigned idTc, int x, int y, PIELIGHT colour) { iV_DrawImageTc(Image(imageFile, id), Image(imageFile, idTc), x, y, colour); }

extern void iV_TransBoxFill(float x0, float y0, float x1, float y1);
extern void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT colour);

bool pie_InitGraphics();
bool pie_ShutdownGraphics();

void pie_DownLoadRadar(UDWORD *buffer);
void pie_RenderRadar();
void pie_SetRadar(GLfloat x, GLfloat y, GLfloat width, GLfloat height, int twidth, int theight, bool filter);

enum SCREENTYPE
{
	SCREEN_RANDOMBDROP,
	SCREEN_CREDITS,
	SCREEN_MISSIONEND,
};

extern void pie_LoadBackDrop(SCREENTYPE screenType);

#endif //
