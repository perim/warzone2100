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
 * pieBlitFunc.c
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/gamelib/gtime.h"
#include <time.h>

#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/tex.h"
#include "piematrix.h"
#include "screen.h"

/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

static GFX *radarGfx = NULL;
static texture_font_t *fonts[font_count] = { NULL };

/***************************************************************************/
/*
 *	Static function forward declarations
 */
/***************************************************************************/

static bool assertValidImage(IMAGEFILE *imageFile, unsigned id);
static Vector2i makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest = NULL, int x = 0, int y = 0);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

GFX::GFX(GFXTYPE type, GLenum drawType, int coordsPerVertex) : mType(type), mDrawType(drawType), mCoordsPerVertex(coordsPerVertex), mSize(0)
{
	if (type == GFX_TEXTURE_INDEXED)
	{
		glGenBuffers(VBO_COUNT, mBuffers);
	}
	else
	{
		glGenBuffers(VBO_MINIMAL, mBuffers);
	}
	if (type == GFX_TEXTURE)
	{
		glGenTextures(1, &mTexture);
	}
}

void GFX::associateTexture(int i) // refers to texture in tex.cpp table
{
	ASSERT(mType == GFX_TEXTURE_PERSISTENT || mType == GFX_TEXTURE_INDEXED, "Bad GFX type");
	ASSERT(mType != GFX_TEXTURE_PERSISTENT || (i >= 0 && i < pie_NumberOfPages()), "Bad GFX parameter");
	mTexture = i;
}

void GFX::loadTexture(const char *filename, GLenum filter)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	const char *extension = strrchr(filename, '.'); // determine the filetype
	iV_Image image;
	if (!extension || strcmp(extension, ".png") != 0)
	{
		debug(LOG_ERROR, "Bad image filename: %s", filename);
		return;
	}
	if (iV_loadImage_PNG(filename, &image))
	{
		makeTexture(image.width, image.height, filter, iV_getPixelFormat(&image), image.bmp);
		iV_unloadImage(&image);
	}
}

void GFX::makeTexture(int width, int height, GLenum filter, GLenum format, const GLvoid *image)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	mWidth = width;
	mHeight = height;
	mFormat = format;
}

void GFX::updateTexture(const void *image, int width, int height)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	if (width == -1) width = mWidth;
	if (height == -1) height = mHeight;
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, mFormat, GL_UNSIGNED_BYTE, image);
}

static inline int verticesPerPolygon(GLenum drawType)
{
	switch (drawType)
	{
	case GL_TRIANGLES: return 3;
	case GL_LINES: return 2;
	case GL_POINTS: return 1;
	default: ASSERT(false, "Wrong draw type for indexed mode");
	}
	return -1;
}

void GFX::buffers(int vertices, const GLvoid *vertBuf, const GLvoid *auxBuf, const GLvoid *auxBuf2, int polygons, const uint16_t *indices)
{
	glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, vertices * mCoordsPerVertex * sizeof(GLfloat), vertBuf, GL_STATIC_DRAW);
	if (mType == GFX_TEXTURE || mType == GFX_TEXTURE_PERSISTENT || mType == GFX_TEXTURE_INDEXED)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]);
		glBufferData(GL_ARRAY_BUFFER, vertices * 2 * sizeof(GLfloat), auxBuf, GL_STATIC_DRAW);
	}
	else if (mType == GFX_COLOUR || mType == GFX_COLOUR_BLEND)
	{
		// reusing texture buffer for colours for now
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]);
		glBufferData(GL_ARRAY_BUFFER, vertices * 4 * sizeof(GLbyte), auxBuf, GL_STATIC_DRAW);
	}
	mSize = vertices;
	if (mType == GFX_TEXTURE_INDEXED)
	{
		// reusing normal buffer for colours for now... original design starting to show its limits
		ASSERT(polygons > 0 && indices != NULL, "No index parameters given");
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_NORMAL]);
		glBufferData(GL_ARRAY_BUFFER, vertices * 4 * sizeof(GLbyte), auxBuf2, GL_STATIC_DRAW);
		const int vtsPerPoly = verticesPerPolygon(mDrawType);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffers[VBO_INDEX]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygons * vtsPerPoly * sizeof(uint16_t), indices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		mSize = polygons * vtsPerPoly;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GFX::draw()
{
	if (mType == GFX_TEXTURE_PERSISTENT)
	{
		pie_SetRendMode(REND_ALPHA); // used for GUI stuff
		pie_SetTexturePage(mTexture);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}
	else if (mType == GFX_TEXTURE)
	{
		pie_SetRendMode(REND_OPAQUE);
		pie_SetTexturePage(TEXPAGE_EXTERN);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}
	else if (mType == GFX_COLOUR || mType == GFX_COLOUR_BLEND)
	{
		if (mType == GFX_COLOUR)
		{
			// breaks the radar frustum window
			//pie_SetRendMode(REND_OPAQUE);
		}
		else
		{
			pie_SetRendMode(REND_ALPHA);
		}
		pie_SetTexturePage(TEXPAGE_NONE);
		glEnableClientState(GL_COLOR_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]); glColorPointer(4, GL_UNSIGNED_BYTE, 0, NULL);
	}
	else if (mType == GFX_TEXTURE_INDEXED)
	{
		// TODO, alpha blending parameters hard-coded here and for the two other texture types for now,
		// un-hard-code this later by setting it explicitly...
		pie_SetRendMode(REND_ALPHA);
		pie_SetTexturePage(TEXPAGE_EXTERN);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glBindTexture(GL_TEXTURE_2D, mTexture);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_NORMAL]); glColorPointer(4, GL_UNSIGNED_BYTE, 0, NULL); // colours... (sic)
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_TEXCOORD]); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffers[VBO_INDEX]);
		glEnableClientState(GL_VERTEX_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_VERTEX]); glVertexPointer(mCoordsPerVertex, GL_FLOAT, 0, NULL);
		glDrawElements(mDrawType, mSize, GL_UNSIGNED_SHORT, 0);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glErrors();
		return;
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, mBuffers[VBO_VERTEX]); glVertexPointer(mCoordsPerVertex, GL_FLOAT, 0, NULL);
	glDrawArrays(mDrawType, 0, mSize);
	glDisableClientState(GL_VERTEX_ARRAY);
	if (mType == GFX_TEXTURE || mType == GFX_TEXTURE_PERSISTENT || mType == GFX_TEXTURE_INDEXED)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else if (mType == GFX_COLOUR || mType == GFX_COLOUR_BLEND)
	{
		glDisableClientState(GL_COLOR_ARRAY);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GFX::~GFX()
{
	if (mType != GFX_TEXTURE_INDEXED)
	{
		glDeleteBuffers(VBO_MINIMAL, mBuffers);
	}
	else
	{
		glDeleteBuffers(VBO_COUNT, mBuffers);
	}
	if (mType == GFX_TEXTURE)
	{
		glDeleteTextures(1, &mTexture); // 'cos we own this texture
	}
}

/***************************************************************************/

GFXQueue::~GFXQueue()
{
	clear();
}

void GFXQueue::clear()
{
	for (int i = 0; i < jobs.size(); i++)
	{
		delete jobs[i].task;
	}
	jobs.clear();
}

GFXJob &GFXQueue::findJob(GFXTYPE type, GLenum drawType, int coordsPerVertex, int texPage, PIELIGHT texColour, int animation)
{
	(void)texPage; // when we start reusing texture buffers, this will be handy
	// Try to reuse existing entry. For now, do not try to combine texture tasks or
	// strip draw types. TODO: Allow more reuse if we can use the primitive restart extension
	// (a core feature of OpenGL 3.1+ and OpenGL ES 3.0).
	if ((type == GFX_COLOUR || type == GFX_COLOUR_BLEND)
	    && (drawType == GL_POINTS || drawType == GL_LINES || drawType == GL_TRIANGLES))
	{
		for (int i = 0; i < jobs.size(); i++)
		{
			if (jobs[i].type == type
			    && jobs[i].drawType == drawType
			    && jobs[i].coordsPerVertex == coordsPerVertex
			    && jobs[i].animation == animation)
			{
				return jobs[i];
			}
		}
	}
	// no applicable entry found, make a new entry for this type of draw
	GFXJob job;
	job.type = type;
	job.texColour = texColour;
	job.drawType = drawType;
	job.coordsPerVertex = coordsPerVertex;
	job.animation = animation;
	jobs.append(job);
	return jobs[jobs.size() - 1];
}

void GFXQueue::draw()
{
	for (int i = 0; i < jobs.size(); i++)
	{
		if (!jobs[i].task) // create actual GPU data on first draw call
		{
			GFXJob &job = jobs[i];
			GFX *task = new GFX(job.type, job.drawType, job.coordsPerVertex);
			if (job.type == GFX_TEXTURE_PERSISTENT)
			{
				ASSERT(job.texPage >= 0 && job.texPage < pie_NumberOfPages(), "Bad page number: %d", job.texPage);
				task->associateTexture(job.texPage);
			}
			if (job.type == GFX_TEXTURE || job.type == GFX_TEXTURE_PERSISTENT)
			{
				task->buffers(job.vertices, job.verts.data(), job.texcoords.data());
			}
			else if (job.type != GFX_TEXTURE_INDEXED)
			{
				task->buffers(job.vertices, job.verts.data(), job.colours.data());
			}
			else // indexed texture drawing
			{
				task->associateTexture(job.texPage);
				task->buffers(job.vertices, job.verts.data(), job.texcoords.data(), job.colours.data(), job.polygons, job.indices.data());
			}
			job.verts.clear();
			job.texcoords.clear();
			job.colours.clear();
			job.indices.clear();
			jobs[i].task = task;
		}
		glColor4ubv(jobs[i].texColour.vector); // FIXME hack until we have got rid of global colour...
		if ((jobs[i].animation > 0 && jobs[i].animation > realTime % 1000)
		    || (jobs[i].animation < 0 && jobs[i].animation <= realTime % 1000))
		{
			continue; // make blink animation
		}
		jobs[i].task->draw();
	}
}

Vector2f GFXQueue::textSize(iV_fonts fontType, const QString &text)
{
	const int length = text.size();
	texture_font_t *font = fonts[fontType];
	Vector2f pen = { 0, 0 };
	QVector<uint> data = text.toUcs4();
	float height = 0.0f;
	for (int i = 0; i < length; i++)
	{
		texture_glyph_t *glyph = texture_font_get_glyph(font, data[i]);
		if (glyph != NULL)
		{
			int kerning = 0;
			if (i > 0)
			{
				kerning = texture_glyph_get_kerning(glyph, data[i - 1]);
			}
			pen.x += kerning + glyph->advance_x;
			height = MAX(height, glyph->advance_y);
		}
	}
	pen.y += height;
	return pen;
}

// cannot inline since 'fonts' is static here
float GFXQueue::textLineSize(iV_fonts fontType) { return fonts[fontType]->height; }
float GFXQueue::textAboveBase(iV_fonts fontType) { return -fonts[fontType]->ascender; }
float GFXQueue::textBelowBase(iV_fonts fontType) { return -fonts[fontType]->descender; }

Vector2f GFXQueue::text(iV_fonts fontType, const QString &text, float x, float y, PIELIGHT colour, int flags, float width)
{
	Vector2f pen = { x, y };
	Vector2f origin = pen;
	const int length = text.size();
	if (length == 0)
	{
		return pen;
	}
	(void)width; // TODO
	GFXJob &job = findJob(GFX_TEXTURE_INDEXED, GL_TRIANGLES, 2);
	texture_font_t *font = fonts[fontType];
	job.texPage = font->atlas->id;
	job.vertices = length * 4;
	job.polygons = length * 2;
	job.verts.reserve(job.vertices * 2); // reserve sufficient space for data in advance
	job.texcoords.reserve(job.vertices * 2);
	job.indices.reserve(length * 3 * 2);
	job.colours.reserve(length * 4 * 4);
	const QVector<uint> data = text.toUcs4();
	for (int i = 0; i < length; i++)
	{
		texture_glyph_t *glyph = texture_font_get_glyph(font, data[i]);
		if (glyph != NULL)
		{
			int kerning = 0;
			if (i > 0)
			{
				kerning = texture_glyph_get_kerning(glyph, data[i - 1]);
			}
			if (flags & TEXT_ROTATE_270)
			{
				pen.y -= kerning;
				const float x0 = pen.x - glyph->offset_y;
				const float y0 = pen.y - glyph->offset_x - glyph->width;
				const float x1 = x0 + glyph->height;
				const float y1 = y0 + glyph->width;
				job.verts += { x1, y1,  x0, y1,  x0, y0,  x1, y0 };
				pen.y -= glyph->advance_x;
			}
			else
			{
				pen.x += kerning;
				const float x0 = pen.x + glyph->offset_x;
				const float y0 = pen.y - glyph->offset_y;
				const float x1 = x0 + glyph->width;
				const float y1 = y0 + glyph->height;
				job.verts += { x0, y1,  x0, y0,  x1, y0,  x1, y1 };
				pen.x += glyph->advance_x;
			}
			const float s0 = glyph->s0;
			const float t0 = glyph->t0;
			const float s1 = glyph->s1;
			const float t1 = glyph->t1;
			job.texcoords += { s0, t1,  s0, t0,  s1, t0,  s1, t1 };
			uint16_t idx = i * 4;
			job.indices += { idx, (uint16_t)(idx + 1), (uint16_t)(idx + 2), idx, (uint16_t)(idx + 2), (uint16_t)(idx + 3) };
			for (int j = 0; j < 4; j++) job.colours += { colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a };
		}
	}
	return pen - origin;
}

void GFXQueue::line(float x0, float y0, float x1, float y1, PIELIGHT colour, int animation)
{
	const int vertices = 2;
	GFXJob &job = findJob(GFX_COLOUR, GL_LINES, 2, -1, WZCOL_WHITE, animation);
	job.verts += { x0, y0, x1, y1 };
	job.vertices += vertices;
	for (int i = vertices; i-- > 0;) job.colours += { colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a };
}

void GFXQueue::rect(float x0, float y0, float x1, float y1, PIELIGHT colour, GFXTYPE type, int animation) // colour filled rectangle
{
	const int vertices = 4;
	GFXJob &job = findJob(type, GL_TRIANGLE_STRIP, 2, -1, WZCOL_WHITE, animation);
	job.verts += { x0, y0, x1, y0, x0, y1, x1, y1 };
	job.vertices += vertices;
	for (int i = vertices; i-- > 0;) job.colours += { colour.byte.r, colour.byte.g, colour.byte.b, colour.byte.a };
}

void GFXQueue::shadowBox(float x0, float y0, float x1, float y1, float pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill)
{
	rect(x0 + pad, y0 + pad, x1 - pad, y1 - pad, fill, GFX_COLOUR);
	box(x0, y0, x1, y1, first, second);
}

void GFXQueue::box(float x0, float y0, float x1, float y1, PIELIGHT first, PIELIGHT second)
{
	const int vertices = 8;
	GFXJob &job = findJob(GFX_COLOUR, GL_LINES, 2);
	job.verts += { x0, y1, x0, y0, x0, y0, x1, y0, x1, y0, x1, y1, x0, y1, x1, y1 };
	job.vertices += vertices;
	for (int i = (vertices + 1) / 2; i-- > 0;) job.colours += { first.byte.r, first.byte.g, first.byte.b, first.byte.a };
	for (int i = vertices / 2; i-- > 0;) job.colours += { second.byte.r, second.byte.g, second.byte.b, second.byte.a };
}

void GFXQueue::transBoxFill(float x0, float y0, float x1, float y1, PIELIGHT colour)
{
	rect(x0, y0, x1, y1, colour, GFX_COLOUR_BLEND);
}

// internal
void GFXQueue::makeImageFile(Vector2i size, PIERECT dest, const ImageDef *image, PIELIGHT colour)
{
	const int vertices = 4;
	const GLuint texPage = image->textureId;
	const GLfloat invTextureSize = image->invTextureSize;
	const int tu = image->Tu;
	const int tv = image->Tv;
	GFXJob &job = findJob(GFX_TEXTURE_PERSISTENT, GL_TRIANGLE_STRIP, 2, texPage, colour);
	job.texPage = texPage;
	job.verts += { dest.x, dest.y, dest.x + dest.w, dest.y, dest.x, dest.y + dest.h, dest.x + dest.w, dest.y + dest.h };
	job.texcoords += { tu * invTextureSize, tv * invTextureSize, (tu + size.x) * invTextureSize, tv * invTextureSize,
			   tu * invTextureSize, (tv + size.y) * invTextureSize, (tu + size.x) * invTextureSize, (tv + size.y) * invTextureSize };
	job.vertices += vertices;
}

void GFXQueue::imageFile(IMAGEFILE *imageFile, int id, float x, float y, PIELIGHT colour)
{
	if (!assertValidImage(imageFile, id))
	{
		return;
	}
	PIERECT dest;
	const Vector2i size = makePieImage(imageFile, id, &dest, x, y);
	makeImageFile(size, dest, &imageFile->imageDefs[id], colour);
}

void GFXQueue::imageFileRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width)
{
	int hRep, hRemainder;
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}
	const ImageDef *Image = &ImageFile->imageDefs[ID];
	PIERECT dest;
	Vector2i size = makePieImage(ImageFile, ID, &dest, x, y);
	hRemainder = Width % Image->Width;
	for (hRep = 0; hRep < Width / Image->Width; hRep++)
	{
		makeImageFile(size, dest, Image, WZCOL_WHITE);
		dest.x += Image->Width;
	}
	if (hRemainder > 0)	// draw remainder
	{
		size.x = hRemainder;
		dest.w = hRemainder;
		makeImageFile(size, dest, Image, WZCOL_WHITE);
	}
}

void GFXQueue::imageFileRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height)
{
	int vRep, vRemainder;
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}
	const ImageDef *Image = &ImageFile->imageDefs[ID];
	PIERECT dest;
	Vector2i size = makePieImage(ImageFile, ID, &dest, x, y);
	vRemainder = Height % Image->Height;
	for (vRep = 0; vRep < Height / Image->Height; vRep++)
	{
		makeImageFile(size, dest, Image, WZCOL_WHITE);
		dest.y += Image->Height;
	}
	if (vRemainder > 0)	// draw remainder
	{
		size.y = vRemainder;
		dest.h = vRemainder;
		makeImageFile(size, dest, Image, WZCOL_WHITE);
	}
}

void GFXQueue::imageFile(QString filename, float x, float y, float width, float height)
{
	const ImageDef *image = iV_GetImage(filename, x, y);
	const GLfloat w = width > 0 ? width : image->Width;
	const GLfloat h = height > 0 ? height : image->Height;
	PIERECT rect = { x, y, w, h };
	makeImageFile(Vector2i(image->Width, image->Height), rect, image, WZCOL_WHITE);
}

void GFXQueue::imageFileTc(Image image, Image imageTc, int x, int y, PIELIGHT colour)
{
	imageFile(image.images, image.id, x, y);
	imageFile(imageTc.images, imageTc.id, x, y, colour);
}

/***************************************************************************/

void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);

	glColor4ubv(colour.vector);
	glBegin(GL_LINES);
	glVertex2i(x0, y0);
	glVertex2i(x1, y1);
	glEnd();
}

/**
 *	Assumes render mode set up externally, draws filled rectangle.
 */
static void pie_DrawRect(float x0, float y0, float x1, float y1, PIELIGHT colour)
{
	glColor4ubv(colour.vector);
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(x0, y0);
		glVertex2f(x1, y0);
		glVertex2f(x0, y1);
		glVertex2f(x1, y1);
	glEnd();
}

void iV_ShadowBox(int x0, int y0, int x1, int y1, int pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill)
{
	pie_SetRendMode(REND_OPAQUE);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0 + pad, y0 + pad, x1 - pad, y1 - pad, fill);
	iV_Box2(x0, y0, x1, y1, first, second);
}

/***************************************************************************/

void iV_Box2(int x0,int y0, int x1, int y1, PIELIGHT first, PIELIGHT second)
{
	pie_SetTexturePage(TEXPAGE_NONE);

	glColor4ubv(first.vector);
	glBegin(GL_LINES);
	glVertex2i(x0, y1);
	glVertex2i(x0, y0);
	glVertex2i(x0, y0);
	glVertex2i(x1, y0);
	glEnd();
	glColor4ubv(second.vector);
	glBegin(GL_LINES);
	glVertex2i(x1, y0);
	glVertex2i(x1, y1);
	glVertex2i(x0, y1);
	glVertex2i(x1, y1);
	glEnd();
}

/***************************************************************************/

void pie_BoxFill(int x0,int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetRendMode(REND_OPAQUE);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0, y0, x1, y1, colour);
}

/***************************************************************************/

void iV_TransBoxFill(float x0, float y0, float x1, float y1)
{
	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_TRANSPARENT_BOX);
}

/***************************************************************************/

void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT light)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ALPHA);
	pie_DrawRect(x0, y0, x1, y1, light);
}

/***************************************************************************/

static bool assertValidImage(IMAGEFILE *imageFile, unsigned id)
{
	ASSERT_OR_RETURN(false, id < imageFile->imageDefs.size(), "Out of range 1: %u/%d", id, (int)imageFile->imageDefs.size());
	ASSERT_OR_RETURN(false, imageFile->imageDefs[id].TPageID < imageFile->pages.size(), "Out of range 2: %u", imageFile->imageDefs[id].TPageID);
	return true;
}

static void pie_DrawImage(IMAGEFILE *imageFile, int id, Vector2i size, const PIERECT *dest, PIELIGHT colour = WZCOL_WHITE)
{
	ImageDef const &image2 = imageFile->imageDefs[id];
	GLuint texPage = imageFile->pages[image2.TPageID].id;
	GLfloat invTextureSize = 1.f / imageFile->pages[image2.TPageID].size;
	int tu = image2.Tu;
	int tv = image2.Tv;

	pie_SetTexturePage(texPage);
	glColor4ubv(colour.vector);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(tu * invTextureSize, tv * invTextureSize);
		glVertex2f(dest->x, dest->y);

		glTexCoord2f((tu + size.x) * invTextureSize, tv * invTextureSize);
		glVertex2f(dest->x + dest->w, dest->y);

		glTexCoord2f(tu * invTextureSize, (tv + size.y) * invTextureSize);
		glVertex2f(dest->x, dest->y + dest->h);

		glTexCoord2f((tu + size.x) * invTextureSize, (tv + size.y) * invTextureSize);
		glVertex2f(dest->x + dest->w, dest->y + dest->h);
	glEnd();
}

static Vector2i makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest, int x, int y)
{
	ImageDef const &image = imageFile->imageDefs[id];
	Vector2i pieImage;
	pieImage.x = image.Width;
	pieImage.y = image.Height;
	if (dest != NULL)
	{
		dest->x = x + image.XOffset;
		dest->y = y + image.YOffset;
		dest->w = image.Width;
		dest->h = image.Height;
	}
	return pieImage;
}

void iV_DrawImage2(const QString &filename, float x, float y, float width, float height)
{
	ImageDef *image = iV_GetImage(filename, x, y);
	const GLfloat invTextureSize = image->invTextureSize;
	const int tu = image->Tu;
	const int tv = image->Tv;
	const int w = width > 0 ? width : image->Width;
	const int h = height > 0 ? height : image->Height;
	x += image->XOffset;
	y += image->YOffset;
	pie_SetTexturePage(image->textureId);
	glColor4ubv(WZCOL_WHITE.vector);
	pie_SetRendMode(REND_ALPHA);
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(tu * image->invTextureSize, tv * invTextureSize);
		glVertex2f(x, y);

		glTexCoord2f((tu + image->Width) * invTextureSize, tv * invTextureSize);
		glVertex2f(x + w, y);

		glTexCoord2f(tu * invTextureSize, (tv + image->Height) * invTextureSize);
		glVertex2f(x, y + h);

		glTexCoord2f((tu + image->Width) * invTextureSize, (tv + image->Height) * invTextureSize);
		glVertex2f(x + w, y + h);
	glEnd();
}

void iV_DrawImage(IMAGEFILE *ImageFile, UWORD ID, int x, int y)
{
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	pie_SetRendMode(REND_ALPHA);

	pie_DrawImage(ImageFile, ID, pieImage, &dest);
}

void iV_DrawImageTc(Image image, Image imageTc, int x, int y, PIELIGHT colour)
{
	if (!assertValidImage(image.images, image.id) || !assertValidImage(imageTc.images, imageTc.id))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage   = makePieImage(image.images, image.id, &dest, x, y);
	Vector2i pieImageTc = makePieImage(imageTc.images, imageTc.id);

	pie_SetRendMode(REND_ALPHA);

	pie_DrawImage(image.images, image.id, pieImage, &dest);
	pie_DrawImage(imageTc.images, imageTc.id, pieImageTc, &dest, colour);
}

// Repeat a texture
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Width)
{
	int hRep, hRemainder;

	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	hRemainder = Width % Image->Width;

	for (hRep = 0; hRep < Width / Image->Width; hRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
		dest.x += Image->Width;
	}

	// draw remainder
	if (hRemainder > 0)
	{
		pieImage.x = hRemainder;
		dest.w = hRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
	}
}

void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, UWORD ID, int x, int y, int Height)
{
	int vRep, vRemainder;

	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	vRemainder = Height % Image->Height;

	for (vRep = 0; vRep < Height / Image->Height; vRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
		dest.y += Image->Height;
	}

	// draw remainder
	if (vRemainder > 0)
	{
		pieImage.y = vRemainder;
		dest.h = vRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
	}
}

bool pie_InitGraphics()
{
	// Initialize fonts
	// TODO: map language -> font file from config file
	texture_atlas_t *atlas = texture_atlas_new(512, 512, 1);

	fonts[font_regular] = texture_font_new(atlas, "data/fonts/robotoslab-regular.ttf", 13);
	fonts[font_small] = texture_font_new(atlas, "data/fonts/robotoslab-light.ttf", 10);
	fonts[font_large] = texture_font_new(atlas, "data/fonts/robotoslab-bold.ttf", 22);
	fonts[font_scaled] = texture_font_new(atlas, "data/fonts/robotoslab-regular.ttf", 13.f * pie_GetVideoBufferHeight() / 480);
	const wchar_t *cache = L" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
			       L"`abcdefghijklmnopqrstuvwxyz{|}~";
	texture_font_load_glyphs(fonts[font_regular], cache);
	texture_font_load_glyphs(fonts[font_small], cache);
	texture_font_load_glyphs(fonts[font_large], cache);
	texture_font_load_glyphs(fonts[font_scaled], cache);
	debug(LOG_ERROR, "Font texture occupancy: %.2f%%", 100.0 * atlas->used / (float)(atlas->width * atlas->height));

	// Initialize radar
	radarGfx = new GFX(GFX_TEXTURE, GL_TRIANGLE_STRIP, 2);
	return true;
}

bool pie_ShutdownGraphics()
{
	texture_font_delete(fonts[font_regular]);
	texture_font_delete(fonts[font_small]);
	texture_font_delete(fonts[font_large]);
	texture_font_delete(fonts[font_scaled]);
	memset(fonts, 0, sizeof(fonts));

	delete radarGfx; radarGfx = NULL;
	return true;
}

void pie_SetRadar(GLfloat x, GLfloat y, GLfloat width, GLfloat height, int twidth, int theight, bool filter)
{
	radarGfx->makeTexture(twidth, theight, filter ? GL_LINEAR : GL_NEAREST);
	GLfloat texcoords[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
	GLfloat vertices[] = { x, y,  x + width, y,  x, y + height,  x + width, y + height };
	radarGfx->buffers(4, vertices, texcoords);
}

/** Store radar texture with given width and height. */
void pie_DownLoadRadar(UDWORD *buffer)
{
	radarGfx->updateTexture(buffer);
}

/** Display radar texture using the given height and width, depending on zoom level. */
void pie_RenderRadar()
{
	pie_SetRendMode(REND_ALPHA);
	glColor4ubv(WZCOL_WHITE.vector); // hack
	radarGfx->draw();
}

void pie_LoadBackDrop(SCREENTYPE screenType)
{
	char backd[128];

	//randomly load in a backdrop piccy.
	srand( (unsigned)time(NULL) + 17 ); // Use offset since time alone doesn't work very well

	switch (screenType)
	{
		case SCREEN_RANDOMBDROP:
			snprintf(backd, sizeof(backd), "texpages/bdrops/backdrop%i.png", rand() % NUM_BACKDROPS); // Range: 0 to (NUM_BACKDROPS-1)
			break;
		case SCREEN_MISSIONEND:
			sstrcpy(backd, "texpages/bdrops/missionend.png");
			break;

		case SCREEN_CREDITS:
		default:
			sstrcpy(backd, "texpages/bdrops/credits.png");
			break;
	}

	screen_SetBackDropFromFile(backd);
}
