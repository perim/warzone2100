/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
/** \file
 *  Render routines for 3D coloured and shaded transparency rendering.
 */

#include <GLee.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/imd.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/pieclip.h"
#include "piematrix.h"
#include "screen.h"

#define SHADOW_END_DISTANCE (8000*8000) // Keep in sync with lighting.c:FOG_END

#define VERTICES_PER_TRIANGLE 3
#define COLOUR_COMPONENTS 4
#define TEXCOORD_COMPONENTS 2
#define VERTEX_COMPONENTS 3
#define TRIANGLES_PER_TILE 2
#define VERTICES_PER_TILE (TRIANGLES_PER_TILE * VERTICES_PER_TRIANGLE)

extern BOOL drawing_interface;

/*
 *	Local Variables
 */

static unsigned int pieCount = 0;
static unsigned int tileCount = 0;
static unsigned int polyCount = 0;
static bool lighting = false;
static bool lightingstate = false;
static bool shadows = false;

/*
 *	Source
 */

void pie_BeginLighting(const Vector3f * light, bool drawshadows)
{
	const float pos[4] = {light->x, light->y, light->z, 0.0f};
	const float zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	const float ambient[4] = {0.3f, 0.3f, 0.3f, 1.0f};
	const float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
	const float specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glEnable(GL_LIGHT0);

	lighting = lightingstate;
	if (drawshadows)
	{
		shadows = true;
	}
}

bool pie_GetLightingState(void)
{
	return lightingstate;
}

void pie_SetLightingState(bool val)
{
	lightingstate = val;
}

void pie_EndLighting(void)
{
	shadows = false;
	lighting = false;
}

/***************************************************************************
 * pie_Draw3dShape
 *
 * Project and render a pumpkin image to render surface
 * Will support zbuffering, texturing, coloured lighting and alpha effects
 * Avoids recalculating vertex projections for every poly
 ***************************************************************************/

typedef struct {
	float		matrix[16];
	iIMDShape*	shape;
	int		flag;
	int		flag_data;
	Vector3f	light;
} shadowcasting_shape_t;

typedef struct {
	float		matrix[16];
	iIMDShape*	shape;
	int		frame;
	PIELIGHT	colour;
	PIELIGHT	specular;
	int		flag;
	int		flag_data;
} transluscent_shape_t;

static shadowcasting_shape_t* scshapes = NULL;
static unsigned int scshapes_size = 0;
static unsigned int nb_scshapes = 0;
static transluscent_shape_t* tshapes = NULL;
static unsigned int tshapes_size = 0;
static unsigned int nb_tshapes = 0;

static void pie_Draw3DShape2(iIMDShape *shape, int frame, PIELIGHT colour, PIELIGHT teamcolour, WZ_DECL_UNUSED PIELIGHT specular, int pieFlag, int pieFlagData)
{
	iIMDPoly *pPolys;
	bool light = lighting;

	pie_SetAlphaTest(true);

	/* Set fog status */
	if (!(pieFlag & pie_FORCE_FOG) && 
		(pieFlag & pie_ADDITIVE || pieFlag & pie_TRANSLUCENT || pieFlag & pie_BUTTON))
	{
		pie_SetFogStatus(false);
	}
	else
	{
		pie_SetFogStatus(true);
	}

	/* Set tranlucency */
	if (pieFlag & pie_ADDITIVE)
	{
		pie_SetRendMode(REND_ADDITIVE);
		colour.byte.a = (UBYTE)pieFlagData;
		light = false;
	}
	else if (pieFlag & pie_TRANSLUCENT)
	{
		pie_SetRendMode(REND_ALPHA);
		colour.byte.a = (UBYTE)pieFlagData;
		light = false;
	}
	else
	{
		if (pieFlag & pie_BUTTON)
		{
			pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		}
		pie_SetRendMode(REND_OPAQUE);
	}

	if (light)
	{
		const float ambient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		const float diffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		const float specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		const float shininess = 10;

		glEnable(GL_LIGHTING);
		glEnable(GL_NORMALIZE);

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
	}

	if (pieFlag & pie_HEIGHT_SCALED)	// construct
	{
		glScalef(1.0f, (float)pieFlagData / (float)pie_RAISE_SCALE, 1.0f);
	}
	if (pieFlag & pie_RAISE)		// collapse
	{
		glTranslatef(1.0f, (-shape->max.y * (pie_RAISE_SCALE - pieFlagData)) * (1.0f / pie_RAISE_SCALE), 1.0f);
	}

	glColor4ubv(colour.vector);	// Only need to set once for entire model
	pie_SetTexturePage(shape->texpage);

	pie_ActivateShader_TCMask(teamcolour, shape->tcmaskpage);

	frame %= MAX(1, shape->numFrames);

	for (pPolys = shape->polys; pPolys < shape->polys + shape->npolys; pPolys++)
	{
		Vector3f	vertexCoords[pie_MAX_VERTICES_PER_POLYGON];
		unsigned int	n, fidx = frame;
		VERTEXID	*index;

		if (!(pPolys->flags & iV_IMD_TEXANIM))
		{
			fidx = 0;
		}

		for (n = 0, index = pPolys->pindex;
				n < pPolys->npnts;
				n++, index++)
		{
			vertexCoords[n].x = shape->points[*index].x;
			vertexCoords[n].y = shape->points[*index].y;
			vertexCoords[n].z = shape->points[*index].z;
		}

		polyCount++;

		glBegin(GL_TRIANGLE_FAN);

		if (light)
		{
			glNormal3fv((GLfloat*)&pPolys->normal);
		}

		for (n = 0; n < pPolys->npnts; n++)
		{
			glTexCoord2fv((GLfloat*)&pPolys->texCoord[fidx * pPolys->npnts + n]);
			glVertex3fv((GLfloat*)&vertexCoords[n]);
		}

		glEnd();
	}

	pie_DeactivateShader();

	if (pieFlag & pie_BUTTON)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	}

	if (light)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_NORMALIZE);
	}
}

/// returns true if the edges are adjacent
static int compare_edge (EDGE *A, EDGE *B, const Vector3f *pVertices )
{
	if(A->from == B->to)
	{
		if(A->to == B->from)
		{
			return true;
		}
		return Vector3f_Compare(pVertices[A->to], pVertices[B->from]);
	}

	if(!Vector3f_Compare(pVertices[A->from], pVertices[B->to]))
	{
		return false;
	}

	if(A->to == B->from)
	{
		return true;
	}
	return Vector3f_Compare(pVertices[A->to], pVertices[B->from]);
}

/// Add an edge to an edgelist
/// Makes sure only silhouette edges are present
static void addToEdgeList(int a, int b, EDGE *edgelist, unsigned int* edge_count, Vector3f *pVertices)
{
	EDGE newEdge = {a, b};
	unsigned int i;
	BOOL foundMatching = false;

	for(i = 0; i < *edge_count; i++)
	{
		if(edgelist[i].from < 0)
		{
			// does not exist anymore
			continue;
		}
		if(compare_edge(&newEdge, &edgelist[i], pVertices)) {
			// remove the other too
			edgelist[i].from = -1;
			foundMatching = true;
			break;
		}
	}
	if(!foundMatching)
	{
		edgelist[*edge_count] = newEdge;
		(*edge_count)++;
	}
}

/// scale the height according to the flags
static inline float scale_y(float y, int flag, int flag_data)
{
	float tempY = y;
	if (flag & pie_RAISE) {
		tempY = y - flag_data;
		if (y - flag_data < 0) tempY = 0;
	} else if (flag & pie_HEIGHT_SCALED) {
		if(y>0) {
			tempY = (y * flag_data)/pie_RAISE_SCALE;
		}
	}
	return tempY;
}

/// Draw the shadow for a shape
static void pie_DrawShadow(iIMDShape *shape, int flag, int flag_data, Vector3f* light)
{
	unsigned int i, j, n;
	Vector3f *pVertices;
	iIMDPoly *pPolys;
	unsigned int edge_count = 0;
	static EDGE *edgelist = NULL;
	static unsigned int edgelistsize = 256;
	EDGE *drawlist = NULL;

	if(!edgelist)
	{
		edgelist = (EDGE*)malloc(sizeof(EDGE)*edgelistsize);
	}
	pVertices = shape->points;
	if( flag & pie_STATIC_SHADOW && shape->shadowEdgeList )
	{
		drawlist = shape->shadowEdgeList;
		edge_count = shape->nShadowEdges;
	}
	else
	{

		for (i = 0, pPolys = shape->polys; i < shape->npolys; ++i, ++pPolys) {
			Vector3f p[3], v[2], normal = {0.0f, 0.0f, 0.0f};
			VERTEXID current, first;
			for(j = 0; j < 3; j++)
			{
				current = pPolys->pindex[j];
				p[j] = Vector3f_Init(pVertices[current].x, scale_y(pVertices[current].y, flag, flag_data), pVertices[current].z);
			}

			v[0] = Vector3f_Sub(p[2], p[0]);
			v[1] = Vector3f_Sub(p[1], p[0]);
			normal = Vector3f_CrossP(v[0], v[1]);
			if (Vector3f_ScalarP(normal, *light) > 0)
			{
				first = pPolys->pindex[0];
				for (n = 1; n < pPolys->npnts; n++) {
					// link to the previous vertex
					addToEdgeList(pPolys->pindex[n-1], pPolys->pindex[n], edgelist, &edge_count, pVertices);
					// check if the edgelist is still large enough
					if(edge_count >= edgelistsize-1)
					{
						// enlarge
						EDGE* newstack;
						edgelistsize *= 2;
						newstack = (EDGE *)realloc(edgelist, sizeof(EDGE) * edgelistsize);
						if (newstack == NULL)
						{
							debug(LOG_FATAL, "pie_DrawShadow: Out of memory!");
							abort();
							return;
						}

						edgelist = newstack;

						debug(LOG_WARNING, "new edge list size: %u", edgelistsize);
					}
				}
				// back to the first
				addToEdgeList(pPolys->pindex[pPolys->npnts-1], first, edgelist, &edge_count, pVertices);
			}
		}
		//debug(LOG_WARNING, "we have %i edges", edge_count);
		drawlist = edgelist;

		if(flag & pie_STATIC_SHADOW)
		{
			// first compact the current edgelist
			for(i = 0, j = 0; i < edge_count; i++)
			{
				if(edgelist[i].from < 0)
				{
					continue;
				}
				edgelist[j] = edgelist[i];
				j++;
			}
			edge_count = j;
			// then store it in the imd
			shape->nShadowEdges = edge_count;
			shape->shadowEdgeList = (EDGE *)realloc(shape->shadowEdgeList, sizeof(EDGE) * shape->nShadowEdges);
			memcpy(shape->shadowEdgeList, edgelist, sizeof(EDGE) * shape->nShadowEdges);
		}
	}

	// draw the shadow volume
	glBegin(GL_QUADS);
	for(i=0;i<edge_count;i++)
	{
		int a = drawlist[i].from, b = drawlist[i].to;
		if(a < 0)
		{
			continue;
		}

		glVertex3f(pVertices[b].x, scale_y(pVertices[b].y, flag, flag_data), pVertices[b].z);
		glVertex3f(pVertices[b].x+light->x, scale_y(pVertices[b].y, flag, flag_data)+light->y, pVertices[b].z+light->z);
		glVertex3f(pVertices[a].x+light->x, scale_y(pVertices[a].y, flag, flag_data)+light->y, pVertices[a].z+light->z);
		glVertex3f(pVertices[a].x, scale_y(pVertices[a].y, flag, flag_data), pVertices[a].z);
	}
	glEnd();

#ifdef SHOW_SHADOW_EDGES
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glColor4ub(0xFF, 0, 0, 0xFF);
	glBegin(GL_LINES);
	for(i = 0; i < edge_count; i++)
	{
		int a = drawlist[i].from, b = drawlist[i].to;
		if(a < 0)
		{
			continue;
		}

		glVertex3f(pVertices[b].x, scale_y(pVertices[b].y, flag, flag_data), pVertices[b].z);
		glVertex3f(pVertices[a].x, scale_y(pVertices[a].y, flag, flag_data), pVertices[a].z);
	}
	glEnd();
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glEnable(GL_DEPTH_TEST);
#endif
}

static void inverse_matrix(const float * src, float * dst)
{
	const float det = src[0]*src[5]*src[10] + src[4]*src[9]*src[2] + src[8]*src[1]*src[6] - src[2]*src[5]*src[8] - src[6]*src[9]*src[0] - src[10]*src[1]*src[4];
	const float invdet = 1.0f/det;

	dst[0] = invdet * (src[5]*src[10] - src[9]*src[6]);
	dst[1] = invdet * (src[9]*src[2] - src[1]*src[10]);
	dst[2] = invdet * (src[1]*src[6] - src[5]*src[2]);
	dst[3] = invdet * (src[8]*src[6] - src[4]*src[10]);
	dst[4] = invdet * (src[0]*src[10] - src[8]*src[2]);
	dst[5] = invdet * (src[4]*src[2] - src[0]*src[6]);
	dst[6] = invdet * (src[4]*src[9] - src[8]*src[5]);
	dst[7] = invdet * (src[8]*src[1] - src[0]*src[9]);
	dst[8] = invdet * (src[0]*src[5] - src[4]*src[1]);
}

void pie_SetUp(void)
{
	// initialise pie engine (just a placeholder for now)
}

void pie_CleanUp( void )
{
	free( tshapes );
	free( scshapes );
	tshapes = NULL;
	scshapes = NULL;
}

void pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, PIELIGHT specular, int pieFlag, int pieFlagData)
{
	PIELIGHT teamcolour;

	ASSERT_OR_RETURN(, shape, "Attempting to draw null sprite");

	teamcolour = pal_GetTeamColour(team);

	pieCount++;

	if (frame == 0)
	{
		frame = team;
	}

	if (drawing_interface || !shadows)
	{
		pie_Draw3DShape2(shape, frame, colour, teamcolour, specular, pieFlag, pieFlagData);
	}
	else
	{
		if (pieFlag & (pie_ADDITIVE | pie_TRANSLUCENT))
		{
			if (tshapes_size <= nb_tshapes)
			{
				if (tshapes_size == 0)
				{
					tshapes_size = 64;
					tshapes = (transluscent_shape_t*)malloc(tshapes_size*sizeof(transluscent_shape_t));
					memset( tshapes, 0, tshapes_size*sizeof(transluscent_shape_t) );
				}
				else
				{
					const unsigned int old_size = tshapes_size;
					tshapes_size <<= 1;
					tshapes = (transluscent_shape_t*)realloc(tshapes, tshapes_size*sizeof(transluscent_shape_t));
					memset( &tshapes[old_size], 0, (tshapes_size-old_size)*sizeof(transluscent_shape_t) );
				}
			}
			glGetFloatv(GL_MODELVIEW_MATRIX, tshapes[nb_tshapes].matrix);
			tshapes[nb_tshapes].shape = shape;
			tshapes[nb_tshapes].frame = frame;
			tshapes[nb_tshapes].colour = colour;
			tshapes[nb_tshapes].specular = specular;
			tshapes[nb_tshapes].flag = pieFlag;
			tshapes[nb_tshapes].flag_data = pieFlagData;
			nb_tshapes++;
		}
		else
		{
			if(pieFlag & pie_SHADOW || pieFlag & pie_STATIC_SHADOW)
			{
				float distance;

				// draw a shadow
				if (scshapes_size <= nb_scshapes)
				{
					if (scshapes_size == 0)
					{
						scshapes_size = 64;
						scshapes = (shadowcasting_shape_t*)malloc(scshapes_size*sizeof(shadowcasting_shape_t));
						memset( scshapes, 0, scshapes_size*sizeof(shadowcasting_shape_t) );
					}
					else
					{
						const unsigned int old_size = scshapes_size;
						scshapes_size <<= 1;
						scshapes = (shadowcasting_shape_t*)realloc(scshapes, scshapes_size*sizeof(shadowcasting_shape_t));
						memset( &scshapes[old_size], 0, (scshapes_size-old_size)*sizeof(shadowcasting_shape_t) );
					}
				}

				glGetFloatv(GL_MODELVIEW_MATRIX, scshapes[nb_scshapes].matrix);
				distance = scshapes[nb_scshapes].matrix[12] * scshapes[nb_scshapes].matrix[12];
				distance += scshapes[nb_scshapes].matrix[13] * scshapes[nb_scshapes].matrix[13];
				distance += scshapes[nb_scshapes].matrix[14] * scshapes[nb_scshapes].matrix[14];

				// if object is too far in the fog don't generate a shadow.
				if (distance < SHADOW_END_DISTANCE)
				{
					float invmat[9], pos_light0[4];

					inverse_matrix( scshapes[nb_scshapes].matrix, invmat );

					// Calculate the light position relative to the object
					glGetLightfv(GL_LIGHT0, GL_POSITION, pos_light0);
					scshapes[nb_scshapes].light.x = invmat[0] * pos_light0[0] + invmat[3] * pos_light0[1] + invmat[6] * pos_light0[2];
					scshapes[nb_scshapes].light.y = invmat[1] * pos_light0[0] + invmat[4] * pos_light0[1] + invmat[7] * pos_light0[2];
					scshapes[nb_scshapes].light.z = invmat[2] * pos_light0[0] + invmat[5] * pos_light0[1] + invmat[8] * pos_light0[2];

					scshapes[nb_scshapes].shape = shape;
					scshapes[nb_scshapes].flag = pieFlag;
					scshapes[nb_scshapes].flag_data = pieFlagData;

					nb_scshapes++;
				}
			}

			pie_Draw3DShape2(shape, frame, colour, teamcolour, specular, pieFlag, pieFlagData);
		}
	}
}

static void pie_ShadowDrawLoop(void)
{
	unsigned int i = 0;

	for (i = 0; i < nb_scshapes; i++)
	{
		glLoadMatrixf(scshapes[i].matrix);
		pie_DrawShadow(scshapes[i].shape, scshapes[i].flag, scshapes[i].flag_data, &scshapes[i].light);
	}
}

static void pie_DrawShadows(void)
{
	const float width = pie_GetVideoBufferWidth();
	const float height = pie_GetVideoBufferHeight();
	GLenum op_depth_pass_front = GL_INCR, op_depth_pass_back = GL_DECR;

	pie_SetTexturePage(TEXPAGE_NONE);

	glPushMatrix();

	pie_SetAlphaTest(false);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_FALSE);
	glEnable(GL_STENCIL_TEST);

	// Check if we have the required extensions
	if (GLEE_EXT_stencil_wrap)
	{
		op_depth_pass_front = GL_INCR_WRAP_EXT;
		op_depth_pass_back = GL_DECR_WRAP_EXT;
	}

	// generic 1-pass version
	if (GLEE_EXT_stencil_two_side)
	{
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
		glDisable(GL_CULL_FACE);
		glStencilMask(~0);
		glActiveStencilFaceEXT(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_back);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glActiveStencilFaceEXT(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_front);
		glStencilFunc(GL_ALWAYS, 0, ~0);

		pie_ShadowDrawLoop();
		
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}
	// check for ATI-specific 1-pass version
	else if (GLEE_ATI_separate_stencil)
	{
		glDisable(GL_CULL_FACE);
		glStencilMask(~0);
		glStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, op_depth_pass_back);
		glStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, op_depth_pass_front);
		glStencilFunc(GL_ALWAYS, 0, ~0);

		pie_ShadowDrawLoop();	
	}
	// fall back to default 2-pass version
	else
	{
		glStencilMask(~0);
		glStencilFunc(GL_ALWAYS, 0, ~0);
		glEnable(GL_CULL_FACE);
		
		// Setup stencil for front-facing polygons
		glCullFace(GL_BACK);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_front);

		pie_ShadowDrawLoop();

		// Setup stencil for back-facing polygons
		glCullFace(GL_FRONT);
		glStencilOp(GL_KEEP, GL_KEEP, op_depth_pass_back);

		pie_ShadowDrawLoop();
	}

	pie_SetRendMode(REND_ALPHA);
	glEnable(GL_CULL_FACE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilMask(~0);
	glStencilFunc(GL_LESS, 0, ~0);
	glColor4f(0, 0, 0, 0.5);

	pie_PerspectiveEnd();
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(0, 0);
		glVertex2f(width, 0);
		glVertex2f(0, height);
		glVertex2f(width, height);
	glEnd();
	pie_PerspectiveBegin();

	pie_SetRendMode(REND_OPAQUE);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glPopMatrix();

	nb_scshapes = 0;
}

static void pie_DrawRemainingTransShapes(void)
{
	unsigned int i = 0;

	glPushMatrix();
	for (i = 0; i < nb_tshapes; ++i)
	{
		glLoadMatrixf(tshapes[i].matrix);
		pie_Draw3DShape2(tshapes[i].shape, tshapes[i].frame, tshapes[i].colour, tshapes[i].colour,
				 tshapes[i].specular, tshapes[i].flag, tshapes[i].flag_data);
	}
	glPopMatrix();

	nb_tshapes = 0;
}

void pie_RemainingPasses(void)
{
	if(shadows)
	{
		pie_DrawShadows();
	}
	pie_DrawRemainingTransShapes();
}

/***************************************************************************
 * pie_Drawimage
 *
 * General purpose blit function
 * Will support zbuffering, non_textured, coloured lighting and alpha effects
 *
 * replaces all ivis blit functions
 *
 ***************************************************************************/
void pie_DrawImage(const PIEIMAGE *image, const PIERECT *dest)
{
	PIELIGHT colour = WZCOL_WHITE;

	/* Set transparent color to be 0 red, 0 green, 0 blue, 0 alpha */
	polyCount++;

	pie_SetTexturePage(image->texPage);

	glColor4ubv(colour.vector);

	glBegin(GL_TRIANGLE_STRIP);
		//set up 4 pie verts
		glTexCoord2f(image->tu / OLD_TEXTURE_SIZE_FIX, image->tv / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x, dest->y);

		glTexCoord2f((image->tu + image->tw) / OLD_TEXTURE_SIZE_FIX, image->tv / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x + dest->w, dest->y);

		glTexCoord2f(image->tu / OLD_TEXTURE_SIZE_FIX, (image->tv + image->th) / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x, dest->y + dest->h);

		glTexCoord2f((image->tu + image->tw) / OLD_TEXTURE_SIZE_FIX, (image->tv + image->th) / OLD_TEXTURE_SIZE_FIX);
		glVertex2f(dest->x + dest->w, dest->y + dest->h);
	glEnd();
}

void pie_GetResetCounts(unsigned int* pPieCount, unsigned int* pTileCount, unsigned int* pPolyCount, unsigned int* pStateCount)
{
	*pPieCount  = pieCount;
	*pTileCount = tileCount;
	*pPolyCount = polyCount;
	*pStateCount = pieStateCount;

	pieCount = 0;
	tileCount = 0;
	polyCount = 0;
	pieStateCount = 0;
	return;
}
